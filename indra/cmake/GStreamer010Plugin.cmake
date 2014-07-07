# -*- cmake -*-
include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindPkgConfig)

  pkg_check_modules(GSTREAMER010 REQUIRED gstreamer-0.10)
  pkg_check_modules(GSTREAMER010_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10)

else (STANDALONE)
  # possible libxml2 should have its own .cmake file instead
  use_prebuilt_binary(libxml2)
  use_prebuilt_binary(gstreamer)	# includes glib, libxml, and iconv on Windows
  set(GSTREAMER010_FOUND ON FORCE BOOL)
  set(GSTREAMER010_PLUGINS_BASE_FOUND ON FORCE BOOL)

  if (WINDOWS)
    # gstreamer-plugins are packaged with gstreamer now.
    # In case someone wants to have 2 packages again in future uncomment:
    # use_prebuilt_binary(gst_plugins)
    set(GSTREAMER010_INCLUDE_DIRS
		${LIBS_PREBUILT_DIR}/include/gstreamer-0.10
		${LIBS_PREBUILT_DIR}/include/glib
		${LIBS_PREBUILT_DIR}/include/libxml2
		)
  else (WINDOWS)
    use_prebuilt_binary(glib)			# gstreamer needs glib
	use_prebuilt_binary(libxml)
	set(GSTREAMER010_INCLUDE_DIRS
		${LIBS_PREBUILT_DIR}/include/gstreamer-0.10
		${LIBS_PREBUILT_DIR}/include/glib-2.0
		${LIBS_PREBUILT_DIR}/include/libxml2
		)
  endif (WINDOWS)

endif (STANDALONE)

if (WINDOWS)

  # We don't need to explicitly link against gstreamer itself, because
  # LLMediaImplGStreamer probes for the system's copy at runtime.
    set(GSTREAMER010_LIBRARIES
         gstaudio-0.10.lib
         gstbase-0.10.lib
         gstreamer-0.10.lib
         gstvideo-0.10.lib #slvideoplugin
		 gstinterfaces-0.10.lib
         gobject-2.0
         gmodule-2.0
         gthread-2.0
         glib-2.0
         )
else (WINDOWS)
    set(GSTREAMER010_LIBRARIES
         gstvideo-0.10
         gstaudio-0.10
         gstbase-0.10
         gstreamer-0.10
         gobject-2.0
         gmodule-2.0
         dl
         gthread-2.0
         rt
         glib-2.0
         )
endif (USESYSTEMLIBS)


if (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)
  if (NOT DARWIN)
    set(GSTREAMER010 ON CACHE BOOL "Build with GStreamer-0.10 streaming media support.")
    add_definitions(-DLL_GSTREAMER010_ENABLED=1)
  endif (NOT DARWIN)
endif (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)
