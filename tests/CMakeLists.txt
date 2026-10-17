# Standalone "oracle" tests: each test-*.cpp is a self-contained program with a
# main() that returns 0 when every check passes and non-zero otherwise.
# They link against the real core/util static libraries, so they exercise the
# same code the application ships (no re-implementation of the logic under test).
file(GLOB ORACLE_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test-*.cpp")

foreach(ORACLE_SOURCE ${ORACLE_TEST_SOURCES})
    get_filename_component(ORACLE_NAME ${ORACLE_SOURCE} NAME_WE)

    add_executable(${ORACLE_NAME} ${ORACLE_SOURCE})
    target_link_libraries(${ORACLE_NAME} PRIVATE xoj::core xoj::util)
    target_compile_features(${ORACLE_NAME} PRIVATE cxx_std_17)

    add_test(NAME ${ORACLE_NAME} COMMAND ${ORACLE_NAME})
    set_tests_properties(${ORACLE_NAME} PROPERTIES
        LABELS "oracle"
        TIMEOUT 120
    )
endforeach()
