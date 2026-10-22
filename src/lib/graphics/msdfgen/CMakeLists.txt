#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# msdfgen (https://github.com/Chlumsky/msdfgen), MIT licence. Vendored: the core library and FreeType font import (ext/import-font) only. See README.md
# (including why it is here and not in src/pub).
#

INCLUDE(cxx_defaults)

SET(_target
    "msdfgen"
)

SET(_core_sources
    contour-combiners.cpp
    Contour.cpp
    convergent-curve-ordering.cpp
    DistanceMapping.cpp
    edge-coloring.cpp
    edge-segments.cpp
    edge-selectors.cpp
    EdgeHolder.cpp
    equation-solver.cpp
    export-svg.cpp
    msdf-error-correction.cpp
    MSDFErrorCorrection.cpp
    msdfgen.cpp
    Projection.cpp
    rasterization.cpp
    render-sdf.cpp
    save-bmp.cpp
    save-fl32.cpp
    save-rgba.cpp
    save-tiff.cpp
    Scanline.cpp
    sdf-error-estimation.cpp
    shape-description.cpp
    Shape.cpp
)

LIST(
  TRANSFORM _core_sources
  PREPEND msdfgen/core/
)

ADD_LIBRARY(
  ${_target} STATIC
  ${_core_sources} msdfgen/ext/import-font.cpp
)

TARGET_INCLUDE_DIRECTORIES(
  ${_target}
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Upstream's build generates msdfgen-config.h; core/base.h skips it when MSDFGEN_PUBLIC is defined.
TARGET_COMPILE_DEFINITIONS(
  ${_target}
  PUBLIC MSDFGEN_PUBLIC= MSDFGEN_EXT_PUBLIC=
)

TARGET_LINK_LIBRARIES(
  ${_target}
  PUBLIC freetype
)

IF(RV_TARGET_LINUX)
  # The system FreeType keeps ft2build.h in the freetype2 subdirectory.
  TARGET_INCLUDE_DIRECTORIES(
    ${_target}
    PUBLIC ${FREETYPE_INCLUDE_DIR_ft2build}
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
