# -*- cmake -*-

add_subdirectory(base)

add_subdirectory(webkit)
# *FIXME: Gstreamer darwin package
if (NOT DARWIN)
    add_subdirectory(gstreamer010)
endif (NOT DARWIN)

if (QUICKTIME)
    add_subdirectory(quicktime)
endif (QUICKTIME)

if (WINDOWS)
    add_subdirectory(winmmshim)
endif (WINDOWS)

add_subdirectory(example)
