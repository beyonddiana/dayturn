# -*- cmake -*-

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
    add_subdirectory(cef)
    add_subdirectory(libvlc)
endif (LINUX)

if (WINDOWS)
	add_subdirectory(cef)
    add_subdirectory(winmmshim)
    add_subdirectory(libvlc)
endif (WINDOWS)

### add_subdirectory(example)
