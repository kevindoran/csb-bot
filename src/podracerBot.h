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
    // TODO: got integer overflow (-INF) output on one game. Not sure why.
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
                double friendlyThreshold = velThreshold/2; // We know our bouncer wont use a shield.
                const PodState& bouncerPod = gameState.ourState().laggingPod();
                const PodState& previousBouncerPod = gameState.ourState().lastPods[(gameState.ourState().leadPodID + 1) % PLAYER_COUNT];
                if(pod.turnsSinceShield >= SHIELD_COOLDOWN && physics.isCollision(pod, move, bouncerPod,
                                        physics.expectedControl(previousBouncerPod, bouncerPod), friendlyThreshold)) {
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
        cerr << "Targetting: " << gameState.enemyState().leadPodID << endl;
        if(pod.turnsSinceCP < WANDER_TIMEOUT - returnBuffer) {
            Vector target = nav.find_intercept(pod, gameState.enemyState().leadPod());
            PodState& leadPod = gameState.enemyState().leadPod();
            int leadID = gameState.enemyState().leadPodID;
            // Need a tidier way of finding the nextNextCP.
            int nextNextCP = (leadPod.nextCheckpoint + 1) % gameState.race.checkpoints.size();
            if(gameState.turn > 0 && target == gameState.race.checkpoints[nextNextCP].pos) {
                // Move towards target and spin towards enemy.
                PodState nextPos = physics.move(leadPod, physics.expectedControl(gameState.enemyState().lastPods[leadID], leadPod), 1);
                int turnThreshold = 13;
                int seekThreshold = 2;
                move =  nav.preemptSeek(pod, gameState.race.checkpoints[nextNextCP].pos, CHECKPOINT_RADIUS, nextPos.pos,
                    turnThreshold, seekThreshold);
            } else {
                move = nav.intercept(pod, gameState.enemyState().leadPod());
            }
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
