//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkText__Font__h__
#define __TwkText__Font__h__

#include <TwkMath/Box.h>
#include <TwkMath/Vec2.h>
#include <string>
#include <vector>

namespace TwkText
{

    //
    //  One glyph to draw: a rectangle in layout units (pixels at the font's
    //  size, y up, the baseline of the first line at the position passed to
    //  layout()) and the matching rectangle of an atlas page in atlas
    //  pixels. The glyph images are multi-channel signed distance fields
    //  with a true distance in alpha (MTSDF); see Atlas.
    //

    struct GlyphQuad
    {
        int page;
        TwkMath::Vec2f p0; // bottom-left corner
        TwkMath::Vec2f p1; // top-right corner
        TwkMath::Vec2f t0; // atlas pixel matching p0
        TwkMath::Vec2f t1; // atlas pixel matching p1
    };

    typedef std::vector<GlyphQuad> GlyphQuads;

    //
    //  Font
    //
    //  A font face at a size. It measures and lays out UTF-8 text the way
    //  TwkGLText (FTGL) does, so existing annotations keep their position:
    //  the size is in pixels per em, glyph outlines are unhinted, the pen
    //  advances by the glyph advance plus kerning, and bounds are the union
    //  of the glyph outline boxes. With HarfBuzz (TWK_TEXT_HAVE_HARFBUZZ),
    //  text is shaped, so kerning and ligatures come from the font's
    //  OpenType tables and complex scripts are laid out correctly.
    //  Characters the face doesn't have come from fallback fonts (see
    //  fallbackFontsDescription()).
    //
    //  Fonts are cheap handles to faces cached for the whole process, and
    //  are safe to use from any thread.
    //

    struct Face;

    class Font
    {
    public:
        Font();

        //
        //  An empty name is the built-in default font. Otherwise the name is
        //  a font file path; if it can't be opened the default font is used
        //  (with a warning), like GLtext::init(name).
        //

        static Font load(const std::string& name, unsigned int pixelSize);

        bool valid() const { return m_face != nullptr; }

        unsigned int size() const { return m_size; }

        //
        //  Font metrics at this size (GLtext::globalAscenderHeight() /
        //  globalDescenderHeight()). The descender is negative.
        //

        float ascender() const;
        float descender() const;

        //
        //  Distance between lines for layoutNL() and boundsNL():
        //  mult * (ascender - 2 * descender), as in GLtext.
        //

        float lineAdvance(float mult = 1.0f) const;

        //
        //  Bounds of the text's glyph outlines with its baseline at the
        //  origin (GLtext::bounds()), and of several lines
        //  (GLtext::boundsNL(), including its handling of line offsets).
        //

        TwkMath::Box2f bounds(const std::string& text) const;
        TwkMath::Box2f boundsNL(const std::string& text, float mult = 1.0f) const;

        //
        //  Append the glyphs of the text, with its baseline starting at
        //  (x, y), to quads (GLtext::writeAt()). layoutNL() lays out one
        //  line per '\n', each lineAdvance(mult) above the previous one, and
        //  returns the number of lines (GLtext::writeAtNL()).
        //

        void layout(const std::string& text, float x, float y, GlyphQuads& quads) const;
        int layoutNL(const std::string& text, float x, float y, float mult, GlyphQuads& quads) const;

        //
        //  Where characters missing from a font are looked for, e.g. for
        //  logging: the files in RV_TEXT_FALLBACK_FONTS (separated by ':',
        //  or ';' on Windows) first, then the system's font fallback.
        //

        static std::string fallbackFontsDescription();

    private:
        Font(Face* face, unsigned int size);

        Face* m_face;
        unsigned int m_size;
    };

} // namespace TwkText

#endif // __TwkText__Font__h__
