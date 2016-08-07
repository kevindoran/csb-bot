//
// Created by Kevin on 1/08/2016.
//
#define _USE_MATH_DEFINES
#include <cmath>

#include "InputParser.h"
#include "State.h"

#include <string>

using namespace std;

void InputParser::init() {
    int laps, checkpointCount;
    stream >> laps >> checkpointCount;
    vector<Checkpoint> checkpoints;
    for(int i = 0; i < checkpointCount; i++) {
        int x, y;
        stream >> x >> y;
        checkpoints.push_back(Checkpoint(x, y, i));
    }
    race = new Race(laps, checkpoints);
}

GameState InputParser::parseGameState() {
    vector<PlayerState> playerStates;
    for(int i = 0; i < PLAYER_COUNT; i++) {
        vector<PodState> podStates;
        for(int p = 0; p < POD_COUNT; p++) {
            int x, y, vx, vy, angle, nextCheckpoint;
            stream >> x >> y >> vx >> vy >> angle >> nextCheckpoint;
            double angleRad =M_PI * (angle / 180.0);
            PodState* pod = new PodState(x, y, vx, vy, angleRad, nextCheckpoint);
            podStates.push_back(*pod);
        }
        PlayerState* ps = new PlayerState(i, podStates);
        playerStates.push_back(*ps);
    }
    const int OUR_ID = 0; // Possible move this to race if it is indeed true.
    GameState gs = GameState(*race, playerStates, OUR_ID, turn++);
    // Update info about lead and lagging pods of each player. Useful for choosing which steering
    // strategy to use on each pod and to be able to attack the leading enemy pod.
    cerr << "yoyo" << endl;
    if(isPreviousState) {
        for(int i = 0; i < PLAYER_COUNT; i++) {
            PlayerState playerState = gs.playerStates[i];
            playerState.leadPodID = previous.playerStates[i].leadPodID;
            for(int p = 0; p < POD_COUNT; p++) {
                playerState.pods[p].passedCheckpoints = previous.playerStates[i].pods[p].passedCheckpoints;
                playerState.pods[p].passedCheckpoints = previous.playerStates[i].pods[p].turnsSinceCP;
                int previousCP = previous.playerStates[i].pods[p].nextCheckpoint;
                int currentCP = playerState.pods[p].nextCheckpoint;
                if(currentCP != previousCP) {
                    // Passed a checkpoint. Progress.
                    int passed = ++playerState.pods[p].passedCheckpoints;
                    cerr << "Passed: " << passed << endl;
                    if(passed > playerState.leadPod().passedCheckpoints) {
                        playerState.leadPodID = p;
                    } else {
                        playerState.pods[p].turnsSinceCP++;
                    }
                }
            }
        }
    }
    isPreviousState = true;
    previous = gs;
    return gs;
}