#define _USE_MATH_DEFINES
#include <cmath>

#include "InputParser.h"
#include "State.h"

#include <string>

using namespace std;

Race InputParser::init() {
    int laps, checkpointCount;
    stream >> laps >> checkpointCount;
    vector<Vector> checkpoints;
    for(int i = 0; i < checkpointCount; i++) {
        int x, y;
        stream >> x >> y;
        checkpoints.push_back(Vector(x, y));
    }
    return Race(laps, checkpoints);
}

void InputParser::parseTurn(PlayerState playerStates[]) {
    for (int i = 0; i < PLAYER_COUNT; i++) {
        PodState podStates[POD_COUNT];
        for (int p = 0; p < POD_COUNT; p++) {
            int x, y, vx, vy, angle, nextCheckpoint;
            stream >> x >> y >> vx >> vy >> angle >> nextCheckpoint;
            float angleRad = M_PI * (angle / 180.0);
            PodState pod(x, y, vx, vy, angleRad, nextCheckpoint);
            podStates[p] = pod;
        }
        PlayerState ps(podStates);
        playerStates[i] = ps;
    }
}
