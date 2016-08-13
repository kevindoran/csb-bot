#ifndef CODERSSTRIKEBACK_PODRACERBOT_H
#define CODERSSTRIKEBACK_PODRACERBOT_H


#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

#include "State.h"

using namespace std;


class PodraceBot {
public:
    virtual ~PodraceBot() {};
    virtual PodOutput move(GameState& gameState, int podID) {};
};

class MinimalBot : public PodraceBot {
public:
    PodOutput move(GameState& gameState, int podID);
};

class Bouncer : public PodraceBot {
public:
    PodOutput move(GameState& gameState, int podID);
};

#endif //CODERSSTRIKEBACK_PODRACERBOT_H
