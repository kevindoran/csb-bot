//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_INPUTPARSER_H
#define CODERSSTRIKEBACKC_INPUTPARSER_H

#include "State.h"

class InputParser {
    istream& stream;

public:
    InputParser(istream& stream) : stream(stream) {};
    Race init();
    vector<PlayerState> parseTurn();
};

#endif //CODERSSTRIKEBACKC_INPUTPARSER_H
