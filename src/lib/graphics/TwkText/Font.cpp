//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <TwkText/Font.h>
#include <TwkText/Atlas.h>
#include <TwkText/defaultFont.h>
#include <TwkUtil/File.h>
#include <TwkUtil/PathConform.h>
#include <stl_ext/string_algo.h>

//
//  FreeType first: msdfgen's import-font.h only declares
//  adoptFreetypeFont() when FreeType is already included.
//

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <msdfgen/msdfgen.h>
#include <msdfgen/ext/import-font.h>

#ifdef TWK_TEXT_HAVE_HARFBUZZ
#include <hb.h>
#include <hb-ft.h>
#endif

#ifdef PLATFORM_DARWIN
#include <CoreFoundation/CoreFoundation.h>
#include <CoreText/CoreText.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>

namespace TwkText
{
    using namespace std;
    using namespace TwkMath;

    //
    //  The same glyph loading as FTGL's texture fonts, so advances and
    //  outline bounds match TwkGLText.
    //

    static const FT_Int32 loadFlags = FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;

    struct GlyphImage
    {
        bool empty = true;
        int page = 0;
        int x = 0, y = 0, w = 0, h = 0;                  // atlas pixels
        double left = 0, bottom = 0, right = 0, top = 0; // em units
    };

    struct Face
    {
        string name; // file path, or "" for the built-in font
        FT_Face ftFace = nullptr;
        msdfgen::FontHandle* msdf = nullptr;
#ifdef TWK_TEXT_HAVE_HARFBUZZ
        hb_font_t* hbFont = nullptr;
#endif
        unsigned int currentSize = 0;
        map<unsigned int, GlyphImage> glyphs;
    };

    namespace
    {
        //
        //  Everything below is shared by the whole process and used while
        //  holding Atlas::instance().lock().
        //

        struct Library
        {
            FT_Library ft = nullptr;
            map<string, Face*> faces;       // by file path ("" = built-in)
            map<uint32_t, Face*> fallbacks; // by code point (nullptr = none)
            vector<string> fallbackFiles;   // RV_TEXT_FALLBACK_FONTS
            set<string> warned;

            Library()
            {
                if (FT_Init_FreeType(&ft))
                {
                    cerr << "ERROR: TwkText: can't initialize FreeType" << endl;
                    ft = nullptr;
                }

                if (const char* env = getenv("RV_TEXT_FALLBACK_FONTS"))
                {
#ifdef PLATFORM_WINDOWS
                    const char* separators = ";";
#else
                    const char* separators = ":";
#endif
                    stl_ext::tokenize(fallbackFiles, env, separators);
                }
            }
        };

        Library& library()
        {
            static Library lib;
            return lib;
        }

        Face* newFace(FT_Face ftFace, const string& name)
        {
            Face* face = new Face;
            face->name = name;
            face->ftFace = ftFace;
            face->msdf = msdfgen::adoptFreetypeFont(ftFace);
#ifdef TWK_TEXT_HAVE_HARFBUZZ
            face->hbFont = hb_ft_font_create_referenced(ftFace);
            hb_ft_font_set_load_flags(face->hbFont, loadFlags);
#endif
            return face;
        }

        Face* defaultFace()
        {
            Library& lib = library();
            auto it = lib.faces.find("");
            if (it != lib.faces.end())
                return it->second;

            FT_Face ftFace = nullptr;
            if (!lib.ft || FT_New_Memory_Face(lib.ft, default_font, FT_Long(default_fontSize), 0, &ftFace))
            {
                cerr << "ERROR: TwkText: can't load the built-in font" << endl;
                lib.faces[""] = nullptr;
                return nullptr;
            }

            Face* face = newFace(ftFace, "");
            lib.faces[""] = face;
            return face;
        }

        //
        //  A font file, choosing the face with the given PostScript name
        //  from a collection (.ttc), or the first face.
        //

