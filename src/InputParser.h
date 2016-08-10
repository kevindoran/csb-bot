#ifndef CODERSSTRIKEBACK_INPUTPARSER_H
#define CODERSSTRIKEBACK_INPUTPARSER_H

#include "State.h"

class InputParser {
    istream& stream;

public:
    InputParser(istream& stream) : stream(stream) {};
    Race init();
    vector<PlayerState> parseTurn();
};

#endif //CODERSSTRIKEBACK_INPUTPARSER_H
