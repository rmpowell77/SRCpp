cmake_minimum_required(VERSION 3.23)

project(SRCpp VERSION 0.0.1 LANGUAGES CXX)

#============================================================================
# Settable options
#============================================================================
option(SRCPP_WITH_TESTS "Build tests." OFF)
option(SRCPP_WITH_EXAMPLE "Build example." OFF)

# Define header-only interface library
add_library(SRCpp INTERFACE)

include(${PROJECT_SOURCE_DIR}/cmake/compiler.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake)

# Require C++20
target_compile_features(SRCpp INTERFACE cxx_std_20)

# Specify include directories
target_include_directories(SRCpp INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Installation support
include(GNUInstallDirs)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS SRCpp
    EXPORT SRCppTargets
)

install(EXPORT SRCppTargets
    NAMESPACE SRCpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SRCpp
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/SRCppConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/SRCppConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SRCpp
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SRCppConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SRCppConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SRCppConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SRCpp
)

#============================================================================
# Examples
#============================================================================
if(SRCPP_WITH_EXAMPLE)
  add_subdirectory(examples)
endif()


#============================================================================
# Tests
#============================================================================
if(SRCPP_WITH_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()


