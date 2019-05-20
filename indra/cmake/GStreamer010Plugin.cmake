# -*- cmake -*-
include(Prebuilt)

if (USESYSTEMLIBS)
  include(FindPkgConfig)

  pkg_check_modules(GSTREAMER010 REQUIRED gstreamer-0.10)
  pkg_check_modules(GSTREAMER010_PLUGINS_BASE REQUIRED gstreamer-plugins-base-0.10)
 else (USESYSTEMLIBS)
#  use_prebuilt_binary(gstreamer)
  # possible libxml2 should have its own .cmake file instead
  use_prebuilt_binary(libxml2)
  # Possibly libxml and glib should have their own .cmake file instead...
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
	use_prebuilt_binary(glib)		# gstreamer needs glib
	use_prebuilt_binary(gstreamer)
	use_prebuilt_binary(libxml2)
      if (LINUX AND ${ARCH} STREQUAL "x86_64")
	set(GSTREAMER010_INCLUDE_DIRS
		${LIBS_PREBUILT_DIR}/include/gstreamer-0.10
		${LIBS_PREBUILT_DIR}/include/glib # Linux 64 bit new lib
		${LIBS_PREBUILT_DIR}/include/gobject
		${LIBS_PREBUILT_DIR}/include/gio
		${LIBS_PREBUILT_DIR}/include/gmodule
		${LIBS_PREBUILT_DIR}/include/libxml2
		)
      endif (LINUX AND ${ARCH} STREQUAL "x86_64")
      if (LINUX AND ${ARCH} STREQUAL "i686")
	set(GSTREAMER010_INCLUDE_DIRS
		${LIBS_PREBUILT_DIR}/include/gstreamer-0.10
		${LIBS_PREBUILT_DIR}/include/glib 
		${LIBS_PREBUILT_DIR}/include/gobject
		${LIBS_PREBUILT_DIR}/include/gio
		${LIBS_PREBUILT_DIR}/include/gmodule
		${LIBS_PREBUILT_DIR}/include/libxml2
		)
     endif (LINUX AND ${ARCH} STREQUAL "i686")
  endif (WINDOWS)

endif (USESYSTEMLIBS)

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
  if (LINUX AND ${ARCH} STREQUAL "x86_64")
    use_prebuilt_binary(glib)
    use_prebuilt_binary(atk)
	use_prebuilt_binary(cairo)
	use_prebuilt_binary(pango)
	use_prebuilt_binary(pixman)
    use_prebuilt_binary(gtk)
	use_prebuilt_binary(gdk-pixbuf)
	use_prebuilt_binary(harfbuzz)		
    set(GSTREAMER010_LIBRARIES
        gstvideo-0.10
        gstaudio-0.10
        gstbase-0.10
        gstreamer-0.10
        atk-1.0
        cairo
        gdk-x11-2.0
        gdk_pixbuf-2.0
        Xinerama
        glib-2.0
        gio-2.0
        gmodule-2.0
        gobject-2.0
        gthread-2.0
        gtk-x11-2.0
        pango-1.0
        pangoft2-1.0
        #pangox-1.0 this library is obsolete http://ftp.gnome.org/pub/GNOME/sources/pangox-compat/ if need here is the source
        pangoxft-1.0
        pixman-1
        ${FREETYPE_LIBRARIES}
        pangocairo-1.0
        dl
        rt
        )
  endif (LINUX AND ${ARCH} STREQUAL "x86_64")

  if (LINUX AND ${ARCH} STREQUAL "i686")
    use_prebuilt_binary(glib)
    use_prebuilt_binary(atk)
	use_prebuilt_binary(cairo)
	use_prebuilt_binary(pango)
	use_prebuilt_binary(pixman)
    use_prebuilt_binary(gtk)
	use_prebuilt_binary(gdk-pixbuf)
	use_prebuilt_binary(harfbuzz)		
    set(GSTREAMER010_LIBRARIES
        gstvideo-0.10
        gstaudio-0.10
        gstbase-0.10
        gstreamer-0.10
        atk-1.0
        cairo
        gdk-x11-2.0
        gdk_pixbuf-2.0
        Xinerama
        glib-2.0
        gio-2.0
        gmodule-2.0
        gobject-2.0
        gthread-2.0
        gtk-x11-2.0
        pango-1.0
        pangoft2-1.0
        #pangox-1.0 this library is obsolete http://ftp.gnome.org/pub/GNOME/sources/pangox-compat/ if need here is the source
        pangoxft-1.0
        pixman-1
        ${FREETYPE_LIBRARIES}
        pangocairo-1.0
        dl
        rt
        )
  endif (LINUX AND ${ARCH} STREQUAL "i686")
endif (WINDOWS)


if (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)
    set(GSTREAMER010 ON CACHE BOOL "Build with GStreamer-0.10 streaming media support.")
    add_definitions(-DLL_GSTREAMER010_ENABLED=1)
endif (GSTREAMER010_FOUND AND GSTREAMER010_PLUGINS_BASE_FOUND)
