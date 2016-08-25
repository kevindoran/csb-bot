#ifndef CODERSSTRIKEBACK_BOT_H
#define CODERSSTRIKEBACK_BOT_H

#include "State.h"

class DuelBot {
public:
    /**
     * Calculate the moves for both podracers.
     */
    virtual PairOutput move(GameState& gameState) = 0;
};


class SimBot {
public:
    virtual void move(PodState ourPods[], PodState enemyPods[]) = 0;
    virtual void setTurn(int turn) {};
};



#endif //CODERSSTRIKEBACK_BOT_H
