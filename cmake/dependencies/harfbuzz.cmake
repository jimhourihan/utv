#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# HarfBuzz (text shaping) for TwkText. Optional for now: OpenUTVDeps doesn't ship it yet, so Windows builds without it and TwkText maps characters to glyphs
# directly. macOS and Linux use the system's (Homebrew, or libharfbuzz-dev).
#
# Defines the target harfbuzz::harfbuzz when HarfBuzz is found.
#

FIND_PACKAGE(harfbuzz CONFIG QUIET)

IF(NOT TARGET harfbuzz::harfbuzz)
  FIND_PACKAGE(PkgConfig QUIET)
  IF(PKG_CONFIG_FOUND)
    PKG_CHECK_MODULES(HARFBUZZ QUIET IMPORTED_TARGET harfbuzz)
    IF(TARGET PkgConfig::HARFBUZZ)
      ADD_LIBRARY(harfbuzz::harfbuzz ALIAS PkgConfig::HARFBUZZ)
      SET_PROPERTY(
        TARGET PkgConfig::HARFBUZZ
        PROPERTY IMPORTED_GLOBAL TRUE
      )
    ENDIF()
  ENDIF()
ELSE()
  SET_PROPERTY(
    TARGET harfbuzz::harfbuzz
    PROPERTY IMPORTED_GLOBAL TRUE
  )
ENDIF()

IF(TARGET harfbuzz::harfbuzz)
  MESSAGE(STATUS "HarfBuzz found: text shaping enabled")
ELSE()
  MESSAGE(STATUS "HarfBuzz not found: text is laid out without shaping")
ENDIF()
