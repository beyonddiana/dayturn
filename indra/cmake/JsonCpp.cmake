# -*- cmake -*-

include(Prebuilt)

set(JSONCPP_FIND_QUIETLY ON)
set(JSONCPP_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindJsonCpp)
else (USESYSTEMLIBS)
  use_prebuilt_binary(jsoncpp)
  if (WINDOWS)
    set(JSONCPP_LIBRARIES 
      debug json_libmdd.lib
      optimized json_libmd.lib)
  elseif (LINUX)
    if(${ARCH} STREQUAL "x86_64")
      set(JSONCPP_LIBRARIES libjson_linux-gcc-4.6_libmt.a)
    else(${ARCH} STREQUAL "x86_64")
      set(JSONCPP_LIBRARIES libjson_linux-gcc-4.1.3_libmt.a)
    endif(${ARCH} STREQUAL "x86_64")
  endif (WINDOWS)
  set(JSONCPP_INCLUDE_DIR "${LIBS_PREBUILT_DIR}/include/jsoncpp" "${LIBS_PREBUILT_DIR}/include/json")
endif (USESYSTEMLIBS)
