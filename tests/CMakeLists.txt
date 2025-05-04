include(CTest)
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        2b60af89e23d28eefc081bc930831ee9d45ea58b # v3.8.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(SRCppTestCommon
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestCommon.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestUtils.cpp
)

target_link_libraries(
  SRCppTestCommon
  Catch2::Catch2WithMain
  SRCpp
  samplerate
)

add_executable(SRCppTestPull
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestPull.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestUtils.cpp
)

target_link_libraries(
  SRCppTestPull
  Catch2::Catch2WithMain
  SRCpp
  samplerate
)

add_executable(SRCppTestPush
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestPush.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestUtils.cpp
)

target_link_libraries(
  SRCppTestPush
  Catch2::Catch2WithMain
  SRCpp
  samplerate
)

add_executable(SRCppTestConvert
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestConvert.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestUtils.cpp
)

target_link_libraries(
  SRCppTestConvert
  Catch2::Catch2WithMain
  SRCpp
  samplerate
)

add_executable(SRCppBuilds
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppBuilds.cpp
)

target_link_libraries(
  SRCppBuilds
  SRCpp
  samplerate
)

add_test(NAME SRCppTestCommon COMMAND SRCppTestCommon)
add_test(NAME SRCppTestConvert COMMAND SRCppTestConvert)
add_test(NAME SRCppTestPull COMMAND SRCppTestPull)
add_test(NAME SRCppTestPush COMMAND SRCppTestPush)
add_test(NAME SRCppBuilds COMMAND SRCppBuilds)

SetupCompilerForTarget(SRCppTestCommon)
SetupCompilerForTarget(SRCppTestConvert)
SetupCompilerForTarget(SRCppTestPush)
SetupCompilerForTarget(SRCppTestPull)
SetupCompilerForTarget(SRCppBuilds)
