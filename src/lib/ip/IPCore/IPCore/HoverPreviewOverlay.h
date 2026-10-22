//
// Copyright (c) 2026 Seth Rosenthal.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __IPCore__HoverPreviewOverlay__h__
#define __IPCore__HoverPreviewOverlay__h__
#include <string>

namespace TwkApp
{
    class VideoDevice;
}

namespace IPCore
{
    class Renderer;

    //
    //  class HoverPreviewOverlay
    //
    //  The timeline hover preview: the "hoverPreview" output texture (see
    //  IPGraph::hoverPreviewNode()) and a label on a translucent panel
    //  with an arrow pointing down at the frame. Session's
    //  RendererBackend makes it (HoverPreviewOverlayGL for GL), and
    //  Session::renderHoverPreviewOverlay() calls render() after the UI
    //  has drawn, with the control device's framebuffer bound, so the
    //  overlay is part of the rendered frame on every presentation path.
    //
    //  The UI sets the state below. Positions and sizes are in framebuffer
    //  pixels with the origin at the bottom left. The overlay draws only
    //  while its opacity is above zero and the renderer has produced the
    //  preview texture this frame. Main thread only.
    //

    class HoverPreviewOverlay
    {
    public:
        HoverPreviewOverlay();
        virtual ~HoverPreviewOverlay();

        //
        //  The tip of the arrow, which points at the frame. The panel sits
        //  above it, moved sideways if needed to stay inside the
        //  framebuffer; the arrow keeps pointing at the tip.
        //

        void setPosition(float tipX, float tipY);

        //
        //  The size the preview image is drawn at, in framebuffer pixels.
        //  The panel adds padding and a row for the label.
        //

        void setImageSize(float width, float height);

        void setLabel(const std::string& label) { m_label = label; }

        //
        //  0 hides the overlay, 1 is fully opaque. Values in between are
        //  used to fade it in and out.
        //

        void setOpacity(float opacity);

        float opacity() const { return m_opacity; }

        virtual void render(const Renderer*, const TwkApp::VideoDevice*) const = 0;

    protected:
        float m_tipX;
        float m_tipY;
        float m_imageWidth;
        float m_imageHeight;
        float m_opacity;
        std::string m_label;
    };

} // namespace IPCore

#endif // __IPCore__HoverPreviewOverlay__h__
