//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <TwkGLF/GLText.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLProgram.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkText/Atlas.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

#if defined(PLATFORM_DARWIN)
#include <OpenGL/OpenGL.h>
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#elif defined(PLATFORM_WINDOWS)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> // wglGetCurrentContext
#endif

#ifndef GL_PIXEL_UNPACK_BUFFER
#define GL_PIXEL_UNPACK_BUFFER 0x88EC
#endif
#ifndef GL_PIXEL_UNPACK_BUFFER_BINDING
#define GL_PIXEL_UNPACK_BUFFER_BINDING 0x88EF
#endif

namespace TwkGLF
{
    using namespace std;
    using namespace TwkMath;

    namespace
    {
        //
        //  The current GL context, to keep per-context programs and
        //  textures apart. Linux contexts are either EGL or GLX; look both
        //  up at run time so TwkGLF doesn't need to link either.
        //

        void* currentContext()
        {
#if defined(PLATFORM_DARWIN)
            return (void*)CGLGetCurrentContext();
#elif defined(PLATFORM_WINDOWS)
            return (void*)wglGetCurrentContext();
#elif defined(PLATFORM_LINUX)
            typedef void* (*GetContext)();
            static GetContext egl = (GetContext)dlsym(RTLD_DEFAULT, "eglGetCurrentContext");
            static GetContext glx = (GetContext)dlsym(RTLD_DEFAULT, "glXGetCurrentContext");
            void* c = egl ? egl() : nullptr;
            if (!c && glx)
                c = glx();
            return c;
#else
            return nullptr;
#endif
        }

        //
        //  MTSDF text shader. The distance is the median of the RGB
        //  channels; the screen pixel range (how many screen pixels the
        //  atlas distance range covers) turns it into antialiased coverage.
        //

        const char* vertex120 = "#version 120\n"
                                "attribute vec2 in_Position;\n"
                                "attribute vec2 in_TexCoord;\n"
                                "uniform mat4 projMatrix, modelviewMatrix;\n"
                                "varying vec2 atlasPixel;\n"
                                "void main()\n"
                                "{\n"
                                "    atlasPixel = in_TexCoord;\n"
                                "    gl_Position = projMatrix * modelviewMatrix * vec4(in_Position, 0.0, 1.0);\n"
                                "}\n";

        const char* vertex150 = "#version 150\n"
                                "in vec2 in_Position;\n"
                                "in vec2 in_TexCoord;\n"
                                "uniform mat4 projMatrix, modelviewMatrix;\n"
                                "out vec2 atlasPixel;\n"
                                "void main()\n"
                                "{\n"
                                "    atlasPixel = in_TexCoord;\n"
                                "    gl_Position = projMatrix * modelviewMatrix * vec4(in_Position, 0.0, 1.0);\n"
                                "}\n";

        const char* fragmentBody = "uniform sampler2D atlas;\n"
                                   "uniform vec2 atlasSize;\n"
                                   "uniform float pixelRange;\n"
                                   "uniform vec4 color;\n"
                                   "float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }\n"
                                   "float coverage(vec4 s, vec2 tc)\n"
                                   "{\n"
                                   "    vec2 unitRange = vec2(pixelRange) / atlasSize;\n"
                                   "    vec2 screenTexSize = vec2(1.0) / fwidth(tc);\n"
                                   "    float screenPixelRange = max(0.5 * dot(unitRange, screenTexSize), 1.0);\n"
                                   "    float d = median(s.r, s.g, s.b) - 0.5;\n"
                                   "    return clamp(screenPixelRange * d + 0.5, 0.0, 1.0);\n"
                                   "}\n";

        const char* fragment120 = "    vec2 tc = atlasPixel / atlasSize;\n"
                                  "    float a = coverage(texture2D(atlas, tc), tc);\n"
                                  "    gl_FragColor = vec4(color.rgb, color.a * a);\n"
                                  "}\n";

        const char* fragment150 = "    vec2 tc = atlasPixel / atlasSize;\n"
                                  "    float a = coverage(texture(atlas, tc), tc);\n"
                                  "    FragColor = vec4(color.rgb, color.a * a);\n"
                                  "}\n";

