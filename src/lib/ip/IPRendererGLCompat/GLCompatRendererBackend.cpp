//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <IPRendererGLCompat/GLCompatRendererBackend.h>
#include <IPRendererGLCompat/GLShaderCompiler.h>
#include <IPRendererGLCompat/HoverPreviewOverlayGL.h>
#include <IPRendererGLCompat/ImageRenderer.h>
#include <TwkGLF/GL.h>
#include <TwkGLF/GLVideoDevice.h>

namespace IPCore
{
    using namespace std;

    bool GLCompatRendererBackend::initialized() const { return ImageRenderer::queryGLFinished(); }

    bool GLCompatRendererBackend::initialize(const TwkApp::VideoDevice* control)
    {
#if defined(PLATFORM_DARWIN)
        // Darwin Metal hybrid path: IPCore renders through an offscreen GL context
        // owned by QTMetalVideoDevice.  Make the control device's context current
        // when available; defer until the first render if it is not ready yet
        // (initialized() stays false).
        if (const TwkGLF::GLVideoDevice* glDev = dynamic_cast<const TwkGLF::GLVideoDevice*>(control))
        {
            glDev->makeCurrent();
        }
        else
        {
            return false;
        }

        if (TwkGLF::safeGLGetString(GL_VERSION).empty())
            return false;
#endif
        ImageRenderer::queryGL();
        return true;
    }

    void GLCompatRendererBackend::storeInfo(IPNode* sessionNode) const
    {
#if defined(PLATFORM_DARWIN)
        // Defer until initialize() can make the control device's GL context
        // current (Metal hybrid path at session construction).
        if (TwkGLF::safeGLGetString(GL_VERSION).empty())
            return;
#endif
        ImageRenderer::queryGLIntoContainer(sessionNode);
    }

    RendererCapabilities GLCompatRendererBackend::capabilities() const
    {
        RendererCapabilities c;
        c.api = "OpenGL";
        c.floatFormats = ImageRenderer::hasFloatFormats();
        c.clientStorage = ImageRenderer::queryClientStorage();
        c.threadedUpload = ImageRenderer::useThreadedUpload();
        return c;
    }

    void GLCompatRendererBackend::printInfo(ostream& o) const { ImageRenderer::printGLInfo(o); }

    bool GLCompatRendererBackend::setDebugOption(const string& flag)
    {
        if (flag == "gpu")
            ImageRenderer::debugGpu(true);
        else if (flag == "passes")
            ImageRenderer::setPassDebug(true);
        else if (flag == "imagefbo")
            ImageRenderer::setOutputIntermediateDebug(true);
        else if (flag == "nogpucache")
            ImageRenderer::setNoGPUCache(true);
        else if (flag == "imagefbolog")
            ImageRenderer::setIntermediateLogging(true);
        else
            return false;
        return true;
    }

    Renderer* GLCompatRendererBackend::newRenderer(const string& type) { return new ImageRenderer(type); }

    HoverPreviewOverlay* GLCompatRendererBackend::newHoverPreviewOverlay() { return new HoverPreviewOverlayGL(); }

    void registerGLCompatRendererBackend()
    {
        static GLCompatRendererBackend backend;
        static Shader::GLShaderCompiler compiler;
        registerRendererBackend(&backend);
        Shader::setCompiler(&compiler);
    }

} // namespace IPCore
