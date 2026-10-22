//
// Copyright (c) 2026 Seth Rosenthal.
// All rights reserved.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <IPCore/HoverPreviewOverlay.h>
#include <algorithm>

namespace IPCore
{

    HoverPreviewOverlay::HoverPreviewOverlay()
        : m_tipX(0)
        , m_tipY(0)
        , m_imageWidth(0)
        , m_imageHeight(0)
        , m_opacity(0)
    {
    }

    HoverPreviewOverlay::~HoverPreviewOverlay() {}

    void HoverPreviewOverlay::setPosition(float tipX, float tipY)
    {
        m_tipX = tipX;
        m_tipY = tipY;
    }

    void HoverPreviewOverlay::setImageSize(float width, float height)
    {
        m_imageWidth = width;
        m_imageHeight = height;
    }

    void HoverPreviewOverlay::setOpacity(float opacity) { m_opacity = std::min(std::max(opacity, 0.0f), 1.0f); }

} // namespace IPCore
