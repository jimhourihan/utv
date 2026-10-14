#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# TwkText: font loading, text layout and the MTSDF glyph atlas, with no GL. Renderers draw the glyph quads it produces (TwkGLF/GLText.h for OpenGL).
#

INCLUDE(cxx_defaults)

SET(_target
    "TwkText"
)

SET(_sources
    Atlas.cpp Font.cpp defaultFont.cpp
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_sources}
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC TwkMath
  PRIVATE msdfgen freetype stl_ext TwkUtil
)

IF(TARGET harfbuzz::harfbuzz)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE harfbuzz::harfbuzz
  )
  TARGET_COMPILE_DEFINITIONS(
    ${_target}
    PRIVATE TWK_TEXT_HAVE_HARFBUZZ
  )
ENDIF()

IF(TARGET Fontconfig::Fontconfig)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE Fontconfig::Fontconfig
  )
  TARGET_COMPILE_DEFINITIONS(
    ${_target}
    PRIVATE TWK_TEXT_HAVE_FONTCONFIG
  )
ENDIF()

IF(RV_TARGET_DARWIN)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PRIVATE "-framework CoreText" "-framework CoreFoundation"
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
