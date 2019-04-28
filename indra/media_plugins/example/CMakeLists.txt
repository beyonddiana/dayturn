# -*- cmake -*-

project(media_plugin_example)

include(00-Common)
include(LLCommon)
include(LLImage)
include(LLPlugin)
include(LLMath)
include(LLRender)
include(LLWindow)
include(Linking)
include(PluginAPI)
include(MediaPluginBase)
include(OpenGL)

include(ExamplePlugin)

include_directories(
    ${LLPLUGIN_INCLUDE_DIRS}
    ${MEDIA_PLUGIN_BASE_INCLUDE_DIRS}
    ${LLCOMMON_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLIMAGE_INCLUDE_DIRS}
    ${LLRENDER_INCLUDE_DIRS}
    ${LLWINDOW_INCLUDE_DIRS}
)
include_directories(SYSTEM
    ${LLCOMMON_SYSTEM_INCLUDE_DIRS}
    )


### media_plugin_example

if(NOT WORD_SIZE EQUAL 32)
  if(WINDOWS)
    add_definitions(/FIXED:NO)
  else(WINDOWS) # not windows therefore gcc LINUX
    add_definitions(-fPIC)
  endif(WINDOWS)
endif(NOT WORD_SIZE EQUAL 32)

set(media_plugin_example_SOURCE_FILES
    media_plugin_example.cpp
    )

add_library(media_plugin_example
    SHARED
    ${media_plugin_example_SOURCE_FILES}
)

target_link_libraries(media_plugin_example
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  ${LLCOMMON_LIBRARIES}
  ${EXAMPLE_PLUGIN_LIBRARIES}
  ${PLUGIN_API_WINDOWS_LIBRARIES}
)

add_dependencies(media_plugin_example
  ${LLPLUGIN_LIBRARIES}
  ${MEDIA_PLUGIN_BASE_LIBRARIES}
  # Using ${LLCOMMON_LIBRARIES} here drags in a whole bunch of Boost stuff
  # that only produces CMake warnings about nonexistent dependencies.
  llcommon
)

if (WINDOWS)
  set_target_properties(
    media_plugin_example
    PROPERTIES
    LINK_FLAGS "/MANIFEST:NO /SAFESEH:NO /LTCG /NODEFAULTLIB:LIBCMT"
    )
endif (WINDOWS)

