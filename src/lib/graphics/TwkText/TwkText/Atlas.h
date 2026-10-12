//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkText__Atlas__h__
#define __TwkText__Atlas__h__

#include <cstddef>
#include <mutex>
#include <vector>

namespace TwkText
{

    //
    //  Atlas
    //
    //  The glyph images used by GlyphQuads, shared by every font in the
    //  process. Glyphs are generated the first time a layout needs them
    //  and packed into pages: RGBA8 images whose RGB channels are a
    //  multi-channel signed distance field and whose alpha is a true signed
    //  distance (MTSDF). A distance of 0 (the glyph outline) is stored as
    //  0.5, and pixelRange atlas pixels span the stored range [0, 1].
    //
    //  Pages are pageWidth wide and grow in height (doubling, up to
    //  pageMaxHeight) as glyphs are added, so a renderer uploads small
    //  textures until more glyphs are needed. version changes whenever a
    //  page's pixels or height change.
    //
    //  Read pages while holding lock(); layout adds glyphs under the same
    //  lock.
    //

    class Atlas
    {
    public:
        static constexpr int pageWidth = 2048;
        static constexpr int pageMaxHeight = 2048;

        //
        //  Glyphs are generated at emPixels atlas pixels per em, with a
        //  distance range of pixelRange atlas pixels.
        //

        static constexpr int emPixels = 48;
        static constexpr float pixelRange = 4.0f;

        struct Page
        {
            int height = 0;
            unsigned int version = 0;
            std::vector<unsigned char> pixels; // pageWidth * height * 4, bottom row first
        };

        static Atlas& instance();

        std::mutex& lock() { return m_lock; }

        size_t pageCount() const { return m_pages.size(); }

        const Page& page(size_t i) const { return m_pages[i]; }

        //
        //  Internal: add a glyph image of w x h pixels (RGBA8, bottom row
        //  first). Sets page and the image's bottom-left atlas pixel.
        //  Requires lock().
        //

        void add(int w, int h, const unsigned char* pixels, int& page, int& x, int& y);

    private:
        Atlas() {}

        struct Shelf
        {
            int y;
            int height;
            int x;
        };

        struct PagePacking
        {
            std::vector<Shelf> shelves;
            int top = 0;
        };

        bool place(size_t i, int w, int h, int& x, int& y);
        void grow(size_t i, int height);

        std::mutex m_lock;
        std::vector<Page> m_pages;
        std::vector<PagePacking> m_packing;
    };

} // namespace TwkText

#endif // __TwkText__Atlas__h__
