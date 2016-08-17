cmake_minimum_required(VERSION 3.5)
project(codersStrikeBackC)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(src lib)

add_subdirectory(src)
add_subdirectory(test)

add_custom_target(deploy)
add_dependencies(deploy PodracerBot)
add_custom_command(TARGET deploy POST_BUILD
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/merge.py ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/merged.cpp
        )

add_executable(main src/main.cpp)
target_link_libraries(main PodracerBot)

add_executable(simulation src/simulationMain.cpp)
target_link_libraries(simulation PodracerBot)

add_executable(merged merged.cpp)
add_dependencies(merged deploy)

