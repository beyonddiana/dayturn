# -*- cmake -*-
include(Linking)
include(Prebuilt)

if (USESYSTEMLIBS)
    set(CEFPLUGIN OFF CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
else (USESYSTEMLIBS)
    use_prebuilt_binary(llceflib)
    set(CEFPLUGIN ON CACHE BOOL
        "CEFPLUGIN support for the llplugin/llmedia test apps.")
        set(CEF_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/include/cef)
endif (USESYSTEMLIBS)

if (WINDOWS)
    set(CEF_PLUGIN_LIBRARIES
        libcef.lib
        libcef_dll_wrapper.lib
        llceflib.lib
    )

elseif (LINUX)
    set(CEF_PLUGIN_LIBRARIES
        libcef.so
        libcef_dll_wrapper.a
        libllceflib.so
    )
endif (WINDOWS)
if (LINUX)
      add_definitions(-std=gnu++11)
endif (LINUX)
