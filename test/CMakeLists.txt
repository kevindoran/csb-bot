project(PodracerBot_tests)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")

set(gtest_SOURCE_DIR lib/googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
add_subdirectory(${gtest_SOURCE_DIR})

add_executable(runTests
        input_parser_test.cpp
        physics_test.cpp
        navigation_test.cpp
        duel_bot_test.cpp)

target_link_libraries(runTests gtest gtest_main)
target_link_libraries(runTests PodracerBot)
