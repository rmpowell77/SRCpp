include(CTest)
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)
set(TEST_SOURCES
  SRCppTestCommon.cpp
  SRCppTestPull.cpp
  SRCppTestPush.cpp
  SRCppTestConvert.cpp
)

set(CONVERT_TEST
  SRCppTestPullConvert
  SRCppTestPushConvert
)

set(CONVERT_TYPES
  short
  int
  float
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
      GTest::gtest_main
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

foreach(standard IN ITEMS 20 23)
  foreach(test_src IN LISTS CONVERT_TEST)
    foreach(To IN LISTS CONVERT_TYPES)
      foreach(From IN LISTS CONVERT_TYPES)
        set(test_name ${test_src}_${To}_${From}_cxx${standard})
        add_executable(${test_name}
          ${CMAKE_CURRENT_SOURCE_DIR}/${test_src}.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/SRCppTestUtils.cpp
        )
        target_link_libraries(
          ${test_name}
          GTest::gtest_main
          SRCpp
          samplerate
        )
        target_compile_definitions(${test_name} PRIVATE TO_TYPE=${To})
        target_compile_definitions(${test_name} PRIVATE FROM_TYPE=${From})
        add_test(NAME ${test_name} COMMAND ${test_name})
        SetupCompilerForTarget(${test_name} ${standard})
      endforeach()
    endforeach()
  endforeach()
endforeach()
