#ifndef CODERSSTRIKEBACK_GAMESTATE_H
#define CODERSSTRIKEBACK_GAMESTATE_H


#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>

#include "Vector.h"

using namespace std;


static const int MAX_THRUST = 200;
static const int MAX_ANGLE_DEG = 18;
static constexpr float MAX_ANGLE = M_PI * MAX_ANGLE_DEG / 180;
static constexpr float DRAG = 0.85f;
static const int POD_COUNT = 2;
static const int PLAYER_COUNT = 2;
static const int CHECKPOINT_RADIUS = 600;
static const int POD_RADIUS = 400;
static const int POD_RADIUS_SQ = 400 * 400;
static const int WANDER_TIMEOUT = 100;
static const int SHIELD_COOLDOWN = 3;
static const int OUR_PLAYER_ID = 0;
static const int BOOST_ACC = 650;


class Race {
public:
    int laps;
    vector<Vector> checkpoints;
    vector<float> nextCPDistaces;
    vector<float> previousCPDistances;
    float maxCheckpointDist = 0;

    Race() {}

    Race(int laps, vector<Vector> checkpoints) :
            laps(laps), checkpoints(checkpoints) {
        previousCPDistances.push_back(0);
        for(int i = 0; i < checkpoints.size()-1; i++) {
            nextCPDistaces.push_back((checkpoints[i] - checkpoints[i+1]).getLength());
            previousCPDistances.push_back(nextCPDistaces[i]);
            maxCheckpointDist = max(maxCheckpointDist, nextCPDistaces[i]);
        }
        nextCPDistaces.push_back((checkpoints[checkpoints.size()-1] - checkpoints[0]).getLength());
        previousCPDistances[0] = nextCPDistaces[checkpoints.size()-1];
        maxCheckpointDist = max(maxCheckpointDist, nextCPDistaces[checkpoints.size()-1]);
    }

    float distToNextCP(int fromCP) {
        return nextCPDistaces[fromCP];
    }

    float distFromPrevCP(int toCP) {
        return previousCPDistances[toCP];
    }

    int totalCPCount() {
        return laps * checkpoints.size();
    }

    int followingCheckpoint(int cp) {
        return (cp + 1) % checkpoints.size();
    }
};

struct PodState {
    Vector pos;
    Vector vel;
    // In radians
    float angle = 0;
    bool shieldEnabled = false;
    int nextCheckpoint = 1;
    int passedCheckpoints = 0;
    int turnsSinceCP = 0;
    int turnsSinceShield = SHIELD_COOLDOWN + 1;
    bool boostAvailable = true;

    PodState() {}

    PodState(int x, int y, int vx, int vy, float angle, int nextCheckpoint) :
            pos(x, y), vel(vx, vy), angle(angle), nextCheckpoint(nextCheckpoint) { }

    PodState(Vector pos, Vector vel, float angle, int nextCheckpoint) :
            pos(pos), vel(vel), angle(angle), nextCheckpoint(nextCheckpoint) {}

    // Many tests and simulations don't care about the checkpoints.
    PodState(Vector pos, Vector vel, float angle) : pos(pos), vel(vel), angle(angle) {}

    bool operator ==(const PodState& other) const {
        return (pos == other.pos && vel == other.vel && angle == other.angle && nextCheckpoint == other.nextCheckpoint);
    }

    bool operator !=(const PodState& other) const {
        return !(*this == other);
    }

    void addAngle(float angleRadians) {
        angle += angleRadians;
        if(angle >= 2*M_PI) angle -= 2*M_PI;
        else if(angle < 0) angle += 2*M_PI;
    }
};

struct PlayerState {
    PodState pods[POD_COUNT];
    PodState lastPods[POD_COUNT];
    int leadPodID = 0;

    PlayerState() {}

    PlayerState(PodState inPods[]) {
        memcpy(pods, inPods, POD_COUNT*sizeof(PodState));
    }

    PlayerState(PodState inPods[], PodState inLastPods[]) {
        memcpy(pods, inPods, POD_COUNT*sizeof(PodState));
        memcpy(lastPods, inLastPods, POD_COUNT*sizeof(PodState));
    }

    PodState& leadPod() {
        return pods[leadPodID];
    }

    PodState& laggingPod() {
        return pods[(leadPodID + 1) % 2];
    }
};

struct GameState {
    Race race;
    PlayerState playerStates[PLAYER_COUNT];
    int turn = 0;

    GameState() {};

    GameState(Race& race, PlayerState inPlayerStates[], int turn) :
            race(race), turn(turn) {
        memcpy(playerStates, inPlayerStates, PLAYER_COUNT*sizeof(PlayerState));
    }

    PlayerState& ourState() {
        return playerStates[OUR_PLAYER_ID];
    }

    PlayerState& enemyState() {
        return playerStates[(OUR_PLAYER_ID + 1) % 2];
    }
};


struct PodOutputAbs {
    float thrust;
    Vector target;
    static const int BOOST = -1;
    static const int SHIELD = -2;

    PodOutputAbs() {}

    PodOutputAbs(float thrust, Vector direction) :
            thrust(thrust), target(direction) {}

    string toString() {
        stringstream out;
        string thrustStr;
        if(thrust == BOOST) {
            thrustStr = "BOOST";
        } else if(thrust == SHIELD){
            thrustStr = "SHIELD";
        } else {
            thrustStr = to_string((int)round(thrust));
        }
        out << (int) round(target.x) << " " << (int) round(target.y) << " " << thrustStr;
        return out.str();
    }

    void enableShield() {
        thrust = SHIELD;
    }

    void enableBoost() {
        thrust = BOOST;
    }
};


class PodOutputSim {
public:
    int thrust = 0;
    float angle = 0;
    bool shieldEnabled = false;
    bool boostEnabled = false;

    PodOutputSim() {}

    PodOutputSim(int thrust, float angle, bool shieldEnabled, bool boostEnabled) :
            thrust(thrust), angle(angle), shieldEnabled(shieldEnabled), boostEnabled(boostEnabled) {}

    static PodOutputSim fromAbsolute(const PodState &pod, const PodOutputAbs &abs);

    PodOutputAbs absolute(const PodState &pod);
};

struct PairOutput {
    PodOutputSim o1;
    PodOutputSim o2;

    PairOutput() {}
    PairOutput(PodOutputSim o1, PodOutputSim o2) : o1(o1), o2(o2) {}
};

class State {
    void update(const PodOutputAbs& pod1, int id);
    GameState previous;
    GameState current;
public:
    Race race;
    int turn = 0;

    State() {}
    State(Race race) : race(race) {}
    void preTurnUpdate(PlayerState input[]);
    void postTurnUpdate(PodOutputAbs pod1, PodOutputAbs pod2);
    GameState& game() {return current;}
};

#endif //CODERSSTRIKEBACK_GAMESTATE_H
