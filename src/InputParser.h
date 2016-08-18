#ifndef CODERSSTRIKEBACK_INPUTPARSER_H
#define CODERSSTRIKEBACK_INPUTPARSER_H

#include "State.h"

class InputParser {
    istream& stream;

public:
    InputParser(istream& stream) : stream(stream) {};
    Race init();
    void parseTurn(PlayerState playerStates[]);
};

#endif //CODERSSTRIKEBACK_INPUTPARSER_H
