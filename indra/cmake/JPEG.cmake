# -*- cmake -*-
include(Prebuilt)

include(Linking)
set(JPEG_FIND_QUIETLY ON)
set(JPEG_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindJPEG)
else (USESYSTEMLIBS)
  use_prebuilt_binary(jpeglib)
  if (LINUX)
    set(JPEG_LIBRARIES jpeg)
  elseif (WINDOWS)
    set(JPEG_LIBRARIES jpeglib)
  endif (LINUX)
  set(JPEG_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
endif (USESYSTEMLIBS)
