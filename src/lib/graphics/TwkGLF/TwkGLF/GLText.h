//
//  Copyright (C) 2026  Contributors to the OpenUTV Project.
//
//  SPDX-License-Identifier: Apache-2.0
//
#ifndef __TwkGLF__GLText__h__
#define __TwkGLF__GLText__h__

#include <TwkGLF/GLVBO.h>
#include <TwkMath/Color.h>
#include <TwkMath/Mat44.h>
#include <TwkText/Font.h>

namespace TwkGLF
{

    //
    //  Draw glyph quads laid out by TwkText::Font into the current
    //  framebuffer and viewport, with the current blend state. Positions are
    //  transformed like GLPipeline's: projection * modelview * position.
    //  The color is not premultiplied; the glyph coverage scales its alpha
    //  (as TwkGLText did), so blend with GL_SRC_ALPHA,
    //  GL_ONE_MINUS_SRC_ALPHA.
    //
    //  The shader program and atlas textures are kept per GL context and
    //  uploaded when the atlas changes. Works in compatibility and core
    //  profile contexts. The current program and texture binding are
    //  restored afterwards.
    //

    void drawText(const TwkText::GlyphQuads& quads, const TwkMath::Mat44f& projection, const TwkMath::Mat44f& modelview,
                  const TwkMath::Col4f& color, GLVBO::GLVBOVector& vbos);

    //
    //  True when RV_TEXT_RENDERER=ftgl: the renderer's text (annotations,
    //  the hover preview label) is drawn with TwkGLText (FTGL) as before,
    //  instead of TwkText and drawText(). For comparisons; to be removed
    //  with TwkGLText.
    //

    bool useFTGLText();

} // namespace TwkGLF

#endif // __TwkGLF__GLText__h__
