project(PodracerBot)

set(HEADER_FILES
        InputParser.h
        Physics.h
        State.h
        podracerBot.h
        Vector.h
        Navigation.h)


set(SOURCE_FILES
        InputParser.cpp
        Physics.cpp
        podracerBot.cpp
        Vector.cpp
        Navigation.cpp
        State.cpp)

add_library(PodracerBot STATIC ${SOURCE_FILES} ${HEADER_FILES})


