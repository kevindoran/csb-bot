//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_PODRACERBOT_H_H
#define CODERSSTRIKEBACKC_PODRACERBOT_H_H


#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>

#include "Navigation.h"
#include "Physics.h"

using namespace std;


class PodraceBot {
public:
    virtual ~PodraceBot() {};
    virtual PodOutput move(GameState& gameState) {};
};

class MinimalBot : PodraceBot {
    int racer;
public:
    MinimalBot(int racer) : racer(racer) {};
    PodOutput move(GameState& gameState) {
        Navigation nav(gameState.race);
        PodState pod = gameState.ourState().pods[racer];
        Checkpoint ck = gameState.race.checkpoints[pod.nextCheckpoint];
//        PodOutput move = nav.seek(gameState.ourState().pods[racer], ck.pos, 100);
//        PodOutput move = nav.turnSaturationAdjust(pod, nav.seek(gameState.ourState().pods[racer], ck.pos, 100));
        PodOutput move = nav.preemptSeek(pod);
        return move;//PodOutput(100, ck.pos);
    }
};

class Bouncer : PodraceBot {
    int racer;
public:
    Bouncer(int racer) : racer(racer) {};

    PodOutput move(GameState& gameState) {
        Navigation nav(gameState.race);
        PodState pod = gameState.ourState().pods[racer];
        int returnBuffer = 15;
        if(pod.turnsSinceCP < WANDER_TIMEOUT - returnBuffer) {
            return nav.intercept(pod, gameState.enemyState().leadPod());
        } else {
            return nav.preemptSeek(pod);
        }
    }

};




#endif //CODERSSTRIKEBACKC_PODRACERBOT_H_H
