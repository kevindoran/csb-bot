#ifndef CODERSSTRIKEBACK_PODRACERBOT_H
#define CODERSSTRIKEBACK_PODRACERBOT_H


#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

#include "State.h"
#include "Bot.h"

using namespace std;


class SingleBot {
public:
    virtual ~SingleBot() {};
    virtual PodOutputAbs move(GameState& gameState, int podID) {};
};

class Racer : public SingleBot {
public:
    PodOutputAbs move(GameState& gameState, int podID);
};

class Bouncer : public SingleBot {
public:
    PodOutputAbs move(GameState& gameState, int podID);
};

// Wrapper around the old bots so that they can work in the simulation.
class TraditionalBot : public DuelBot {
    Racer racer;
    Bouncer bouncer;

public:
    void init(Race& race) {
    }

    PairOutput move(GameState& state) {
        int leadPodID = state.ourState().leadPodID;
        PodOutputAbs po1 = 0 == leadPodID ? racer.move(state, 0) : bouncer.move(state, 0);
        PodOutputAbs po2 = 1 == leadPodID ? racer.move(state, 1) : bouncer.move(state, 1);
        return PairOutput(PodOutputSim::fromAbsolute(state.ourState().pods[0], po1),
                          PodOutputSim::fromAbsolute(state.ourState().pods[1], po2));
    }
};

#endif //CODERSSTRIKEBACK_PODRACERBOT_H
