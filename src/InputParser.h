//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_INPUTPARSER_H
#define CODERSSTRIKEBACKC_INPUTPARSER_H

#include "State.h"

class InputParser {
    Race *race;
    int turn = 0;
    istream& stream;

public:
    InputParser(istream& stream) : stream(stream) {};
    void init();
    Race getRace() {return *race;}
    GameState parseGameState();
};

#endif //CODERSSTRIKEBACKC_INPUTPARSER_H
