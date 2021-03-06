# Collect source files into the "sources" variable and unit test files into the
# "gtest_sources" variable.
ign_get_libsources_and_unittests(sources gtest_sources)

ign_add_component(ogre SOURCES ${sources} GET_TARGET_NAME ogre_target)

if(OGRE_VERSION VERSION_LESS 1.10.3)
  add_definitions(-DOGRE_VERSION_LT_1_10_3)
endif()
if(OGRE_VERSION VERSION_LESS 1.10.1)
  add_definitions(-DOGRE_VERSION_LT_1_10_1)
endif()
if(OGRE_VERSION VERSION_LESS 1.12.0)
  add_definitions(-DOGRE_VERSION_LT_1_12_0)
endif()

find_package(OpenGL)

set_property(
  SOURCE OgreRenderEngine.cc OgreRTShaderSystem.cc OgreGpuRays.cc OgreMaterial.cc OgreDepthCamera.cc OgreThermalCamera.cc
  PROPERTY COMPILE_DEFINITIONS
  IGN_RENDERING_RESOURCE_PATH="${IGN_RENDERING_RESOURCE_PATH}"
  OGRE_RESOURCE_PATH="${OGRE_RESOURCE_PATH}"
)

target_link_libraries(${ogre_target}
  PUBLIC
    ${ignition-common${IGN_COMMON_VER}_LIBRARIES}
  PRIVATE
    ignition-plugin${IGN_PLUGIN_VER}::register
    ${OPENGL_LIBRARIES}
    IgnOGRE::IgnOGRE
    )

# Build the unit tests
ign_build_tests(TYPE UNIT SOURCES ${gtest_sources} LIB_DEPS ${ogre_target})

if(WIN32)
  # tests needs .dll in the same directory
  add_custom_command(TARGET ${ogre_target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:${ogre_target}> ${CMAKE_CURRENT_BINARY_DIR})
endif()


add_subdirectory(media)
