//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <IPRendererGLCompat/GLCompatRendererBackend.h>
#include <IPRendererGLCompat/GLShaderCompiler.h>
#include <IPRendererGLCompat/HoverPreviewOverlayGL.h>
#include <IPRendererGLCompat/ImageRenderer.h>
#include <IPCore/ShaderFunction.h>
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
        const bool first = !initialized();
        ImageRenderer::queryGL();

        //
        //  The GLSL version the shaders are written for. Without this
        //  Shader::Function asks the compiler when it first needs it.
        //

        if (first)
        {
            const string glsl = TwkGLF::safeGLGetString(GL_SHADING_LANGUAGE_VERSION);
            if (!glsl.empty())
                Shader::Function::useShadingLanguageVersion(glsl.c_str());
        }

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
        c.version = ImageRenderer::glVersion();
        c.vendor = ImageRenderer::glVendor();
        c.device = ImageRenderer::glRenderer();
        c.floatFormats = ImageRenderer::hasFloatFormats();
        c.clientStorage = ImageRenderer::queryClientStorage();
        c.threadedUpload = ImageRenderer::queryThreadedUpload();
        c.acceptableTypes = ImageRenderer::fbAcceptableTypes();
        return c;
    }

    //
    //  The settings are ImageRenderer's statics, which every ImageRenderer
    //  reads when it renders, so setOptions() reaches existing renderers.
    //  Background has the same order as ImageRenderer::BGPattern.
    //

    RendererOptions GLCompatRendererBackend::options() const
    {
        RendererOptions o;
        o.defaultBackground = RendererOptions::Background(ImageRenderer::defaultBGPattern);
        o.allowPixelBuffers = ImageRenderer::getDefaultAllowPBOs();
        o.appleClientStorage = ImageRenderer::getUseAppleClientStorage();
        o.threadedUpload = ImageRenderer::getUseThreadedUpload();
        o.noGPUCache = ImageRenderer::noGPUCache();
        o.passDebug = ImageRenderer::passDebug();
        o.intermediateDebug = ImageRenderer::outputIntermediateDebug();
        o.intermediateLogging = ImageRenderer::intermediateLogging();
        o.debugGpu = ImageRenderer::debugGpu();
        return o;
    }

    void GLCompatRendererBackend::setOptions(const RendererOptions& o)
    {
        ImageRenderer::setDefaultBGPattern(ImageRenderer::BGPattern(o.defaultBackground));
        ImageRenderer::defaultAllowPBOs(o.allowPixelBuffers);
        ImageRenderer::setUseAppleClientStorage(o.appleClientStorage);
        ImageRenderer::setUseThreadedUpload(o.threadedUpload);
        ImageRenderer::setNoGPUCache(o.noGPUCache);
        ImageRenderer::setPassDebug(o.passDebug);
        ImageRenderer::setOutputIntermediateDebug(o.intermediateDebug);
        ImageRenderer::setIntermediateLogging(o.intermediateLogging);
        ImageRenderer::debugGpu(o.debugGpu);
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