        Face* fileFace(const string& path, const string& postScriptName = "")
        {
            Library& lib = library();
            const string key = postScriptName.empty() ? path : path + "#" + postScriptName;
            auto it = lib.faces.find(key);
            if (it != lib.faces.end())
                return it->second;

            Face* face = nullptr;
            FT_Face probe = nullptr;

            if (lib.ft && !FT_New_Face(lib.ft, path.c_str(), -1, &probe))
            {
                const FT_Long count = probe->num_faces;
                FT_Done_Face(probe);

                FT_Long index = 0;
                if (!postScriptName.empty())
                {
                    for (FT_Long i = 0; i < count; i++)
                    {
                        FT_Face f = nullptr;
                        if (FT_New_Face(lib.ft, path.c_str(), i, &f))
                            continue;
                        const char* ps = FT_Get_Postscript_Name(f);
                        const bool match = ps && postScriptName == ps;
                        FT_Done_Face(f);
                        if (match)
                        {
                            index = i;
                            break;
                        }
                    }
                }

                FT_Face ftFace = nullptr;
                if (!FT_New_Face(lib.ft, path.c_str(), index, &ftFace))
                    face = newFace(ftFace, path);
            }

            lib.faces[key] = face;
            return face;
        }

        bool hasGlyph(const Face* face, uint32_t c) { return face && FT_Get_Char_Index(face->ftFace, c) != 0; }

#ifdef PLATFORM_DARWIN
        //
        //  CoreText's fallback (cascade) list for the system UI font picks a
        //  font that has the character; FreeType then opens its file.
        //

        Face* systemFallback(uint32_t c)
        {
            const uint32_t utf32 = CFSwapInt32HostToLittle(c);
            CFStringRef str = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8*)&utf32, 4, kCFStringEncodingUTF32LE, false);
            if (!str)
                return nullptr;

            Face* face = nullptr;
            CTFontRef base = CTFontCreateWithName(CFSTR("Helvetica"), 12.0, nullptr);
            CTFontRef font = base ? CTFontCreateForString(base, str, CFRangeMake(0, CFStringGetLength(str))) : nullptr;

            if (font)
            {
                CFStringRef psName = CTFontCopyPostScriptName(font);
                CFURLRef url = (CFURLRef)CTFontCopyAttribute(font, kCTFontURLAttribute);
                char path[4096];
                char ps[512];

                if (url && psName && CFURLGetFileSystemRepresentation(url, true, (UInt8*)path, sizeof(path))
                    && CFStringGetCString(psName, ps, sizeof(ps), kCFStringEncodingUTF8) && strcmp(ps, "LastResort") != 0)
                {
                    face = fileFace(path, ps);
                }

                if (url)
                    CFRelease(url);
                if (psName)
                    CFRelease(psName);
                CFRelease(font);
            }

            if (base)
                CFRelease(base);
            CFRelease(str);
            return face;
        }
#else
        //
        //  TODO: fontconfig (Linux) and DirectWrite (Windows) fallback. Until
        //  then RV_TEXT_FALLBACK_FONTS is the only fallback there.
        //

        Face* systemFallback(uint32_t) { return nullptr; }
