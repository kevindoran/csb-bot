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
static const int POD_RADIUS = 400;
static const int WANDER_TIMEOUT = 100;
static const int SHIELD_COOLDOWN = 3;
static const int OUR_PLAYER_ID = 0;
static const int BOOST_VELOCITY = 800;

struct Checkpoint {
    Vector pos;
    int order;

    Checkpoint(int x, int y, int order) :
            pos(x, y), order(order) {}
};

struct Race {
    int laps;
    vector<Checkpoint> checkpoints;
    double maxCheckpointDist = 0;

    Race() {}

    Race(int laps, vector<Checkpoint> checkpoints) :
            laps(laps), checkpoints(checkpoints) {
        for(int i = 0; i < checkpoints.size()-1; i++) {
            maxCheckpointDist = max(maxCheckpointDist, (checkpoints[i].pos - checkpoints[i+1].pos).getLength());
        }
    }
};

struct PodState {
    Vector pos;
    Vector vel;
    // In radians
    double angle;
    int nextCheckpoint;
    int passedCheckpoints = 0;
    int turnsSinceCP = 0;
    int turnsSinceShield = 0;
    bool boostAvailable = true;

    PodState(int x, int y, int vx, int vy, double angle, int nextCheckpoint) :
            pos(x, y), vel(vx, vy), angle(angle), nextCheckpoint(nextCheckpoint) { }

    PodState(Vector pos, Vector vel, double angle, int nextCheckpoint) :
            pos(pos), vel(vel), angle(angle), nextCheckpoint(nextCheckpoint) {}

    // Many tests and simulations don't care about the checkpoints.
    PodState(Vector pos, Vector vel, double angle) : pos(pos), vel(vel), angle(angle) {}

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
    vector<PodState> lastPods;
    int id; // Why is this needed?
    int leadPodID = 0;
    PlayerState(int id, vector<PodState> pods) : id(id), pods(pods) {}

    PlayerState(int id, vector<PodState> pods, vector<PodState> lastPods)
            : id(id), pods(pods), lastPods(lastPods) {}

    PodState& leadPod() {
        return pods[leadPodID];
    }

    PodState& laggingPod() {
        return pods[(leadPodID + 1) % 2];
    }
};

struct GameState {
    Race race;
    vector<PlayerState> playerStates;
    int turn = 0;

    GameState() {};

    GameState(Race& race, vector<PlayerState>& playerStates, int turn) :
            race(race), playerStates(playerStates), turn(turn) {}

    PlayerState& ourState() {
        return playerStates[OUR_PLAYER_ID];
    }

    PlayerState& enemyState() {
        return playerStates[(OUR_PLAYER_ID + 1) % 2];
    }
};

struct PodOutput {
    double thrust; // double or int? Game treats it as int, but maybe double might be useful elsewhere.
    Vector target;
    static const int BOOST = -1;
    static const int SHIELD = -2;

    PodOutput() {}

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

    void enableShield() {
        thrust = SHIELD;
    }

    void enableBoost() {
        thrust = BOOST;
    }
};

class State {
    void update(const PodOutput& pod1, int id);
    GameState previous;
    GameState current;
public:
    Race race;
    int turn = 0;

    State(Race race) : race(race) {};
    void preTurnUpdate(vector<PlayerState> input);
    void postTurnUpdate(PodOutput pod1, PodOutput pod2);
    GameState& game() {return current;}
};



#endif //CODERSSTRIKEBACKC_GAMESTATE_H
