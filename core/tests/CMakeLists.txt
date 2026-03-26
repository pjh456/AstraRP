file(GLOB_RECURSE TEST_SOURCES "test_*.cpp")

foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE astrarp_lib)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

add_custom_target(astrarp_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running AstraRP tests")
