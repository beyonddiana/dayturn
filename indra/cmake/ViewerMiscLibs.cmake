# -*- cmake -*-
include(Prebuilt)
if (USESYSTEMLIBS)
  set(USESYSTEMLIBS OFF)
  use_prebuilt_binary(slvoice)
  use_prebuilt_binary(libhunspell)
#kokuafixme
#   if(LINUX AND ${ARCH} STREQUAL "x86_64")
#     use_prebuilt_binary(32bitcompatibilitylibs)
#   endif(LINUX AND ${ARCH} STREQUAL "x86_64")
  set(USESYSTEMLIBS ON)
else (USESYSTEMLIBS)
  use_prebuilt_binary(libuuid)
  use_prebuilt_binary(slvoice)
  use_prebuilt_binary(fontconfig)
  if(LINUX)
     if (${ARCH} STREQUAL "x86_64")
      use_prebuilt_binary(32bitcompatibilitylibs)
      # for mesh, this is built with colladadom and
      # KOKUAFIXME: should be packaged with colladadom the next time packaging it
      use_prebuilt_binary(minizip)
     endif (${ARCH} STREQUAL "x86_64")
  endif(LINUX)
endif(USESYSTEMLIBS)
