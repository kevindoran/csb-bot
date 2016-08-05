//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_GAMESTATE_H
#define CODERSSTRIKEBACKC_GAMESTATE_H

#include <vector>
#include <string>
#include <sstream>
#include "Vector.h"

using namespace std;


static const int MAX_THRUST = 200;
static const int MAX_ANGLE = 18;
static constexpr double DRAG = 0.85;
static const int POD_COUNT = 2;
static const int PLAYER_COUNT = 2;
static const int CHECKPOINT_WIDTH = 600;

struct Checkpoint {
    const Vector pos;
    const int order;

    Checkpoint(int x, int y, int order) :
            pos(x, y), order(order) {}
};

struct Race {
    const int laps;
    const vector<Checkpoint> checkpoints;
//    const int CHECKPOINT_COUNT;

    Race(int laps, vector<Checkpoint> checkpoints) :
            laps(laps), checkpoints(checkpoints) {};
};

struct PodState {
    const Vector pos;
    const Vector vel;
    // In radians
    const int angle;
    const int nextCheckpoint;

    PodState(int x, int y, int vx, int vy, int angle, int nextCheckpoint) :
            pos(x, y), vel(vx, vy), angle(angle), nextCheckpoint(nextCheckpoint) { }

    PodState(Vector pos, Vector vel, int angle, int nextCheckpoint) :
            pos(pos), vel(vel), angle(angle), nextCheckpoint(nextCheckpoint) {}

    bool operator ==(const PodState& other) const {
        return (pos == other.pos && vel == other.vel && angle == other.angle && nextCheckpoint == other.nextCheckpoint);
    }

    bool operator !=(const PodState& other) const {
        return !(*this == other);
    }
};

struct PlayerState {
//    static const int POD_COUNT = 2;
    vector<PodState> pods;
    const int id; // Why is this needed?
    PlayerState(int id, vector<PodState> pods) : id(id), pods(pods) {}
};

struct GameState {
    const Race race;
    const vector<PlayerState> playerStates;
    const int ourPlayerId;
    const int turn;

    GameState(Race race, vector<PlayerState>& playerStates, int ourPlayerId, int turn) :
            race(race), playerStates(playerStates), ourPlayerId(ourPlayerId), turn(turn) {}

    PlayerState ourState() {
        return playerStates[ourPlayerId];
    }
};

struct PodOutput {
    const int thrust;
    const Vector dir;
    static const int BOOST = -1;
    static const int SHIELD = -2;

    PodOutput(int thrust, Vector direction) :
            thrust(thrust), dir(direction) {}

    string toString() {
        stringstream out;
        string thrustStr;
        if(thrust == BOOST) {
            thrustStr = "BOOST";
        } else if(thrust == SHIELD){
            thrustStr = "SHIELD";
        } else {
            thrustStr = to_string((int)thrust);
        }
        out << (int) dir.x << " " << (int) dir.y << " " << thrustStr;
        return out.str();
    }
};



#endif //CODERSSTRIKEBACKC_GAMESTATE_H
