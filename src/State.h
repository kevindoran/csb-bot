//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_GAMESTATE_H
#define CODERSSTRIKEBACKC_GAMESTATE_H


#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
#include "Vector.h"

using namespace std;


static const int MAX_THRUST = 100;
static const int MAX_ANGLE_DEG = 15;
static constexpr double MAX_ANGLE = M_PI * MAX_ANGLE_DEG / 180;
static constexpr double DRAG = 0.85;
static const int POD_COUNT = 2;
static const int PLAYER_COUNT = 2;
static const int CHECKPOINT_RADIUS = 600;

struct Checkpoint {
    Vector pos;
    int order;

    Checkpoint(int x, int y, int order) :
            pos(x, y), order(order) {}
};

struct Race {
    int laps;
    vector<Checkpoint> checkpoints;
//    const int CHECKPOINT_COUNT;

    Race(int laps, vector<Checkpoint> checkpoints) :
            laps(laps), checkpoints(checkpoints) {};
};

struct PodState {
    Vector pos;
    Vector vel;
    // In radians
    double angle;
    int nextCheckpoint;

    PodState(int x, int y, int vx, int vy, double angle, int nextCheckpoint) :
            pos(x, y), vel(vx, vy), angle(angle), nextCheckpoint(nextCheckpoint) { }

    PodState(Vector pos, Vector vel, double angle, int nextCheckpoint) :
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
    Race race;
    vector<PlayerState> playerStates;
    int ourPlayerId;
    int turn;

    GameState(Race race, vector<PlayerState>& playerStates, int ourPlayerId, int turn) :
            race(race), playerStates(playerStates), ourPlayerId(ourPlayerId), turn(turn) {}

    PlayerState ourState() {
        return playerStates[ourPlayerId];
    }
};

struct PodOutput {
    double thrust; // double or int? Game treats it as int, but maybe double might be useful elsewhere.
    Vector target;
    static const int BOOST = -1;
    static const int SHIELD = -2;

    PodOutput(double thrust, Vector direction) :
            thrust(thrust), target(direction) {}

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
        out << (int) target.x << " " << (int) target.y << " " << thrustStr;
        return out.str();
    }
};



#endif //CODERSSTRIKEBACKC_GAMESTATE_H
