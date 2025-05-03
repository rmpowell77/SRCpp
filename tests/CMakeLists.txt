include(CTest)
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        2b60af89e23d28eefc081bc930831ee9d45ea58b # v3.8.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(SRCppTests
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTests.cpp
)

target_link_libraries(
  SRCppTests
  Catch2::Catch2WithMain
  SRCpp
  samplerate
)

add_test(NAME UnitTest COMMAND SRCppTests)

add_executable(SRCppBuilds
  ${CMAKE_CURRENT_SOURCE_DIR}/SRCppBuilds.cpp
)

target_link_libraries(
  SRCppBuilds
  SRCpp
  samplerate
)

add_test(NAME BuildTest COMMAND SRCppBuilds)

SetupCompilerForTarget(SRCppTests)
