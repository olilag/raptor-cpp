
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG "main"
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(RFTests RouteFinderTests.cpp)
target_link_libraries(RFTests PRIVATE GTest::gtest_main raptor PUBLIC cf_compiler_flags)

enable_testing()

include(GoogleTest)
gtest_discover_tests(RFTests)
