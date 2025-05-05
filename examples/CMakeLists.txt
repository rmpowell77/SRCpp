cmake_minimum_required(VERSION 3.23)

add_executable(
  SRCppExamples
  SRCppExamples.cpp
)

target_link_libraries(
  SRCppExamples 
  samplerate
  SRCpp
)

SetupCompilerForTarget(SRCppExamples)
