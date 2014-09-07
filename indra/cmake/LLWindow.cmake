# -*- cmake -*-

include(Variables)
include(GLEXT)
include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindSDL)

  # This should be done by FindSDL.  Sigh.
  mark_as_advanced(
      SDLMAIN_LIBRARY
      SDL_INCLUDE_DIR
      SDL_LIBRARY
      )
else (USESYSTEMLIBS)
  if (LINUX)
    use_prebuilt_binary(SDL)#kokuafixme was:SDL-noartwork
    set (SDL_FOUND TRUE)
    set (SDL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include)
    set (SDL_LIBRARY SDL)
    set (SDL_LIBRARY SDL directfb fusion direct)
    #if (NOT ${ARCH} STREQUAL "x86_64")
    #  list(APPEND SDL_LIBRARY directfb fusion direct)
    #endif (NOT ${ARCH} STREQUAL "x86_64")
  endif (LINUX)	
endif (USESYSTEMLIBS)

if (SDL_FOUND)
  include_directories(${SDL_INCLUDE_DIR})
endif (SDL_FOUND)

set(LLWINDOW_INCLUDE_DIRS
    ${GLEXT_INCLUDE_DIR}
    ${LIBS_OPEN_DIR}/llwindow
    )

if (BUILD_HEADLESS)
  set(LLWINDOW_HEADLESS_LIBRARIES
      llwindowheadless
      )
endif (BUILD_HEADLESS)

  set(LLWINDOW_LIBRARIES
      llwindow
      )
