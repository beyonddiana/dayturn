# -*- cmake -*-
include(Linking)
include(Prebuilt)

set(CARES_FIND_QUIETLY ON)
set(CARES_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindCARes)
else (USESYSTEMLIBS)
    use_prebuilt_binary(ares)
    add_definitions("-DCARES_STATICLIB")
    if (WINDOWS)
        set(CARES_LIBRARIES areslib)
    else (WINDOWS)
        set(CARES_LIBRARIES cares)
    endif (WINDOWS)
    set(CARES_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/ares)
endif (USESYSTEMLIBS)
