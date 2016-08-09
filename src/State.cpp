
#include "State.h"

void State::preTurnUpdate(vector<PlayerState> playerStates) {
    if(turn > 0) {
        for (int i = 0; i < PLAYER_COUNT; i++) {
            playerStates[i].leadPodID = previous.playerStates[i].leadPodID;
            playerStates[i].lastPods = previous.playerStates[i].pods;
            vector<int> passed;
            for (int p = 0; p < POD_COUNT; p++) {
                playerStates[i].pods[p].passedCheckpoints = previous.playerStates[i].pods[p].passedCheckpoints;
                playerStates[i].pods[p].turnsSinceCP = previous.playerStates[i].pods[p].turnsSinceCP;
                playerStates[i].pods[p].turnsSinceShield = previous.playerStates[i].pods[p].turnsSinceShield + 1;
                playerStates[i].pods[p].boostAvailable = previous.playerStates[i].pods[p].boostAvailable;
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
                Checkpoint& cp = race.checkpoints[p0.nextCheckpoint];
                playerStates[i].leadPodID = (cp.pos - p0.pos).getLength() < (cp.pos - p1.pos).getLength() ? 0 : 1;
            } else {
                playerStates[i].leadPodID = passed[0] > passed[1] ? 0 : 1;
            }
        }
    }
    current = GameState(race, playerStates, turn);
    if(turn == 0) {
        // Get pod 0 to target enemy 0 so that there are no cross-overs in the path.
        current.ourState().leadPodID = 1;
        current.enemyState().leadPodID = 0;
    }
}

void State::postTurnUpdate(PodOutput pod1, PodOutput pod2) {
    update(pod1, 0);
    update(pod2, 1);
    previous = current;
    turn++;
}

void State::update(const PodOutput& pod, int id) {
    if(pod.thrust == PodOutput::BOOST) {
        current.ourState().pods[id].boostAvailable = false;
    } else if(pod.thrust == PodOutput::SHIELD) {
        current.ourState().pods[id].turnsSinceShield = 0;
    }
}




