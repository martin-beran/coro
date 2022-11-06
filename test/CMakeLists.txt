# Link Boost::Test library to test programs
link_libraries(
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    Threads::Threads
)

set(
    TEST_PROGRAMS
    dummy_boost
    generator
    lazy
    log
    sync
)

foreach (P ${TEST_PROGRAMS})
    add_executable(test_${P} test_${P}.cpp)
    add_test(NAME ${P} COMMAND test_${P})
endforeach()
