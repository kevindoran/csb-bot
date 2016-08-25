project(PodracerBot)

set(HEADER_FILES
        InputParser.h
        Physics.h
        State.h
        PodracerBot.h
        Vector.h
        Navigation.h
        AnnealingBot.h
        Simulation.h
        BlockingQueue.h)


set(SOURCE_FILES
        InputParser.cpp
        Physics.cpp
        PodracerBot.cpp
        Vector.cpp
        Navigation.cpp
        State.cpp
        AnnealingBot.cpp
        Simulation.cpp
        parameterSimMain.cpp)

add_library(PodracerBot STATIC ${SOURCE_FILES} ${HEADER_FILES})


