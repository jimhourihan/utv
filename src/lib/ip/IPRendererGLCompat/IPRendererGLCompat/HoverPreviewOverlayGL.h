//
// Copyright (c) 2026 Seth Rosenthal.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPRendererGLCompat__HoverPreviewOverlayGL__h__
#define __IPRendererGLCompat__HoverPreviewOverlayGL__h__
#include <IPCore/HoverPreviewOverlay.h>

namespace IPCore
{
    //
    //  class HoverPreviewOverlayGL
    //
    //  The GL HoverPreviewOverlay: draws the preview texture and a label
    //  on a translucent panel with rounded corners, a soft shadow and an
    //  arrow pointing down at the frame.
    //

    class HoverPreviewOverlayGL : public HoverPreviewOverlay
    {
    public:
        void render(const Renderer*, const TwkApp::VideoDevice*) const override;
    };

} // namespace IPCore

#endif // __IPRendererGLCompat__HoverPreviewOverlayGL__h__