        bool glsl150()
        {
            int major = 0, minor = 0;
            const string version = safeGLGetString(GL_SHADING_LANGUAGE_VERSION);
            sscanf(version.c_str(), "%d.%d", &major, &minor);
            return major > 1 || (major == 1 && minor >= 50);
        }

        class TextProgram : public GLProgram
        {
        public:
            bool compile() override
            {
                const bool modern = glsl150();
                const string vs = modern ? vertex150 : vertex120;
                const string fs =
                    string(modern ? "#version 150\nin vec2 atlasPixel;\nout vec4 FragColor;\n" : "#version 120\nvarying vec2 atlasPixel;\n")
                    + fragmentBody + "void main()\n{\n" + (modern ? fragment150 : fragment120);

                GLuint v = shader(GL_VERTEX_SHADER, vs);
                GLuint f = shader(GL_FRAGMENT_SHADER, fs);
                if (!v || !f)
                    return false;

                GLuint p = glCreateProgram();
                glAttachShader(p, v);
                glAttachShader(p, f);
                glLinkProgram(p);
                glDeleteShader(v);
                glDeleteShader(f);

                GLint linked = GL_FALSE;
                glGetProgramiv(p, GL_LINK_STATUS, &linked);
                if (!linked)
                {
                    char log[2048] = "";
                    glGetProgramInfoLog(p, sizeof(log), nullptr, log);
                    cerr << "ERROR: GLText: linking the text shader: " << log << endl;
                    glDeleteProgram(p);
                    return false;
                }

                m_programId = p;
                collectUniforms();
                collectAttribs();
                return true;
            }

        private:
            static GLuint shader(GLenum type, const string& source)
            {
                GLuint s = glCreateShader(type);
                const char* src = source.c_str();
                glShaderSource(s, 1, &src, nullptr);
                glCompileShader(s);

                GLint ok = GL_FALSE;
                glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
                if (!ok)
                {
                    char log[2048] = "";
                    glGetShaderInfoLog(s, sizeof(log), nullptr, log);
                    cerr << "ERROR: GLText: compiling the text shader: " << log << endl;
                    glDeleteShader(s);
                    return 0;
                }
                return s;
            }
        };

        struct PageTexture
        {
            GLuint id = 0;
            unsigned int version = 0;
            int height = 0;
        };

        struct ContextResources
        {
            TextProgram* program = nullptr;
            bool programFailed = false;
            vector<PageTexture> pages;
        };

        mutex resourcesLock;
        map<void*, ContextResources> resources;

        //
        //  A destroyed context's handle can be reused by a new one, so check
        //  that the objects still exist before using them.
        //

        ContextResources& contextResources()
        {
            lock_guard<mutex> guard(resourcesLock);
            ContextResources& r = resources[currentContext()];

            if (r.program && !glIsProgram(GLuint(r.program->programId())))
            {
                delete r.program;
                r.program = nullptr;
                r.programFailed = false;
                r.pages.clear();
            }

            if (!r.program && !r.programFailed)
            {
                r.program = new TextProgram;
                if (!r.program->compile())
                {
                    delete r.program;
                    r.program = nullptr;
                    r.programFailed = true;
                }
            }

            return r;
        }

        //
        //  Make the page's texture current, uploading it if the atlas page
        //  changed. Requires the atlas lock.
        //

        void bindPage(ContextResources& r, size_t i)
        {
            const TwkText::Atlas::Page& page = TwkText::Atlas::instance().page(i);
            if (r.pages.size() <= i)
                r.pages.resize(i + 1);
            PageTexture& t = r.pages[i];

            if (t.id && !glIsTexture(t.id))
                t = PageTexture();

            const bool create = !t.id;
            if (create)
            {
                glGenTextures(1, &t.id);
                glBindTexture(GL_TEXTURE_2D, t.id);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
            else
            {
                glBindTexture(GL_TEXTURE_2D, t.id);
            }

            if (create || t.version != page.version)
            {
                GLint unpackBuffer = 0, alignment = 4, rowLength = 0, skipRows = 0, skipPixels = 0;
                glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
                glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
                glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
                glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
                glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);

                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

                if (t.height != page.height)
                {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TwkText::Atlas::pageWidth, page.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                                 page.pixels.data());
                    t.height = page.height;
                }
                else
                {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TwkText::Atlas::pageWidth, page.height, GL_RGBA, GL_UNSIGNED_BYTE,
                                    page.pixels.data());
                }

