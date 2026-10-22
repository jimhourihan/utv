//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
//
//  DRAFT (MODULARIZE_IPCORE.md, step A2). Not included or built anywhere
//  yet. It is the interface Session and the apps would use instead of
//  ImageRenderer, so that the renderer can be a library loaded at run time
//  (GL first). The operations come from the A1 inventory in
//  MODULARIZE_IPCORE.md. Nothing here names a graphics API.
//
#ifndef __IPCore__Renderer__h__
#define __IPCore__Renderer__h__

#include <IPCore/IPImage.h>
#include <TwkApp/VideoDevice.h>
#include <TwkFB/FrameBuffer.h>
#include <TwkMath/Box.h>
#include <TwkUtil/Timer.h>
#include <cstdint>
#include <iosfwd>
#include <set>
#include <string>
#include <vector>

namespace IPCore
{
    class IPNode;

    //
    //  A texture owned by the backend, as seen by scripts
    //  (renderedImages(), imagesAtPixel(), audioTextureID()). Today these
    //  are GL texture names. With a non-GL backend only the MuGL emulation
    //  (MODULARIZE_IPCORE.md, Plan C) can use them.
    //

    struct RenderTexture
    {
        uint64_t id = 0;

        bool valid() const { return id != 0; }
    };

    //
    //  Settings a backend is created with. These replace ImageRenderer's
    //  static setters, which Options and RvPreferences call before or
    //  between renders.
    //

    struct RendererOptions
    {
        enum class Background
        {
            Solid0,
            Solid18,
            Solid50,
            Solid100,
            Checker,
            CrossHatch
        };

        Background defaultBackground = Background::Checker;
        bool allowPixelBuffers = true;  // PBOs on GL
        bool appleClientStorage = true; // macOS upload fast path on GL
        bool threadedUpload = false;
        bool noGPUCache = false;
        bool passDebug = false;
        bool intermediateDebug = false;
        bool intermediateLogging = false;
        bool debugGpu = false; // "-debug gpu"
    };

    //
    //  What a backend can do on the current device. These replace
    //  ImageRenderer's static query*() functions. They are valid once the
    //  backend has a device (for GL: a current context).
    //

    struct RendererCapabilities
    {
        std::string api;     // "OpenGL", "Vulkan", "Metal"
        std::string version; // e.g. "4.1 Metal - 89.3"
        std::string vendor;
        std::string device;

        bool floatFormats = false;
        bool clientStorage = false;
        bool threadedUpload = false;
        std::set<TwkFB::FrameBuffer::DataType> acceptableTypes;

        //
        //  Name / value pairs for the "-debug gpu" report and for the
        //  session node's properties (today queryGLIntoContainer()).
        //

        std::vector<std::pair<std::string, std::string>> details;
    };

    //
    //  The renderer. One per Session, made by a RendererBackend.
    //

    class Renderer
    {
    public:
        using VideoDevice = TwkApp::VideoDevice;
        using DisplayMode = VideoDevice::DisplayMode;
        using Margins = VideoDevice::Margins;
        using Matrix = IPImage::Matrix;
        using Box2f = TwkMath::Box2f;
        using Background = RendererOptions::Background;

        //
        //  Script drawing (render events) and audio for output devices.
        //  Same as ImageRenderer::AuxRender and AuxAudio. With a non-GL
        //  backend AuxRender::render() runs against the MuGL emulation.
        //

        class AuxRender
        {
        public:
            virtual ~AuxRender() = default;
            virtual void render(DisplayMode, bool leftEye, bool rightEye, bool forController, bool forOutput) const = 0;
        };

        class AuxAudio
        {
        public:
            virtual ~AuxAudio() = default;
            virtual void* audioForFrame(int frame, size_t seqindex, size_t& n) const = 0;
            virtual bool isAvailable() const = 0;
        };

        //
        //  What was drawn, for the UI and scripts (RenderQuery,
        //  imagesAtPixel(), renderedImages()). ImageRenderer::RenderedImage
        //  with the GL texture name replaced by a RenderTexture.
        //

