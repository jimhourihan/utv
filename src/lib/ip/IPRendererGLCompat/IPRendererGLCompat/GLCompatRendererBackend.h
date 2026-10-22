//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPRendererGLCompat__GLCompatRendererBackend__h__
#define __IPRendererGLCompat__GLCompatRendererBackend__h__
#include <IPCore/Renderer.h>

namespace IPCore
{

    //
    //  The GL (compatibility profile, or core with UTV_GL_PROFILE=core)
    //  RendererBackend: ImageRenderer and HoverPreviewOverlayGL. The apps
    //  call registerGLCompatRendererBackend() before making a Session.
    //

    class GLCompatRendererBackend : public RendererBackend
    {
    public:
        std::string name() const override { return "GL"; }

        bool initialized() const override;
        bool initialize(const TwkApp::VideoDevice* control) override;
        void storeInfo(IPNode* sessionNode) const override;

        RendererCapabilities capabilities() const override;
        void printInfo(std::ostream&) const override;

        bool setDebugOption(const std::string& flag) override;

        Renderer* newRenderer(const std::string& type) override;
        HoverPreviewOverlay* newHoverPreviewOverlay() override;
    };

    void registerGLCompatRendererBackend();

} // namespace IPCore

#endif // __IPRendererGLCompat__GLCompatRendererBackend__h__
