#include "State.h"
#include "Physics.h"
#include "Navigation.h"
#include "PodracerBot.h"

// TODO: got integer overflow (-max int) output on one game. Not sure why.
PodOutput MinimalBot::move(GameState& gameState, int podID) {
// Where should these two go...
    Navigation nav(gameState.race);
    Physics physics(gameState.race);
    PodState& pod = gameState.ourState().pods[podID];
    Checkpoint ck = gameState.race.checkpoints[pod.nextCheckpoint];
    PodOutput move;
    if(pod.passedCheckpoints == gameState.race.laps * gameState.race.checkpoints.size() - 1) {
        // Last checkpoint, no need to line-up the following checkpoint.
        cerr << "Last Checkpoint!" << endl;
        move = nav.turnSaturationAdjust(pod, nav.seek(pod, ck.pos));
    } else {
        move = nav.preemptSeek(pod);
    }
    double velThreshold = 190;
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
            double friendlyThreshold = velThreshold/3; // We know our bouncer wont use a shield.
            const PodState& bouncerPod = gameState.ourState().laggingPod();
            const PodState& previousBouncerPod = gameState.ourState().lastPods[(gameState.ourState().leadPodID + 1) % PLAYER_COUNT];
            if(pod.turnsSinceShield >= SHIELD_COOLDOWN && physics.isCollision(pod, move, bouncerPod,
                                                                              physics.expectedControl(previousBouncerPod, bouncerPod), friendlyThreshold)) {
                move.enableShield();
            }
        }
    }
    if(pod.boostAvailable && move.thrust != PodOutput::SHIELD) {
        double boostAngleLimit = M_PI * (4.0 / 180.0);
        double minimumDistFactor = 0.8;
        double distThreshold = gameState.race.maxCheckpointDist * minimumDistFactor;
        if(abs(physics.angleTo(pod.pos, ck.pos) - pod.angle) < boostAngleLimit &&
           (ck.pos - pod.pos).getLength() >= distThreshold) {
            move.thrust = PodOutput::BOOST;
            pod.boostAvailable = false;
        }
    }
    return move;
}

PodOutput Bouncer::move(GameState& gameState, int podID) {
    Navigation nav(gameState.race);
    Physics physics(gameState.race);
    PodState& pod = gameState.ourState().pods[podID];
    int returnBuffer = 20;
    PodOutput move;
    int minTurnsSinceCP = min(gameState.ourState().pods[0].turnsSinceCP, gameState.ourState().pods[1].turnsSinceCP);
    if(gameState.turn > 0 && minTurnsSinceCP < WANDER_TIMEOUT - returnBuffer) {
        Vector target = nav.find_intercept(pod, gameState.enemyState().leadPod());
        PodState& leadPod = gameState.enemyState().leadPod();
        int leadID = gameState.enemyState().leadPodID;
        // Need a tidier way of finding the nextNextCP.
        int nextNextCP = (leadPod.nextCheckpoint + 1) % gameState.race.checkpoints.size();
        if(gameState.turn > 0 && target == gameState.race.checkpoints[nextNextCP].pos) {
            cerr << "Targeting next next CP: " << nextNextCP << endl;
            // Move towards target and spin towards enemy.
            PodState nextPos = physics.move(leadPod, physics.expectedControl(gameState.enemyState().lastPods[leadID], leadPod), 1);
            int turnThreshold = 8;
            int seekThreshold = 0;
            move =  nav.preemptSeek(pod, gameState.race.checkpoints[nextNextCP].pos, CHECKPOINT_RADIUS*3, nextPos.pos,
                                    turnThreshold, seekThreshold);
        } else {
            cerr << "Intercepting pod: " << leadID << endl;
            move = nav.intercept(pod, gameState.enemyState().leadPod());
        }
    } else {
        move = nav.turnSaturationAdjust(pod, nav.seek(pod, gameState.race.checkpoints[pod.nextCheckpoint].pos));
        if(pod.boostAvailable && move.thrust != PodOutput::SHIELD) {
            double boostAngleLimit = M_PI * (5.0 / 180.0);
            double minimumDistFactor = 0.7;
            Checkpoint ck = gameState.race.checkpoints[pod.nextCheckpoint];
            double distThreshold = gameState.race.maxCheckpointDist * minimumDistFactor;
            if(abs(physics.angleTo(pod.pos, ck.pos) - pod.angle) < boostAngleLimit &&
               (ck.pos - pod.pos).getLength() >= distThreshold) {
                move.thrust = PodOutput::BOOST;
                pod.boostAvailable = false;
            }
        }
    }
    if(gameState.turn > 0) {
        double velThreshold = 150;
        for (int i = 0; i < gameState.enemyState().pods.size(); i++) {
            const PodState& enemyPod = gameState.enemyState().pods[i];
            const PodState& previousEnemyPod = gameState.enemyState().lastPods[i];
            if (pod.turnsSinceShield >= SHIELD_COOLDOWN && physics.isCollision(pod, move, enemyPod, physics.expectedControl(previousEnemyPod, enemyPod), velThreshold)) {
                move.enableShield();
                break;
            }
        }
        double friendlyVelThreshold = 60;
        int turnsAhead = 2;
        const PodState& lead = gameState.ourState().leadPod();
        const PodState& previousLead = gameState.ourState().lastPods[gameState.ourState().leadPodID];
        const PodState& ourPrev = gameState.ourState().lastPods[podID];
        // If we are already engaged with the enemy (shield enabled) all collision predictions will be useless,
        // better to just keep pestering the enemy.
        if(move.thrust != PodOutput::SHIELD && physics.isCollision(pod,
                // Best to not use our control, as it would ignore collisions or other things that would slow us down.
                                                                   physics.expectedControl(ourPrev, pod),  // instead of: move.
                                                                   lead, physics.expectedControl(previousLead, lead),
                                                                   friendlyVelThreshold, turnsAhead)) {
            // Cheap way of avoiding the lead. Only one step ahead.
            Vector diff = lead.pos - pod.pos;
            cerr << "Avoiding friendly" << endl;
            move = PodOutput(MAX_THRUST, -diff);
        }
    }
    return move;
}


