
add_library(raptor STATIC IdTranslator.cpp DataStructures.cpp DSHelperFunctions.cpp Algorithm.cpp)
target_link_libraries(raptor PUBLIC just_gtfs UnorderedBimap cf_compiler_flags)
target_include_directories(raptor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set (MY_EXE "ConnectionFinder")
add_executable (${MY_EXE} ConnectionFinder.cpp )
target_link_libraries (${MY_EXE}
  PUBLIC
  raptor
  cf_compiler_flags
)