        struct RenderedImage
        {
            std::string source;
            IPImage::TagMap tagMap;
            int index = 0;
            Box2f imageBox;
            Box2f stencilBox;
            Matrix modelMatrix;
            Matrix globalMatrix;
            Matrix projectionMatrix;
            Matrix textureMatrix;
            Matrix orientationMatrix;
            Matrix placementMatrix;
            int width = 0;
            int height = 0;
            int uncropWidth = 0;
            int uncropHeight = 0;
            int uncropX = 0;
            int uncropY = 0;
            float pixelAspect = 1;
            float initPixelAspect = 1;
            int bitDepth = 0;
            bool floatingPoint = false;
            int numChannels = 0;
            bool planar = false;
            const IPNode* node = nullptr;
            const VideoDevice* device = nullptr;
            size_t serialNum = 0;
            size_t imageNum = 0;
            RenderTexture texture;
            bool isVirtual = false;
            bool touched = false;
            bool render = false;
        };

        using RenderedImages = std::vector<RenderedImage>;

        struct Profiling
        {
            double uploadPlaneTime = 0;
            double fenceWaitTime = 0;
        };

        enum class Cache
        {
            Intermediates, // ImageRenderer::flushImageFBOs()
            Programs,      // ImageRenderer::flushProgramCache()
            All
        };

        virtual ~Renderer() = default;

        //
        //  Devices. The control device is the viewer (or utvio's offscreen
        //  device); the output device is a second display or an SDI / NDI
        //  output. The devices come from the app and must be ones this
        //  backend supports (MODULARIZE_IPCORE.md, A3).
        //

        virtual void setControlDevice(const VideoDevice*) = 0;
        virtual void setOutputDevice(const VideoDevice*) = 0;
        virtual const VideoDevice* controlDevice() const = 0;
        virtual const VideoDevice* currentDevice() const = 0;

        //
        //  Rendering. prefetch() uploads ahead of time (today
        //  prepareTextureDescriptionsForUpload() + prefetch()). With
        //  threaded upload the backend does the upload / draw handshake
        //  (waitToDraw()) inside render().
        //

        virtual void prefetch(const IPImage*) = 0;
        virtual void render(int frame, IPImage* root, const AuxRender* aux = nullptr, const AuxAudio* auxAudio = nullptr,
                            IPImage* uploadRoot = nullptr) = 0;
        virtual bool threadedUpload() const = 0;
        virtual void releaseUploads() = 0; // freeUploadedTextures()

        //
        //  Read back what was last rendered to a device, without its
        //  margins, as RGBA8 (exportCurrentFrame() does its own
        //  glReadPixels today). The caller owns the result.
        //

        virtual TwkFB::FrameBuffer* readBack(const VideoDevice*, const Margins&) = 0;

        //
        //  Results of the last render
        //

        virtual const RenderedImages& renderedImages() const = 0;
        virtual Box2f viewport() const = 0;

        //
        //  Caches and state
        //

        virtual void flush(Cache) = 0;
        virtual void clearState() = 0;
        virtual void unlinkNode(IPNode*) = 0;

        //
        //  Display settings
        //

        virtual void setBackground(Background) = 0;
        virtual Background background() const = 0;
        virtual void setFiltering(int) = 0; // GL_NEAREST / GL_LINEAR today
        virtual int filtering() const = 0;

        //
        //  Profiling
        //

        virtual void beginProfiling(TwkUtil::Timer*) = 0;
        virtual Profiling profiling() const = 0;
    };

    //
    //  A backend: one per renderer library. A library exports
    //
    //      extern "C" IPCore::RendererBackend* UTVRendererBackend();
    //
    //  and is loaded with dlopen() / LoadLibrary(), as the image format
    //  plugins are. The GL backend can also be linked in directly.
    //

    class RendererBackend
    {
    public:
        virtual ~RendererBackend() = default;

        virtual std::string name() const = 0; // "GL", "Vulkan", ...

        //
        //  Valid once a device of this backend exists (for GL: a current
        //  context). Replaces queryGL(), queryGLFinished() and the static
        //  query*() functions.
        //

        virtual const RendererCapabilities& capabilities() const = 0;
        virtual void printInfo(std::ostream&) const = 0; // "-debug gpu"

        virtual Renderer* newRenderer(const RendererOptions&) = 0;
    };

    //
    //  Loading. Session::setRendererType() would call
    //  findRendererBackend() instead of making an ImageRenderer.
    //

    RendererBackend* findRendererBackend(const std::string& name);
    std::vector<std::string> availableRendererBackends();

} // namespace IPCore

#endif // __IPCore__Renderer__h__
