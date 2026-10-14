#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# Fontconfig, for TwkText's system font fallback on Linux (characters the annotation font doesn't have). Optional: without it RV_TEXT_FALLBACK_FONTS is the only
# fallback. macOS uses CoreText instead. Homebrew's or the system's (libfontconfig-dev, fontconfig-devel), through CMake's FindFontconfig.
#
# Defines the target Fontconfig::Fontconfig when Fontconfig is found.
#

IF(RV_TARGET_LINUX)
  FIND_PACKAGE(Fontconfig QUIET)

  IF(TARGET Fontconfig::Fontconfig)
    SET_PROPERTY(
      TARGET Fontconfig::Fontconfig
      PROPERTY IMPORTED_GLOBAL TRUE
    )
    MESSAGE(STATUS "Fontconfig found: system font fallback enabled")
  ELSE()
    MESSAGE(STATUS "Fontconfig not found: no system font fallback")
  ENDIF()
ENDIF()
