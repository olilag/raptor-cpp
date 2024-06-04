# CMakeList.txt : CMake project for ConnectionFinder, include source and define
# project specific logic here.

cmake_minimum_required (VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("ConnectionFinder")

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Release")
  set(CMAKE_BUILD_TYPE "Release")
endif()

# Compile options
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

add_library (cf_compiler_flags INTERFACE)
target_compile_features (cf_compiler_flags INTERFACE cxx_std_20)
target_compile_options(cf_compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:/W4;/utf-8>>"
)

add_subdirectory (${PROJECT_SOURCE_DIR}/lib)
add_subdirectory (${PROJECT_SOURCE_DIR}/src)
add_subdirectory (${PROJECT_SOURCE_DIR}/testing)
