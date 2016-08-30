# -*- cmake -*-

add_subdirectory(base)

if (LINUX)
    add_subdirectory(gstreamer010)
endif (LINUX)

if (DARWIN)
    add_subdirectory(quicktime)
endif (DARWIN)

if( LINUX )
    add_subdirectory(cef)
endif( LINUX )

if (WINDOWS)
	add_subdirectory(cef)
    add_subdirectory(winmmshim)
    add_subdirectory(libvlc)
endif (WINDOWS)

### add_subdirectory(example)
