//******************************************************************************
// Copyright (c) 2003 Tweak Inc.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
//******************************************************************************

#include <TwkGLF/GL.h>
#include <cstdio>
#include <cstring>
#include <sstream>

using namespace TwkMath;
using namespace std;

// #define TRACK_GL_CONTEXT

#ifdef TRACK_GL_CONTEXT
// Needs Qt: add Qt::Gui to TwkGLF's link libraries when enabling this
#include <QOpenGLContext>
#endif

namespace
{

#ifdef TRACK_GL_CONTEXT

#define DECLARE_VAR(typ, var, def) \
    typ m_prev##var = def;         \
    typ m_curr##var = def;

#define PRINT_VAR(var) std::cerr << "[" << #var << ": " << m_prev##var << " to " << m_curr##var << "] ";

#define UPDATEVAR_VAL(var, val)     \
    m_prev##var = m_curr##var;      \
    m_curr##var = val;              \
    if (m_prev##var != m_curr##var) \
    {                               \
        changed = true;             \
    }

#define UPDATEVAR_GLGET(var, ident)     \
    m_prev##var = m_curr##var;          \
    glGetIntegerv(ident, &m_curr##var); \
    if (m_prev##var != m_curr##var)     \
    {                                   \
        changed = true;                 \
    }

    class GLContextStateTracker
    {
    public:
        GLContextStateTracker()
        {
            m_ctx = QOpenGLContext::currentContext();
            m_name = "????";

            std::cerr << "-------> [NEW CONTEXT: " << name() << "]" << std::endl;
        }

        void update(std::string_view file, std::string_view function, int line)
        {
            bool changed = false;
            QOpenGLContext* context = QOpenGLContext::currentContext();

            UPDATEVAR_VAL(QT_FBO, context->defaultFramebufferObject());
            UPDATEVAR_GLGET(GL_FBO, GL_FRAMEBUFFER_BINDING);
            UPDATEVAR_GLGET(GL_RBO, GL_RENDERBUFFER_BINDING);

            if (changed)
            {
                std::cerr << "   [QT_CTX: " << name() << "] ";

                PRINT_VAR(QT_FBO);
                PRINT_VAR(GL_FBO);
                PRINT_VAR(GL_RBO);

                bool printDetails = true;
                if (printDetails)
                    std::cerr << " " << shorterPath(file).data() << " " << function.data() << "@" << line;

                std::cerr << std::endl;
            }
        }

        void setName(std::string_view name) { m_name = name; }

        std::string name() const
        {
            std::ostringstream ss;
            ss << (void*)(m_ctx) << "/" << m_name.c_str();

            std::string s = ss.str();
            return s;
        }

    protected:
        DECLARE_VAR(int32_t, QT_FBO, -1);
        DECLARE_VAR(int32_t, GL_FBO, -1);
        DECLARE_VAR(int32_t, GL_RBO, -1);

        std::string m_name = "";
        void* m_ctx = nullptr;
    };

    class GLMultiContextStateTracker
    {
    public:
        GLMultiContextStateTracker() {}

        void setCurrentContextName(std::string_view name)
        {
            GLContextStateTracker& tracker = m_trackers[QOpenGLContext::currentContext()];
            tracker.setName(name);
        }

        void update(std::string_view file, std::string_view function, int line)
        {
            m_prevContext = m_currContext;
            m_currContext = QOpenGLContext::currentContext();

            if (m_prevContext == nullptr)
                m_prevContext = m_currContext;

            if (m_currContext != m_prevContext)
            {
                const GLContextStateTracker& t0 = m_trackers[m_prevContext];
                const GLContextStateTracker& t1 = m_trackers[m_currContext];

                std::cerr << "@@ [QT_CTX: " << t0.name() << " to " << t1.name() << "]" << std::endl;
            }

            if (m_currContext)
            {
                m_trackers[m_currContext].update(file, function, line);
            }
        }

    protected:
        std::map<QOpenGLContext*, GLContextStateTracker> m_trackers;
        QOpenGLContext* m_prevContext = nullptr;
        QOpenGLContext* m_currContext = nullptr;
    };

    static GLMultiContextStateTracker s_multiTracker;
#endif // TRACK_GL_CONTEXT

    static std::string_view shorterPath(std::string_view path)
    {
        const bool veryshort = true;

        if (veryshort)
        {
            size_t pos = path.rfind('/');
            if (pos != std::string_view::npos)
            {
                std::string_view result = path.substr(pos + 1);
                return result;
            }
        }
        else
        {
            size_t pos = path.find("/src/");
            if (pos != std::string_view::npos)
                return path.substr(pos);
        }

        return path;
    }

    struct token_string
    {
        GLuint Token;
        const char* String;
    };

    static const struct token_string GLErrors[] = {
        {GL_NO_ERROR, "no error"},
        {GL_INVALID_ENUM, "invalid enumerant"},
        {GL_INVALID_VALUE, "invalid value"},
        {GL_INVALID_OPERATION, "invalid operation"},
        {GL_STACK_OVERFLOW, "stack overflow"},
        {GL_STACK_UNDERFLOW, "stack underflow"},
        {GL_OUT_OF_MEMORY, "out of memory"},
        {GL_TABLE_TOO_LARGE, "table too large"},
#ifdef GL_EXT_framebuffer_object
        {GL_INVALID_FRAMEBUFFER_OPERATION_EXT, "invalid framebuffer operation"},
#endif
        {static_cast<GLuint>(~0), NULL} /* end of list indicator */
    };

} // namespace

namespace TwkGLF
{

    string safeGLGetString(GLenum name)
    {
        if (const GLubyte* s = glGetString(name))
            return string((const char*)s);
        else
            return string();
    }

    void glVersion(int& major, int& minor)
    {
        major = 0;
        minor = 0;
        const string version = safeGLGetString(GL_VERSION);
        sscanf(version.c_str(), "%d.%d", &major, &minor);
    }

    bool isCoreProfile()
    {
        //
        //  Values from GL 3.0 / 3.2, which Apple's GL 2.1 headers lack.
        //

        const GLenum contextFlags = 0x821E;       // GL_CONTEXT_FLAGS
        const GLint forwardCompatibleBit = 0x1;   // GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
        const GLenum contextProfileMask = 0x9126; // GL_CONTEXT_PROFILE_MASK
        const GLint coreProfileBit = 0x1;         // GL_CONTEXT_CORE_PROFILE_BIT

        int major, minor;
        glVersion(major, minor);

        if (major < 3)
            return false;

        if (major == 3 && minor < 2)
        {
            GLint flags = 0;
            glGetIntegerv(contextFlags, &flags);
            return (flags & forwardCompatibleBit) != 0;
        }

#if defined(PLATFORM_DARWIN)
        //
        //  macOS only has core profile contexts above GL 2.1.
        //

        return true;
#else
        GLint mask = 0;
        glGetIntegerv(contextProfileMask, &mask);
        return (mask & coreProfileBit) != 0;
#endif
    }

    vector<string> glExtensions()
    {
        vector<string> extensions;
        int major, minor;
        glVersion(major, minor);

        if (major >= 3)
        {
            GLint count = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &count);

            for (GLint i = 0; i < count; i++)
            {
                if (const GLubyte* name = glGetStringi(GL_EXTENSIONS, i))
                    extensions.push_back((const char*)name);
            }
        }
        else
        {
            istringstream names(safeGLGetString(GL_EXTENSIONS));
            string name;
            while (names >> name)
                extensions.push_back(name);
        }

        return extensions;
    }

    bool hasExtension(const char* name)
    {
        if (!name)
            return false;

        //
        //  Extensions RV checks for that became part of core GL, with the
        //  version that added them. GL_ARB_imaging and
        //  GL_ARB_fragment_program never did.
        //

        struct CoreExtension
        {
            const char* name;
            int major;
            int minor;
        };

        static const CoreExtension coreExtensions[] = {
            {"GL_ARB_texture_non_power_of_two", 2, 0},
            {"GL_ARB_pixel_buffer_object", 2, 1},
            {"GL_ARB_texture_float", 3, 0},
            {"GL_ARB_half_float_pixel", 3, 0},
            {"GL_ARB_color_buffer_float", 3, 0},
            {"GL_ARB_framebuffer_object", 3, 0},
            {"GL_ARB_texture_rg", 3, 0},
            {"GL_ARB_vertex_array_object", 3, 0},
            {"GL_ARB_texture_rectangle", 3, 1},
            {"GL_ARB_sync", 3, 2},
            {"GL_ARB_texture_swizzle", 3, 3},
        };

        int major, minor;
        glVersion(major, minor);

        for (const CoreExtension& e : coreExtensions)
        {
            if (!strcmp(name, e.name) && (major > e.major || (major == e.major && minor >= e.minor)))
                return true;
        }

        //
        //  Whole names only: a substring search would find
        //  "GL_ARB_texture_float" inside a longer name.
        //

        for (const string& e : glExtensions())
        {
            if (e == name)
                return true;
        }

        return false;
    }

    string errorString(GLenum errorCode)
    {
        for (size_t i = 0; GLErrors[i].String; i++)
        {
            if (GLErrors[i].Token == errorCode)
                return string(GLErrors[i].String);
        }

        ostringstream str;
        str << "unknown GL error code: " << errorCode;
        return str.str();
    }

} // namespace TwkGLF

bool twkGlPrintError(std::string_view file, std::string_view function, const int line, const std::string_view msg)
{
    if (GLuint err = glGetError())
    {
        std::cerr << "GL_ERROR: " << shorterPath(file).data() << "::" << function.data() << ":" << line << " [" << TwkGLF::errorString(err)
                  << "]" << std::endl;
        return false;
    }

    // Track OpenGL context updates.
    // Enable the line below to track what's going on with current contexts and
    // current FBOs.
#ifdef TRACK_GL_CONTEXT
    s_multiTracker.update(file, function, line);
#endif

    return true;
}

//
//  The pixel store settings of core profile GL, packing then unpacking.
//

static const GLenum pixelStoreNames[GLPixelStoreState::count] = {
    GL_PACK_SWAP_BYTES,  GL_PACK_LSB_FIRST,     GL_PACK_ROW_LENGTH,    GL_PACK_IMAGE_HEIGHT, GL_PACK_SKIP_ROWS,    GL_PACK_SKIP_PIXELS,
    GL_PACK_SKIP_IMAGES, GL_PACK_ALIGNMENT,     GL_UNPACK_SWAP_BYTES,  GL_UNPACK_LSB_FIRST,  GL_UNPACK_ROW_LENGTH, GL_UNPACK_IMAGE_HEIGHT,
    GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_IMAGES, GL_UNPACK_ALIGNMENT};

void GLPixelStoreState::save()
{
    for (int i = 0; i < count; i++)
        glGetIntegerv(pixelStoreNames[i], &values[i]);
}

void GLPixelStoreState::restore() const
{
    for (int i = 0; i < count; i++)
        glPixelStorei(pixelStoreNames[i], values[i]);
}

bool glSupportsExtension(const char* extstring) { return TwkGLF::hasExtension(extstring); }
