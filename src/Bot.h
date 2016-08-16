#ifndef CODERSSTRIKEBACK_BOT_H
#define CODERSSTRIKEBACK_BOT_H

#include "State.h"

class DuelBot {
public:
    virtual void init(Race& race) = 0;
    /**
     * Calculate the moves for both podracers.
     */
    virtual PairOutput move(GameState& gameState) = 0;
};


class SimBot {
public:
    virtual void init(Race& race) = 0;
    virtual void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) = 0;
};



#endif //CODERSSTRIKEBACK_BOT_H