#endif

        Face* fallbackFace(uint32_t c)
        {
            Library& lib = library();
            auto it = lib.fallbacks.find(c);
            if (it != lib.fallbacks.end())
                return it->second;

            Face* found = nullptr;

            for (const string& file : lib.fallbackFiles)
            {
                Face* face = fileFace(file);
                if (hasGlyph(face, c))
                {
                    found = face;
                    break;
                }
            }

            if (!found)
            {
                Face* face = systemFallback(c);
                if (hasGlyph(face, c))
                    found = face;
            }

            lib.fallbacks[c] = found;
            return found;
        }

        void setSize(Face* face, unsigned int size)
        {
            if (face->currentSize == size)
                return;

            //
            //  As FTGL's FTSize::CharSize(): size points at 72 dpi, so one em
            //  is size pixels.
            //

            FT_Set_Char_Size(face->ftFace, FT_F26Dot6(size) * 64, FT_F26Dot6(size) * 64, 72, 72);
            face->currentSize = size;
#ifdef TWK_TEXT_HAVE_HARFBUZZ
            hb_ft_font_changed(face->hbFont);
#endif
        }

        //
        //  UTF-8 to code points. Invalid bytes become U+FFFD.
        //

        void decode(const string& text, vector<uint32_t>& out)
        {
            const unsigned char* s = (const unsigned char*)text.data();
            const size_t n = text.size();

            for (size_t i = 0; i < n;)
            {
                const unsigned char c = s[i];
                uint32_t cp = 0;
                size_t len = 0;

                if (c < 0x80)
                {
                    cp = c;
                    len = 1;
                }
                else if ((c & 0xE0) == 0xC0)
                {
                    cp = c & 0x1F;
                    len = 2;
                }
                else if ((c & 0xF0) == 0xE0)
                {
                    cp = c & 0x0F;
                    len = 3;
                }
                else if ((c & 0xF8) == 0xF0)
                {
                    cp = c & 0x07;
                    len = 4;
                }

                bool ok = len != 0 && i + len <= n;
                for (size_t k = 1; ok && k < len; k++)
                {
                    if ((s[i + k] & 0xC0) != 0x80)
                        ok = false;
                    else
                        cp = (cp << 6) | (s[i + k] & 0x3F);
                }

                //
                //  Reject overlong forms, surrogates and values past U+10FFFF.
                //

                if (ok
                    && ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800) || (len == 4 && cp < 0x10000) || (cp >= 0xD800 && cp <= 0xDFFF)
                        || cp > 0x10FFFF))
                {
                    ok = false;
                }

                if (ok)
                {
                    out.push_back(cp);
                    i += len;
                }
                else
                {
                    out.push_back(0xFFFD);
                    i++;
                }
            }
        }

        //
        //  A glyph and its pen position (the glyph origin), in pixels at the
        //  font's size relative to the start of the line.
        //

        struct Placed
        {
            Face* face;
            unsigned int glyph;
            double x;
            double y;
        };

        void shapeRun(Face* face, unsigned int size, const uint32_t* cps, size_t count, double& pen, vector<Placed>& out)
        {
            setSize(face, size);

#ifdef TWK_TEXT_HAVE_HARFBUZZ
            hb_buffer_t* buffer = hb_buffer_create();
            hb_buffer_add_codepoints(buffer, cps, int(count), 0, int(count));
            hb_buffer_guess_segment_properties(buffer);

            //
            //  FTGL (TwkGLText) applied neither kerning (see the version
            //  without HarfBuzz below) nor ligatures, so existing
            //  annotations were laid out without them. Turn those off to
            //  keep their glyphs where they were.
            //  The features scripts need to be correct (Arabic forms,
            //  Indic reordering, mark positioning) stay on.
            //

            static const hb_feature_t features[] = {
                {HB_TAG('k', 'e', 'r', 'n'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END},
                {HB_TAG('l', 'i', 'g', 'a'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END},
                {HB_TAG('c', 'l', 'i', 'g'), 0, HB_FEATURE_GLOBAL_START, HB_FEATURE_GLOBAL_END},
            };

            hb_shape(face->hbFont, buffer, features, sizeof(features) / sizeof(features[0]));

            unsigned int n = 0;
            const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buffer, &n);
            const hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buffer, &n);

            //
            //  hb_ft fonts are scaled in 26.6 fixed point pixels.
            //

            for (unsigned int i = 0; i < n; i++)
            {
                out.push_back({face, info[i].codepoint, pen + pos[i].x_offset / 64.0, pos[i].y_offset / 64.0});
                pen += pos[i].x_advance / 64.0;
            }

            hb_buffer_destroy(buffer);
#else
            //
            //  The pen moves by the glyph advance. No kerning, to match FTGL
            //  (TwkGLText): it fills its kerning cache before a size is set,
            //  so pairs of glyphs with indices below 128 (all common Latin
            //  glyphs) were never kerned.
            //

            FT_Face ft = face->ftFace;

            for (size_t i = 0; i < count; i++)
            {
                const FT_UInt glyph = FT_Get_Char_Index(ft, cps[i]);
                out.push_back({face, glyph, pen, 0.0});

                if (!FT_Load_Glyph(ft, glyph, loadFlags))
                    pen += ft->glyph->advance.x / 64.0;
            }
#endif
        }

        //
        //  Lay out one line: each character uses the font if it has the
        //  glyph, otherwise a fallback font, and runs of characters in one
        //  font are shaped together.
        //

        void place(Face* primary, unsigned int size, const string& text, vector<Placed>& out)
        {
            vector<uint32_t> cps;
            decode(text, cps);
            if (cps.empty())
                return;

            vector<Face*> faces(cps.size());
            for (size_t i = 0; i < cps.size(); i++)
            {
                const uint32_t c = cps[i];
                Face* face = primary;

                if (c >= 0x20 && !hasGlyph(primary, c))
                {
                    if (Face* f = fallbackFace(c))
                        face = f;
                }

                faces[i] = face;
            }

            double pen = 0;
            size_t start = 0;
            for (size_t i = 1; i <= cps.size(); i++)
            {
                if (i == cps.size() || faces[i] != faces[start])
                {
                    shapeRun(faces[start], size, &cps[start], i - start, pen, out);
                    start = i;
                }
            }
        }

        //
        //  The glyph outline's box at the size, as FTGL's FTBBox: an empty
        //  outline gives a zero box at the origin.
        //

        Box2f outlineBox(Face* face, unsigned int size, unsigned int glyph)
        {
            setSize(face, size);
            FT_BBox box = {0, 0, 0, 0};

            if (!FT_Load_Glyph(face->ftFace, glyph, loadFlags) && face->ftFace->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
                FT_Outline_Get_CBox(&face->ftFace->glyph->outline, &box);

            return Box2f(Vec2f(box.xMin / 64.0f, box.yMin / 64.0f), Vec2f(box.xMax / 64.0f, box.yMax / 64.0f));
        }

        //
        //  Generate the glyph's MTSDF image and add it to the atlas.
        //

        const GlyphImage& glyphImage(Face* face, unsigned int glyph)
        {
            auto it = face->glyphs.find(glyph);
            if (it != face->glyphs.end())
                return it->second;

            GlyphImage& image = face->glyphs[glyph];

            msdfgen::Shape shape;
            if (!msdfgen::loadGlyph(shape, face->msdf, msdfgen::GlyphIndex(glyph), msdfgen::FONT_SCALING_EM_NORMALIZED))
                return image;

            shape.normalize();
            if (shape.contours.empty())
                return image;

            const msdfgen::Shape::Bounds bounds = shape.getBounds();
            if (!(bounds.r > bounds.l && bounds.t > bounds.b))
                return image;

            msdfgen::edgeColoringInkTrap(shape, 3.0);

            const double scale = Atlas::emPixels;
            const double range = Atlas::pixelRange;
            const int pad = int(std::ceil(range / 2.0)) + 1;
            const int w = int(std::ceil((bounds.r - bounds.l) * scale)) + 2 * pad;
            const int h = int(std::ceil((bounds.t - bounds.b) * scale)) + 2 * pad;
            const double tx = pad / scale - bounds.l;
            const double ty = pad / scale - bounds.b;

            msdfgen::Bitmap<float, 4> bitmap(w, h);
            const msdfgen::Projection projection(msdfgen::Vector2(scale), msdfgen::Vector2(tx, ty));
            const msdfgen::SDFTransformation transformation(projection, msdfgen::DistanceMapping(msdfgen::Range(range / scale)));
            msdfgen::generateMTSDF(bitmap, shape, transformation, msdfgen::MSDFGeneratorConfig(true));

            //
            //  Make inside/outside follow the nonzero fill rule whatever the
            //  font's contour direction.
            //

            msdfgen::distanceSignCorrection(bitmap, shape, projection, 0.5f, msdfgen::FILL_NONZERO);

            vector<unsigned char> pixels(size_t(w) * h * 4);
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    const float* p = bitmap(x, y);
                    unsigned char* q = &pixels[(size_t(y) * w + x) * 4];
                    for (int c = 0; c < 4; c++)
                        q[c] = (unsigned char)(std::min(std::max(p[c], 0.0f), 1.0f) * 255.0f + 0.5f);
                }
            }

            Atlas::instance().add(w, h, pixels.data(), image.page, image.x, image.y);

            image.empty = false;
            image.w = w;
            image.h = h;
            image.left = -tx;
            image.bottom = -ty;
            image.right = w / scale - tx;
            image.top = h / scale - ty;
            return image;
        }

        void lines(const string& text, vector<string>& out) { stl_ext::tokenize(out, text, "\n"); }

    } // namespace

    Font::Font()
        : m_face(nullptr)
        , m_size(0)
    {
    }

    Font::Font(Face* face, unsigned int size)
        : m_face(face)
        , m_size(size)
    {
    }

    Font Font::load(const std::string& name, unsigned int size)
    {
        lock_guard<mutex> guard(Atlas::instance().lock());

        if (name.empty())
            return Font(defaultFace(), size);

        const string path = TwkUtil::pathConform(name);
        Face* face = TwkUtil::fileExists(path.c_str()) ? fileFace(path) : nullptr;

        if (!face)
        {
            Library& lib = library();
            if (lib.warned.insert(name).second)
                cerr << "WARNING: can't open '" << name << "'. Using default font." << endl;
            face = defaultFace();
        }

        return Font(face, size);
    }

    float Font::ascender() const
    {
        if (!m_face)
            return 0.0f;
        lock_guard<mutex> guard(Atlas::instance().lock());
        setSize(m_face, m_size);
        return m_face->ftFace->size->metrics.ascender / 64.0f;
    }

    float Font::descender() const
    {
        if (!m_face)
            return 0.0f;
        lock_guard<mutex> guard(Atlas::instance().lock());
        setSize(m_face, m_size);
        return m_face->ftFace->size->metrics.descender / 64.0f;
    }

    float Font::lineAdvance(float mult) const { return mult * (ascender() - 2.0f * descender()); }

    Box2f Font::bounds(const std::string& text) const
    {
        Box2f total(Vec2f(0.0f), Vec2f(0.0f));
        if (!m_face || text.empty())
            return total;

        lock_guard<mutex> guard(Atlas::instance().lock());
        vector<Placed> glyphs;
        place(m_face, m_size, text, glyphs);

        bool first = true;
        for (const Placed& g : glyphs)
        {
            Box2f b = outlineBox(g.face, m_size, g.glyph);
            const Vec2f offset(float(g.x), float(g.y));
            b.min += offset;
            b.max += offset;

            if (first)
                total = b;
            else
                total.extendBy(b);
            first = false;
        }

        return total;
    }

    Box2f Font::boundsNL(const std::string& text, float mult) const
    {
        vector<string> ls;
        lines(text, ls);
        const float advance = lineAdvance(mult);

        //
        //  As GLtext::boundsNL(), which moves only the bottom of each later
        //  line down by the line advance.
        //

        Box2f total;
        total.makeEmpty();
        for (size_t i = 0; i < ls.size(); i++)
        {
            Box2f b = bounds(ls[i]);
            b.min.y -= advance * i;
            total.extendBy(b);
        }
        return total;
    }

    void Font::layout(const std::string& text, float x, float y, GlyphQuads& quads) const
    {
        if (!m_face || text.empty())
            return;

        lock_guard<mutex> guard(Atlas::instance().lock());
        vector<Placed> glyphs;
        place(m_face, m_size, text, glyphs);

        const float s = float(m_size);
        for (const Placed& g : glyphs)
        {
            const GlyphImage& image = glyphImage(g.face, g.glyph);
            if (image.empty)
                continue;

            const float ox = x + float(g.x);
            const float oy = y + float(g.y);

            GlyphQuad q;
            q.page = image.page;
            q.p0 = Vec2f(ox + float(image.left) * s, oy + float(image.bottom) * s);
            q.p1 = Vec2f(ox + float(image.right) * s, oy + float(image.top) * s);
            q.t0 = Vec2f(float(image.x), float(image.y));
            q.t1 = Vec2f(float(image.x + image.w), float(image.y + image.h));
            quads.push_back(q);
        }
    }

    int Font::layoutNL(const std::string& text, float x, float y, float mult, GlyphQuads& quads) const
    {
        vector<string> ls;
        lines(text, ls);
        const float advance = lineAdvance(mult);

        //
        //  As GLtext::writeAtNL(): each line above the previous one.
        //

        for (size_t i = 0; i < ls.size(); i++)
            layout(ls[i], x, y + advance * i, quads);

        return int(ls.size());
    }

    std::string Font::fallbackFontsDescription()
    {
        lock_guard<mutex> guard(Atlas::instance().lock());
        ostringstream out;
        for (const string& f : library().fallbackFiles)
            out << f << ", ";
#ifdef PLATFORM_DARWIN
        out << "CoreText font fallback";
#else
        out << "no system font fallback";
#endif
        return out.str();
    }

} // namespace TwkText
