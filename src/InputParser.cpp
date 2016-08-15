#define _USE_MATH_DEFINES
#include <cmath>

#include "InputParser.h"
#include "State.h"

#include <string>

using namespace std;

Race InputParser::init() {
    int laps, checkpointCount;
    stream >> laps >> checkpointCount;
    vector<Checkpoint> checkpoints;
    for(int i = 0; i < checkpointCount; i++) {
        int x, y;
        stream >> x >> y;
        checkpoints.push_back(Checkpoint(x, y, i));
    }
    return Race(laps, checkpoints);
}

vector<PlayerState> InputParser::parseTurn() {
    vector<PlayerState> playerStates;
    for (int i = 0; i < PLAYER_COUNT; i++) {
        vector<PodState> podStates;
        for (int p = 0; p < POD_COUNT; p++) {
            int x, y, vx, vy, angle, nextCheckpoint;
            stream >> x >> y >> vx >> vy >> angle >> nextCheckpoint;
            float angleRad = M_PI * (angle / 180.0);
            PodState pod(x, y, vx, vy, angleRad, nextCheckpoint);
            podStates.push_back(pod);
        }
        PlayerState ps(i, podStates);
        playerStates.push_back(ps);
    }
    return playerStates;
}
