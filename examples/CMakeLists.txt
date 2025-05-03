cmake_minimum_required(VERSION 3.23)
project(SRCpp_examples VERSION 0.1)

add_executable(
  test_SRCpp
  test_SRCpp.cpp
)

target_link_libraries(
  test_SRCpp 
  samplerate
  SRCpp
)

SetupCompilerForTarget(test_SRCpp)
