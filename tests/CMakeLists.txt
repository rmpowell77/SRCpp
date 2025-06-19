include(CTest)
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG        2b60af89e23d28eefc081bc930831ee9d45ea58b # v3.8.1
)
FetchContent_MakeAvailable(Catch2)
set(TEST_SOURCES
  SRCppTestCommon.cpp
  SRCppTestPull.cpp
  SRCppTestPush.cpp
  SRCppTestConvert.cpp
)

foreach(standard IN ITEMS 20 23)
  foreach(test_src IN LISTS TEST_SOURCES)
    string(REPLACE ".cpp" "" test_name ${test_src})
    set(exe_name ${test_name}_cxx${standard})
    add_executable(${exe_name}
      ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}
      ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestUtils.cpp
    )
    target_link_libraries(
      ${exe_name}
      Catch2::Catch2WithMain
      SRCpp
      samplerate
    )
    add_test(NAME ${exe_name} COMMAND ${exe_name})
    SetupCompilerForTarget(${exe_name} ${standard})
  endforeach()
endforeach()

foreach(standard IN ITEMS 20 23)
  set(exe_name SRCppBuilds_cxx${standard})
  add_executable(${exe_name}
    ${CMAKE_CURRENT_SOURCE_DIR}/SRCppBuilds.cpp
  )
  target_link_libraries(
    ${exe_name}
    SRCpp
    samplerate
  )
  add_test(NAME ${exe_name} COMMAND ${exe_name})
  SetupCompilerForTarget(${exe_name} ${standard})
endforeach()
