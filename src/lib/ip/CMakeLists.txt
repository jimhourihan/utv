#
# Copyright (C) 2022  Autodesk, Inc. All Rights Reserved.
#
# SPDX-License-Identifier: Apache-2.0
#
ADD_SUBDIRECTORY(IPCore)
ADD_SUBDIRECTORY(IPBaseNodes)
ADD_SUBDIRECTORY(IPRendererGLCompat)
ADD_SUBDIRECTORY(IPMu)
ADD_SUBDIRECTORY(OCIONodes)
ADD_SUBDIRECTORY(ICCNodes)
ADD_SUBDIRECTORY(IPCoreCommands)

#
# IPCore and the node libraries are API-neutral: GL is in IPRendererGLCompat (MODULARIZE_IPCORE.md). The Qt targets are imported in subdirectories, so the check
# cannot follow them from here to the GL library they link: they are named instead.
#
INCLUDE(rv_check_no_link)
RV_CHECK_NO_LINK(
  TARGETS
  IPCore
  IPBaseNodes
  IPMu
  FORBIDDEN
  IPRendererGLCompat
  TwkGLF
  TwkGLText
  TwkGLFFBO
  TwkGLFCoreGraphics
  MuGL
  glew
  GLEW::GLEW
  GL
  EGL
  OpenGL::GL
  OpenGL::OpenGL
  OpenGL::GLX
  OpenGL::EGL
  opengl32
  Qt6::Gui
  Qt6::OpenGL
  Qt6::OpenGLWidgets
  WrapOpenGL::WrapOpenGL
  REASON
  "the renderer backends hold the graphics API code"
)
