# compiler.cmake
# any compiler specific details do them here

if(CMAKE_GENERATOR STREQUAL Xcode)
  set(CMAKE_OSX_ARCHITECTURES "\$(ARCHS_STANDARD)")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CXX_STANDARD)
	set(CMAKE_CXX_STANDARD 23)
endif()

if (CMAKE_CXX_STANDARD LESS 20)
    message(FATAL_ERROR "SRCpp requires C++20 or higher.")
endif()

# set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-format-style=file")

macro(SetupCompilerForTarget arg cpp_version)
  if(NOT MSVC)
  target_compile_options(${arg} PRIVATE -Wall -Wextra)
  endif()
  set_target_properties(${arg} PROPERTIES CXX_STANDARD ${cpp_version})
endmacro()

option(ENABLE_ASAN "Enable AddressSanitizer in Debug" ON)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_ASAN AND NOT MSVC)
    set(SANITIZER_FLAGS "-fsanitize=address")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
endif()
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer in Debug" ON)
if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND ENABLE_UBSAN AND NOT MSVC)
  set(UBSAN_FLAGS "-fsanitize=undefined")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${UBSAN_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${UBSAN_FLAGS}")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${UBSAN_FLAGS}")
endif()