                t.version = page.version;

                glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GLuint(unpackBuffer));
                glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
                glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
                glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
            }

            TWK_GLDEBUG;
        }

        void uniformMatrix(const GLProgram* program, const char* name, const Mat44f& m)
        {
            Mat44f copy = m;
            setUniformGLMatrix(GLuint(program->programId()), program->uniformLocation(name), &copy[0][0]);
        }

    } // namespace

    bool useFTGLText()
    {
        static const bool ftgl = []()
        {
            const char* env = getenv("RV_TEXT_RENDERER");
            return env && string(env) == "ftgl";
        }();
        return ftgl;
    }

    void drawText(const TwkText::GlyphQuads& quads, const Mat44f& projection, const Mat44f& modelview, const Col4f& color,
                  GLVBO::GLVBOVector& vbos)
    {
        if (quads.empty())
            return;

        ContextResources& r = contextResources();
        if (!r.program)
            return;

        GLint previousProgram = 0, previousUnit = GL_TEXTURE0, previousTexture = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousUnit);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

        const GLProgram* program = r.program;
        program->use();

        uniformMatrix(program, "projMatrix", projection);
        uniformMatrix(program, "modelviewMatrix", modelview);
        glUniform4f(program->uniformLocation("color"), color.x, color.y, color.z, color.w);
        glUniform1f(program->uniformLocation("pixelRange"), TwkText::Atlas::pixelRange);
        glUniform1i(program->uniformLocation("atlas"), 0);

        RenderPrimitives::VertexAttributeList attributes;
        attributes.push_back(VertexAttribute("in_Position", GL_FLOAT, 2, 0, 4 * sizeof(float)));
        attributes.push_back(VertexAttribute("in_TexCoord", GL_FLOAT, 2, 2 * sizeof(float), 4 * sizeof(float)));

        TwkText::Atlas& atlas = TwkText::Atlas::instance();
        lock_guard<mutex> guard(atlas.lock());

        //
        //  One draw per atlas page.
        //

        for (size_t page = 0; page < atlas.pageCount(); page++)
        {
            vector<float> data;
            for (const TwkText::GlyphQuad& q : quads)
            {
                if (size_t(q.page) != page)
                    continue;

                const float v[4][4] = {{q.p0.x, q.p0.y, q.t0.x, q.t0.y},
                                       {q.p1.x, q.p0.y, q.t1.x, q.t0.y},
                                       {q.p1.x, q.p1.y, q.t1.x, q.t1.y},
                                       {q.p0.x, q.p1.y, q.t0.x, q.t1.y}};
                const int order[6] = {0, 1, 2, 0, 2, 3};
                for (int k : order)
                    data.insert(data.end(), v[k], v[k] + 4);
            }

            if (data.empty())
                continue;

            bindPage(r, page);
            glUniform2f(program->uniformLocation("atlasSize"), float(TwkText::Atlas::pageWidth), float(atlas.page(page).height));

            const size_t vertices = data.size() / 4;
            PrimitiveData buffer(&data.front(), nullptr, GL_TRIANGLES, vertices, vertices / 3, data.size() * sizeof(float));
            RenderPrimitives primitives(program, buffer, attributes, vbos);
            primitives.setupAndRender();
        }

        glBindTexture(GL_TEXTURE_2D, GLuint(previousTexture));
        glActiveTexture(GLenum(previousUnit));
        glUseProgram(GLuint(previousProgram));
        TWK_GLDEBUG;
    }

} // namespace TwkGLF
