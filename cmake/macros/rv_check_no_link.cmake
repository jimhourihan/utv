#
# Copyright (C) 2026  Contributors to the OpenUTV Project.
#
# SPDX-License-Identifier: Apache-2.0
#
# RV_CHECK_NO_LINK(TARGETS <target>... FORBIDDEN <library>... REASON <text>)
#
# Fails the configuration if one of TARGETS links one of FORBIDDEN, directly or through the libraries it links (PUBLIC, PRIVATE and INTERFACE, followed
# recursively). The check runs at the end of the configuration, when every target exists. Used to keep GL out of IPCore (MODULARIZE_IPCORE.md).
#

FUNCTION(_rv_check_no_link_walk _name forbidden chain)
  GET_PROPERTY(
    _visited GLOBAL
    PROPERTY _RV_CHECK_NO_LINK_VISITED
  )
  IF(_name IN_LIST _visited)
    RETURN()
  ENDIF()
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY _RV_CHECK_NO_LINK_VISITED ${_name}
  )

  GET_TARGET_PROPERTY(_aliased ${_name} ALIASED_TARGET)
  IF(_aliased)
    SET(_name
        ${_aliased}
    )
  ENDIF()

  SET(_libs)
  GET_TARGET_PROPERTY(_type ${_name} TYPE)
  IF(NOT _type STREQUAL "INTERFACE_LIBRARY")
    GET_TARGET_PROPERTY(_link ${_name} LINK_LIBRARIES)
    IF(_link)
      LIST(APPEND _libs ${_link})
    ENDIF()
  ENDIF()
  GET_TARGET_PROPERTY(_interface ${_name} INTERFACE_LINK_LIBRARIES)
  IF(_interface)
    LIST(APPEND _libs ${_interface})
  ENDIF()

  FOREACH(
    _lib
    ${_libs}
  )
    STRING(
      REGEX
      REPLACE "^\\$<LINK_ONLY:(.*)>$" "\\1" _lib "${_lib}"
    )
    IF(_lib MATCHES "\\$<")
      CONTINUE()
    ENDIF()

    IF(_lib IN_LIST forbidden)
      SET_PROPERTY(
        GLOBAL APPEND
        PROPERTY _RV_CHECK_NO_LINK_FOUND "${chain} -> ${_lib}"
      )
    ELSEIF(TARGET ${_lib})
      _RV_CHECK_NO_LINK_WALK(${_lib} "${forbidden}" "${chain} -> ${_lib}")
    ENDIF()
  ENDFOREACH()
ENDFUNCTION()

FUNCTION(_rv_check_no_link_deferred)
  GET_PROPERTY(
    _checks GLOBAL
    PROPERTY _RV_CHECK_NO_LINK_CHECKS
  )
  FOREACH(
    _check
    ${_checks}
  )
    GET_PROPERTY(
      _targets GLOBAL
      PROPERTY _RV_CHECK_NO_LINK_${_check}_TARGETS
    )
    GET_PROPERTY(
      _forbidden GLOBAL
      PROPERTY _RV_CHECK_NO_LINK_${_check}_FORBIDDEN
    )
    GET_PROPERTY(
      _reason GLOBAL
      PROPERTY _RV_CHECK_NO_LINK_${_check}_REASON
    )

    FOREACH(
      _target
      ${_targets}
    )
      SET_PROPERTY(
        GLOBAL
        PROPERTY _RV_CHECK_NO_LINK_VISITED ""
      )
      SET_PROPERTY(
        GLOBAL
        PROPERTY _RV_CHECK_NO_LINK_FOUND ""
      )
      _RV_CHECK_NO_LINK_WALK(${_target} "${_forbidden}" "${_target}")
      GET_PROPERTY(
        _found GLOBAL
        PROPERTY _RV_CHECK_NO_LINK_FOUND
      )
      IF(_found)
        LIST(REMOVE_DUPLICATES _found)
        LIST(JOIN _found "\n  " _found)
        MESSAGE(FATAL_ERROR "${_target} must not link a graphics API library (${_reason}). It does:\n  ${_found}")
      ENDIF()
    ENDFOREACH()
  ENDFOREACH()
ENDFUNCTION()

FUNCTION(rv_check_no_link)
  CMAKE_PARSE_ARGUMENTS(arg "" "REASON" "TARGETS;FORBIDDEN" ${ARGN})

  GET_PROPERTY(
    _checks GLOBAL
    PROPERTY _RV_CHECK_NO_LINK_CHECKS
  )
  LIST(LENGTH _checks _n)
  SET_PROPERTY(
    GLOBAL APPEND
    PROPERTY _RV_CHECK_NO_LINK_CHECKS ${_n}
  )
  SET_PROPERTY(
    GLOBAL
    PROPERTY _RV_CHECK_NO_LINK_${_n}_TARGETS ${arg_TARGETS}
  )
  SET_PROPERTY(
    GLOBAL
    PROPERTY _RV_CHECK_NO_LINK_${_n}_FORBIDDEN ${arg_FORBIDDEN}
  )
  SET_PROPERTY(
    GLOBAL
    PROPERTY _RV_CHECK_NO_LINK_${_n}_REASON "${arg_REASON}"
  )

  IF(_n EQUAL 0)
    CMAKE_LANGUAGE(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _rv_check_no_link_deferred)
  ENDIF()
ENDFUNCTION()
