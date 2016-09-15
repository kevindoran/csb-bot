
#include "State.h"
#include "Physics.h"

PodOutputSim PodOutputSim::fromAbsolute(const PodState& pod, const PodOutputAbs& abs) {
    PodOutputSim o;
    o.thrust = abs.thrust;
    if(abs.thrust == PodOutputAbs::SHIELD) o.shieldEnabled = true;
    else if(abs.thrust == PodOutputAbs::BOOST) o.boostEnabled = true;
    o.angle = Physics::turnAngle(pod, abs.target);
    return o;
}

PodOutputAbs PodOutputSim::absolute(const PodState& pod) {
    Vector target = pod.pos + Vector::fromMagAngle(10000, pod.angle + angle);
    PodOutputAbs po(thrust, target);
    if(shieldEnabled) po.enableShield();
    else if(boostEnabled) po.enableBoost();
    return po;
}

void State::preTurnUpdate(PlayerState playerStates[]) {
    if(turn > 0) {
        for (int i = 0; i < PLAYER_COUNT; i++) {
            playerStates[i].leadPodID = previous.playerStates[i].leadPodID;
            playerStates[i].lastPods[0] = previous.playerStates[i].pods[0];
            playerStates[i].lastPods[1] = previous.playerStates[i].pods[1];
            vector<int> passed;
            for (int p = 0; p < POD_COUNT; p++) {
                playerStates[i].pods[p].passedCheckpoints = previous.playerStates[i].pods[p].passedCheckpoints;
                playerStates[i].pods[p].turnsSinceCP = previous.playerStates[i].pods[p].turnsSinceCP;
                playerStates[i].pods[p].turnsSinceShield = previous.playerStates[i].pods[p].turnsSinceShield + 1;
                int previousCP = previous.playerStates[i].pods[p].nextCheckpoint;
                int currentCP = playerStates[i].pods[p].nextCheckpoint;
                int passedCount;
                if (currentCP != previousCP) {
                    // Passed a checkpoint. Progress.
                    playerStates[i].pods[p].turnsSinceCP = 0;
                    passedCount = ++playerStates[i].pods[p].passedCheckpoints;
                } else {
                    passedCount = playerStates[i].pods[p].passedCheckpoints;
                    playerStates[i].pods[p].turnsSinceCP++;
                }
                passed.push_back(passedCount);
            }
            // Update lead pod ID.
            if(passed[0] == passed[1]) {
                PodState& p0 = playerStates[i].pods[0];
                PodState& p1 = playerStates[i].pods[1];
                Vector& cp = race.checkpoints[p0.nextCheckpoint];
                playerStates[i].leadPodID = (cp - p0.pos).getLength() < (cp - p1.pos).getLength() ? 0 : 1;
            } else {
                playerStates[i].leadPodID = passed[0] > passed[1] ? 0 : 1;
            }
        }
    }
    current = GameState(race, playerStates, turn);
    if(turn == 0) {
        for(int i = 0; i < PLAYER_COUNT; i++) {
            for(int j= 0; j < POD_COUNT; j++) {
                current.playerStates[i].pods[j].angle = Physics::angleTo(current.playerStates[i].pods[j].pos, race.checkpoints[1]);
            }
        }
        // Get pod 0 to target enemy 0 so that there are no cross-overs in the path.
        current.ourState().leadPodID = 1;
        current.enemyState().leadPodID = 0;
    }
}

void State::postTurnUpdate(PodOutputAbs pod1, PodOutputAbs pod2) {
    update(pod1, 0);
    update(pod2, 1);
    previous = current;
    turn++;
}

void State::update(const PodOutputAbs& pod, int id) {
    if(pod.thrust == PodOutputAbs::BOOST) {
        current.ourState().pods[id].boostAvailable = false;
    } else if(pod.thrust == PodOutputAbs::SHIELD) {
        current.ourState().pods[id].turnsSinceShield = 0;
    }
}




