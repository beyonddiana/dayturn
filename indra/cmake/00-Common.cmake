# -*- cmake -*-
#
# Compilation options shared by all Dayturn components.

#*****************************************************************************
#   It's important to realize that CMake implicitly concatenates
#   CMAKE_CXX_FLAGS with (e.g.) CMAKE_CXX_FLAGS_RELEASE for Release builds. So
#   set switches in CMAKE_CXX_FLAGS that should affect all builds, but in
#   CMAKE_CXX_FLAGS_RELEASE or CMAKE_CXX_FLAGS_RELWITHDEBINFO for switches
#   that should affect only that build variant.
#
#   Also realize that CMAKE_CXX_FLAGS may already be partially populated on
#   entry to this file.
#*****************************************************************************


if(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
set(${CMAKE_CURRENT_LIST_FILE}_INCLUDED "YES")

include(Variables)

# Portable compilation flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DADDRESS_SIZE=32")

set(CMAKE_CXX_FLAGS_RELEASE
    "-DLL_RELEASE=1 -DLL_RELEASE_FOR_DOWNLOAD=1 -DNDEBUG") 
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
    "-DLL_RELEASE=1 -DNDEBUG -DLL_RELEASE_WITH_DEBUG_INFO=1")


# Configure crash reporting
set(RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in release builds")
set(NON_RELEASE_CRASH_REPORTING OFF CACHE BOOL "Enable use of crash reporting in developer builds")

if(RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DLL_SEND_CRASH_REPORTS=0")
endif()

if(NON_RELEASE_CRASH_REPORTING)
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DLL_SEND_CRASH_REPORTS=0")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLL_SEND_CRASH_REPORTS=0")
endif()  

# Don't bother with MinSizeRel or Debug builds.
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release" CACHE STRING
    "Supported build types." FORCE)
	
set(UNATTENDED OFF CACHE BOOL ”On Windows, turn this ON to not run VSTool.exe after configure.”)


# Platform-specific compilation flags.

if (WINDOWS)
  # Don't build DLLs.
  set(BUILD_SHARED_LIBS OFF)

  # for "backwards compatibility", cmake sneaks in the Zm1000 option which royally
  # screws incredibuild. this hack disables it.
  # for details see: http://connect.microsoft.com/VisualStudio/feedback/details/368107/clxx-fatal-error-c1027-inconsistent-values-for-ym-between-creation-and-use-of-precompiled-headers
  # http://www.ogre3d.org/forums/viewtopic.php?f=2&t=60015
  # http://www.cmake.org/pipermail/cmake/2009-September/032143.html
  string(REPLACE "/Zm1000" " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

    

  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO 
      "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ox /Zc:wchar_t- /Zi /MD /MP /GR /O2 -D_SECURE_STL=0 -D_HAS_ITERATOR_DEBUGGING=0"
      CACHE STRING "C++ compiler release-with-debug options" FORCE)
  set(CMAKE_CXX_FLAGS_RELEASE
      "${CMAKE_CXX_FLAGS_RELEASE} ${LL_CXX_FLAGS} /Ox /Zc:wchar_t- /Zi /GR /MD /MP /O2 /Ob2 -D_SECURE_STL=0 -D_HAS_ITERATOR_DEBUGGING=0"
      CACHE STRING "C++ compiler release options" FORCE)
  
  # zlib has assembly-language object files incompatible with SAFESEH
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE /SAFESEH:NO /NODEFAULTLIB:LIBCMT /IGNORE:4099")

  set(CMAKE_CXX_STANDARD_LIBRARIES "")
  set(CMAKE_C_STANDARD_LIBRARIES "")

  add_definitions(
      /DWIN32
      /D_WINDOWS
      /DLL_WINDOWS=1
      /DNOMINMAX
#      /DDOM_DYNAMIC            # For shared library colladadom
      /DUNICODE
      /D_UNICODE
      /DWINVER=0x0A00
      /D_WIN32_WINNT=0x0A00
      /DLL_OS_DRAGDROP_ENABLED=1
      /DCARES_STATICLIB
      /DLIB_NDOF=1
      )
  add_compile_options(
      /GS
      /TP
      /W3
      /c
      /Zc:forScope
      /nologo
      /Oy-
      /arch:SSE2
      /fp:fast
      )
     
  # Are we using the crummy Visual Studio KDU build workaround?
  if (NOT VS_DISABLE_FATAL_WARNINGS)
    add_definitions(/WX)
  endif (NOT VS_DISABLE_FATAL_WARNINGS)
endif (WINDOWS)




if (USESYSTEMLIBS)
  add_definitions(-DLL_USESYSTEMLIBS=1)
endif (USESYSTEMLIBS)

endif(NOT DEFINED ${CMAKE_CURRENT_LIST_FILE}_INCLUDED)
