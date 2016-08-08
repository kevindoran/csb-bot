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
    virtual PodOutput move(GameState& gameState, int podID) {};
};

class MinimalBot : PodraceBot {
public:
    PodOutput move(GameState& gameState, int podID) {
        // Where should these two go...
        Navigation nav(gameState.race);
        Physics physics(gameState.race);
        PodState& pod = gameState.ourState().pods[podID];
        Checkpoint ck = gameState.race.checkpoints[pod.nextCheckpoint];
//        PodOutput move = nav.seek(gameState.ourState().pods[racer], ck.pos, 100);
//        PodOutput move = nav.turnSaturationAdjust(pod, nav.seek(gameState.ourState().pods[racer], ck.pos, 100));
        PodOutput move = nav.preemptSeek(pod);
        double velThreshold = 220;
        if(gameState.turn > 0) {
            for (int i = 0; i < gameState.enemyState().pods.size(); i++) {
                const PodState& enemyPod = gameState.enemyState().pods[i];
                const PodState& previousEnemyPod = gameState.enemyState().lastPods[i];
                if (pod.turnsSinceShield >= SHIELD_COOLDOWN && physics.isCollision(pod, move, enemyPod,
                                        physics.expectedControl(previousEnemyPod, enemyPod), velThreshold)) {
                    cerr << "SHIELD enabled!" << endl;
                    move.enableShield();
                    break;
                }
            }
            if(move.thrust != PodOutput::SHIELD) {
                // Maybe best to save last player history so it is easy to get at the lead/lagging pod easily.
                // Also, better performance if I actually use the real bouncer pod output.
                const PodState& bouncerPod = gameState.ourState().laggingPod();
                const PodState& previousBouncerPod = gameState.ourState().lastPods[(gameState.ourState().leadPodID + 1) % PLAYER_COUNT];
                if(pod.turnsSinceShield >= SHIELD_COOLDOWN && physics.isCollision(pod, move, bouncerPod,
                                        physics.expectedControl(previousBouncerPod, bouncerPod), velThreshold)) {
                    move.enableShield();
                }
            }
        }
        return move;//PodOutput(100, ck.pos);
    }
};

class Bouncer : PodraceBot {
public:

    PodOutput move(GameState& gameState, int podID) {
        Navigation nav(gameState.race);
        Physics physics(gameState.race);
        PodState& pod = gameState.ourState().pods[podID];
        int returnBuffer = 10;
        PodOutput move;
        if(pod.turnsSinceCP < WANDER_TIMEOUT - returnBuffer) {
            cerr << "Targetting: " << gameState.enemyState().leadPodID << endl;
            move = nav.intercept(pod, gameState.enemyState().leadPod());
        } else {
            move = nav.preemptSeek(pod);
        }
        if(gameState.turn > 0) {
            double friendlyVelThreshold = 30;
            const PodState& lead = gameState.ourState().leadPod();
            const PodState& previousLead = gameState.ourState().lastPods[gameState.ourState().leadPodID];
            if(physics.isCollision(pod, move, lead, physics.expectedControl(previousLead, lead), friendlyVelThreshold)) {
                // Cheap way of avoiding the lead. Only one step ahead.
                Vector diff = lead.pos - pod.pos;
                move = PodOutput(MAX_THRUST, -diff);
                return move;
            }
            double velThreshold = 150;
            for (int i = 0; i < gameState.enemyState().pods.size(); i++) {
                const PodState& enemyPod = gameState.enemyState().pods[i];
                const PodState& previousEnemyPod = gameState.enemyState().lastPods[i];
                if (pod.turnsSinceShield >= SHIELD_COOLDOWN && physics.isCollision(pod, move, enemyPod, physics.expectedControl(previousEnemyPod, enemyPod), velThreshold)) {
                    cerr << "SHIELD enabled!" << endl;
                    move.enableShield();
                    break;
                }
            }
        }
        return move;
    }
};




#endif //CODERSSTRIKEBACKC_PODRACERBOT_H_H
