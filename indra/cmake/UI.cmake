# -*- cmake -*-
include(Prebuilt)
include(FreeType)

if (USESYSTEMLIBS)
  include(FindPkgConfig)
    
  if (LINUX)
    set(PKGCONFIG_PACKAGES
        atk
        cairo
        gdk-2.0
        gdk-pixbuf-2.0
        glib-2.0
        gmodule-2.0
        gtk+-2.0
        gthread-2.0
        libpng
        pango
        pangoft2
        pangox
        pangoxft
        sdl
        )
  endif (LINUX)

  foreach(pkg ${PKGCONFIG_PACKAGES})
    pkg_check_modules(${pkg} REQUIRED ${pkg})
    include_directories(${${pkg}_INCLUDE_DIRS})
    link_directories(${${pkg}_LIBRARY_DIRS})
    list(APPEND UI_LIBRARIES ${${pkg}_LIBRARIES})
    add_definitions(${${pkg}_CFLAGS_OTHERS})
  endforeach(pkg)
else (USESYSTEMLIBS)
if (WINDOWS)
  use_prebuilt_binary(gtk-atk-pango-glib)
endif (WINDOWS)
  if (LINUX)
    use_prebuilt_binary(glib)		# gtk-etc needs glib
    use_prebuilt_binary(gtk-etc)

    set(UI_LIBRARIES
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
        pangox-1.0
        pangoxft-1.0
        pixman-1
        ${FREETYPE_LIBRARIES}
        pangocairo-1.0
        )
  endif (LINUX)

  include_directories (
      ${LIBS_PREBUILT_DIR}/include
      ${LIBS_PREBUILT_DIR}/include/cairo
      ${LIBS_PREBUILT_DIR}/include/pixman-1

      )
  foreach(include ${${LL_ARCH}_INCLUDES})
      include_directories(${LIBS_PREBUILT_DIR}/include/${include})
  endforeach(include)
endif (USESYSTEMLIBS)

if (LINUX)
  add_definitions(-DLL_GTK=1 -DLL_X11=1)
endif (LINUX)
