cmake_minimum_required(VERSION 3.23)

project(SRCpp VERSION 0.1)

#============================================================================
# Settable options
#============================================================================
option(SRCPP_WITH_TESTS "Build tests." OFF)
option(SRCPP_WITH_EXAMPLE "Build example." OFF)

include(GNUInstallDirs)

include(${PROJECT_SOURCE_DIR}/cmake/compiler.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/dependencies.cmake)

add_library(
  SRCpp
  INTERFACE
  include/SRCpp/SRCpp.hpp
)

target_include_directories(
  SRCpp
  INTERFACE
  include
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


