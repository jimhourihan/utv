#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# The GL renderer backend for IPCore (MODULARIZE_IPCORE.md): ImageRenderer and the GL code it uses. IPCore and the node libraries do not depend on it; the apps
# link it and call registerGLCompatRendererBackend().
#

INCLUDE(cxx_defaults)
INCLUDE(rv_quote_file)

SET(_target
    "IPRendererGLCompat"
)

SET(_sources
    GLCompatRendererBackend.cpp
    GLShaderCompiler.cpp
    ImageRenderer.cpp
    PaintCommandGL.cpp
    ImageFBO.cpp
    ShaderProgram.cpp
    HoverPreviewOverlayGL.cpp
)

# Create CL Kernels
SET(_cl_kernels
    Histogram48k Histogram32k Histogram16k
)
QUOTE_FILES(_cooked_cl_kernels INPUT_LIST ${_cl_kernels} SUFFIX "cl")
LIST(APPEND _sources ${_cooked_cl_kernels})

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
  PUBLIC IPCore TwkGLF
  PRIVATE TwkGLText TwkFB TwkMath TwkMovie TwkUtil stl_ext
)

IF(RV_TARGET_WINDOWS)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC glew
  )
  TARGET_COMPILE_OPTIONS(
    ${_target}
    PRIVATE "-wd4355"
  )
ELSEIF(RV_TARGET_DARWIN)
  FIND_PACKAGE(OpenCL REQUIRED)
  TARGET_LINK_LIBRARIES(
    ${_target}
    PUBLIC OpenCL::OpenCL
  )
ENDIF()

RV_STAGE(TYPE "LIBRARY" TARGET ${_target})
