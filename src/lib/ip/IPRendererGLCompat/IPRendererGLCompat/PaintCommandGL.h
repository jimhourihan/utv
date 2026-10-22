//
// Copyright (C) 2025 Autodesk, Inc. All Rights Reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPRendererGLCompat__PaintCommandGL__h__
#define __IPRendererGLCompat__PaintCommandGL__h__
#include <IPCore/PaintCommand.h>
#include <TwkGLF/GLState.h>
#include <TwkGLF/GLFBO.h>
#include <TwkGLF/GLRenderPrimitives.h>
#include <TwkText/Font.h>

//
//  Drawing the paint commands (IPCore/PaintCommand.h) with GL
//

namespace IPCore
{

    namespace Paint
    {

        typedef TwkGLF::GLFBO GLFBO;
        typedef TwkGLF::GLState GLState;
        typedef TwkGLF::PrimitiveData PrimitiveData;
        typedef TwkGLF::VertexAttribute VertexAttribute;

        class Brush
        {
        public:
            typedef TwkFB::FrameBuffer FrameBuffer;

            Brush(const std::string& name, FrameBuffer* fb, bool radial)
                : m_fb(fb)
                , m_name(name)
                , m_radial(radial)
                , m_id(-1)
            {
            }

            const FrameBuffer* fb() const { return m_fb; }

            unsigned int id() const { return m_id; }

            const std::string& name() const { return m_name; }

        private:
            FrameBuffer* m_fb;
            std::string m_name;
            bool m_radial;
            unsigned int m_id;
            friend class DeclareBrushes;
        };

        class Font
        {
        public:
            Font(const std::string& name)
                : m_name(name)
                , m_initialized(false)
                , m_size(0)
            {
            }

            const std::string& name() const { return m_name; }

            void init() const;

            bool initialized() const { return m_initialized; }

            void setSize(unsigned int s) const;
            void setColor(float, float, float, float) const;
            std::pair<float, float> computeText(const std::string&, std::string&, float space, std::string origin) const;
            float globalAscenderHeight() const;
            float globalDescenderHeight() const;

            //
            //  The font at the current size, for drawing with TwkGLF::drawText()
            //  (unless TwkGLF::useFTGLText()).
            //

            const TwkText::Font& textFont() const { return m_text; }

        private:
            TwkMath::Box2f boundsNL(const std::string& text, float space) const;

            std::string m_name;
            mutable bool m_initialized;
            mutable unsigned int m_size;
            mutable TwkText::Font m_text;
            friend class DeclareFonts;
        };

        class CommandContext
        {
        public:
            explicit CommandContext(TwkMath::Mat44f pid, TwkMath::Mat44f m, const GLFBO* i, const GLFBO* t, const GLFBO* c, GLState*& g,
                                    bool hasSten, TwkMath::Vec4f sten = TwkMath::Vec4f(0.0))
            {
                hasStencil = hasSten;
                stencilBox = sten;
                projMatrix = pid;
                modelviewMatrix = m;
                initialRender = i;
                currentTexture = t;
                currentRender = c;
                glState = g;
            }

            ~CommandContext() {}

            bool hasStencil;
            TwkMath::Vec4f stencilBox;

            TwkMath::Mat44f projMatrix;
            TwkMath::Mat44f modelviewMatrix;
            const GLFBO* initialRender;
            const GLFBO* currentTexture;
            const GLFBO* currentRender;
            GLState* glState;
        };

        struct PaintContext
        {
            PaintContext()
                : initialRender(0)
                , tempRender1(0)
                , tempRender2(0)
                , cachedfbo(0)
                , image(0)
                , hasStencil(false)
                , updateCache(true)
                , cacheUpdated(false)
                , lastCommand(0)
                , commandExecuted(0)
            {
            }

            const GLFBO* initialRender;
            const GLFBO* tempRender1;
            const GLFBO* tempRender2;
            const GLFBO* cachedfbo;
            const IPImage* image;
            bool hasStencil;
            TwkMath::Vec4f stencilBox;
            bool updateCache;
            bool cacheUpdated;
            const Paint::Command* lastCommand;
            IPImage::PaintCommands commands;
            GLState* glState;
            size_t commandExecuted;
        };

        void renderPaintCommands(PaintContext&);

    } // namespace Paint

} // namespace IPCore

#endif // __IPRendererGLCompat__PaintCommandGL__h__
