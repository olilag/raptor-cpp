
add_library (Array INTERFACE)
target_include_directories (Array INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Array)
add_library (just_gtfs INTERFACE)
target_include_directories (just_gtfs INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/just_gtfs)
add_library (UnorderedBimap INTERFACE)
target_link_libraries (UnorderedBimap INTERFACE Array)
target_include_directories (UnorderedBimap INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/UnorderedBimap)
