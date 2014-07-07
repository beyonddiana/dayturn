# -*- cmake -*-
include(Prebuilt)

set(CURL_FIND_QUIETLY ON)
set(CURL_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindCURL)
else (USESYSTEMLIBS)
  use_prebuilt_binary(curl)
  if (WINDOWS)
    set(CURL_LIBRARIES 
    debug libcurld.lib
    optimized libcurl.lib)
  else (WINDOWS)
     if (LINUX AND ${ARCH} STREQUAL "x86_64")
       set(CURL_LIBRARIES curl)
    else ()
       set(CURL_LIBRARIES libcurl.a)
    endif ()
  endif (WINDOWS)
  set(CURL_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)
