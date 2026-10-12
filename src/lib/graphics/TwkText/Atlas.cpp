//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#include <TwkText/Atlas.h>
#include <algorithm>
#include <cstring>

namespace TwkText
{
    namespace
    {
        //
        //  Glyphs are packed with this many empty pixels between them, so
        //  linear filtering never reads a neighbour's pixels.
        //

        const int gap = 1;

        const int initialPageHeight = 128;
    } // namespace

    Atlas& Atlas::instance()
    {
        static Atlas atlas;
        return atlas;
    }

    void Atlas::grow(size_t i, int height)
    {
        Page& p = m_pages[i];
        if (height <= p.height)
            return;

        p.pixels.resize(size_t(pageWidth) * height * 4, 0);
        p.height = height;
        p.version++;
    }

    bool Atlas::place(size_t i, int w, int h, int& x, int& y)
    {
        PagePacking& packing = m_packing[i];

        //
        //  Shelf packing: the first shelf that is tall enough and has room,
        //  otherwise a new shelf on top. Glyphs at one size are similar in
        //  height, so little space is lost.
        //

        for (Shelf& s : packing.shelves)
        {
            if (h <= s.height && s.x + w <= pageWidth)
            {
                x = s.x;
                y = s.y;
                s.x += w + gap;
                return true;
            }
        }

        if (packing.top + h > pageMaxHeight)
            return false;

        Shelf s;
        s.y = packing.top;
        s.height = h;
        s.x = w + gap;
        packing.shelves.push_back(s);
        packing.top += h + gap;

        x = 0;
        y = s.y;

        int height = std::max(m_pages[i].height, initialPageHeight);
        while (height < packing.top && height < pageMaxHeight)
            height *= 2;
        grow(i, std::min(height, pageMaxHeight));
        return true;
    }

    void Atlas::add(int w, int h, const unsigned char* pixels, int& page, int& x, int& y)
    {
        w = std::min(w, pageWidth);
        h = std::min(h, pageMaxHeight);

        size_t i = 0;
        for (; i < m_pages.size(); i++)
        {
            if (place(i, w, h, x, y))
                break;
        }

        if (i == m_pages.size())
        {
            m_pages.push_back(Page());
            m_packing.push_back(PagePacking());
            place(i, w, h, x, y);
        }

        Page& p = m_pages[i];
        for (int row = 0; row < h; row++)
        {
            memcpy(&p.pixels[(size_t(y + row) * pageWidth + x) * 4], pixels + size_t(row) * w * 4, size_t(w) * 4);
        }

        p.version++;
        page = int(i);
    }

} // namespace TwkText
