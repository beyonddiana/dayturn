# -*- cmake -*-

# FMODEX can be set when launching the make using the argument -DFMOD:BOOL=ON
# No longer used by default, see FMODSTRUDIO.
# Open source devs should use the -DFMODEX:BOOL=ON then if they want to build with FMODEX, whether
# they are using USESYSTEMLIBS or not.

FIND_PATH(FMODEX_INCLUDE_DIR fmod.h PATH_SUFFIXES fmod)

SET(FMODEX_NAMES ${FMODEX_NAMES} fmodex fmodvc fmodexL_vc)
FIND_LIBRARY(FMODEX_LIBRARY
  NAMES ${FMODEX_NAMES}
  PATH_SUFFIXES fmodex
  )

IF (FMODEX_SDK_DIR OR WINDOWS)
    if(WINDOWS)
        set(FMODEX_SDK_DIR "$ENV{PROGRAMFILES}/FMOD SoundSystem/FMOD Programmers API Windows" CACHE PATH "Path to FMODEX")
        STRING(REGEX REPLACE "\\\\" "/" FMODEX_SDK_DIR ${FMODEX_SDK_DIR}) 
    endif(WINDOWS)
    find_library(FMODEX_LIBRARY
             fmodex_vc fmodexL_vc 
             PATHS
             ${FMODEX_SDK_DIR}/api/lib
             ${FMODEX_SDK_DIR}/api
             ${FMODEX_SDK_DIR}
             )
    find_path(FMODEX_INCLUDE_DIR fmod.h
        ${FMODEX_SDK_DIR}/api/inc
        ${FMODEX_SDK_DIR}/api
        ${FMODEX_SDK_DIR}
      )
    find_path(FMODEX_INCLUDE_DIR fmod.h
        ${FMODEX_SDK_DIR}/api/inc
        ${FMODEX_SDK_DIR}/api
        ${FMODEX_SDK_DIR}
      )
    IF (FMODEX_LIBRARY AND FMODEX_INCLUDE_DIR)
      SET(FMODEX_LIBRARIES ${FMODEX_LIBRARY})
      SET(FMODEX_FOUND "YES")
    endif (FMODEX_LIBRARY AND FMODEX_INCLUDE_DIR)
ENDIF (FMODEX_SDK_DIR OR WINDOWS)

IF (FMODEX_FOUND)
  IF (NOT FMODEX_FIND_QUIETLY)
    MESSAGE(STATUS "Found FMODEX: ${FMODEX_LIBRARIES}")
  ENDIF (NOT FMODEX_FIND_QUIETLY)
ELSE (FMODEX_FOUND)
  IF (FMODEX_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find FMODEX library")
  ENDIF (FMODEX_FIND_REQUIRED)
ENDIF (FMODEX_FOUND)

# Deprecated declarations.
SET (NATIVE_FMODEX_INCLUDE_PATH ${FMODEX_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_FMODEX_LIB_PATH ${FMODEX_LIBRARY} PATH)

MARK_AS_ADVANCED(
  FMODEX_LIBRARY
  FMODEX_INCLUDE_DIR
  )
