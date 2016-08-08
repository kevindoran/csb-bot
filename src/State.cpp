
#include "State.h"

void State::preTurnUpdate(vector<PlayerState> playerStates) {
    if(turn > 0) {
        for (int i = 0; i < PLAYER_COUNT; i++) {
            playerStates[i].leadPodID = previous.playerStates[i].leadPodID;
            playerStates[i].lastPods = previous.playerStates[i].pods;
            for (int p = 0; p < POD_COUNT; p++) {
                playerStates[i].pods[p].passedCheckpoints = previous.playerStates[i].pods[p].passedCheckpoints;
                playerStates[i].pods[p].turnsSinceCP = previous.playerStates[i].pods[p].turnsSinceCP;
                playerStates[i].pods[p].turnsSinceShield = previous.playerStates[i].pods[p].turnsSinceShield + 1;
                int previousCP = previous.playerStates[i].pods[p].nextCheckpoint;
                int currentCP = playerStates[i].pods[p].nextCheckpoint;
                if (currentCP != previousCP) {
                    // Passed a checkpoint. Progress.
                    playerStates[i].pods[p].turnsSinceCP = 0;
                    int passed = ++playerStates[i].pods[p].passedCheckpoints;
                    // The current playerStates are fully update until the end of the double for-loop, so we must
                    // compare to the previous checkpoint counts to avoid errors.
                    if (passed > previous.playerStates[i].leadPod().passedCheckpoints) {
                        playerStates[i].leadPodID = p;
                    }
                } else {
                    playerStates[i].pods[p].turnsSinceCP++;
                }
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




