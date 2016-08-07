//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_INPUTPARSER_H
#define CODERSSTRIKEBACKC_INPUTPARSER_H

#include "State.h"

class InputParser {
    Race* race = 0;
    int turn = 0;
    istream& stream;
    GameState previous;
    bool isPreviousState = false;

public:
    InputParser(istream& stream) : stream(stream) {};
    void init();
    Race getRace() {return *race;}
    ~InputParser() { delete race; };
    GameState parseGameState();
};

#endif //CODERSSTRIKEBACKC_INPUTPARSER_H
