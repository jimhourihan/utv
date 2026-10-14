#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# HarfBuzz (text shaping) for TwkText. Optional: without it TwkText maps characters to glyphs directly. Windows uses OpenUTVDeps' (26.6 and later), macOS and
# Linux the system's (Homebrew, or libharfbuzz-dev).
#
# Defines the target harfbuzz::harfbuzz when HarfBuzz is found.
#

IF(RV_TARGET_WINDOWS)
  # OpenUTVDeps' harfbuzzConfig.cmake is vcpkg's stand-in, which builds its paths from VCPKG_INSTALLED_DIR and only works under the vcpkg toolchain.
  FIND_PATH(
    RV_HARFBUZZ_INCLUDE_DIR hb.h
    PATH_SUFFIXES harfbuzz
  )
  FIND_LIBRARY(RV_HARFBUZZ_LIBRARY harfbuzz)
  IF(RV_HARFBUZZ_INCLUDE_DIR
     AND RV_HARFBUZZ_LIBRARY
  )
    ADD_LIBRARY(harfbuzz::harfbuzz UNKNOWN IMPORTED GLOBAL)
    SET_TARGET_PROPERTIES(
      harfbuzz::harfbuzz
      PROPERTIES IMPORTED_LOCATION "${RV_HARFBUZZ_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${RV_HARFBUZZ_INCLUDE_DIR}"
    )
  ENDIF()
ELSE()
  FIND_PACKAGE(harfbuzz CONFIG QUIET)
ENDIF()

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
