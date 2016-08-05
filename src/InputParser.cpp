//
// Created by Kevin on 1/08/2016.
//

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
            PodState* pod = new PodState(x, y, vx, vy, angle, nextCheckpoint);
            podStates.push_back(*pod);
        }
        PlayerState* ps = new PlayerState(i, podStates);
        playerStates.push_back(*ps);
    }
    const int OUR_ID = 0; // Possible move this to race if it is indeed true.
    GameState gs = GameState(*race, playerStates, OUR_ID, turn++);
    return gs;
}