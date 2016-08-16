#ifndef CODERSSTRIKEBACK_DUELBOT_H
#define CODERSSTRIKEBACK_DUELBOT_H

#include <limits>
#include <cstdlib>

#include "State.h"
#include "Navigation.h"
#include "Physics.h"

struct PodOutputSim {
    int thrust;
    float angle;
    bool shieldEnabled;

    PodOutputSim() {}

    PodOutputSim(int thrust, float angle, bool shieldEnabled) : thrust(thrust), angle(angle),
                                                                 shieldEnabled(shieldEnabled) {}

    PodOutput absolute(const PodState& pod) {
        cerr << "Pod pos: "  << pod.pos << endl;
        Vector target = pod.pos + Vector::fromMagAngle(1000, pod.angle + angle);
        return PodOutput(thrust, target);
    }
};

class PairOutput {
public:
    PodOutputSim o1;
    PodOutputSim o2;

    PairOutput() {}

    PairOutput(PodOutputSim o1, PodOutputSim o2) : o1(o1), o2(o2) {}

    static PairOutput random() {
        //int randomSpeed = rand() % (MAX_THRUST + 1);
        int randomSpeed = ((float)rand() / RAND_MAX) > 0.5 ? 0 : MAX_THRUST;
        float randomAngle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
        bool shieldEnabled = false;
        PodOutputSim o1(randomSpeed, randomAngle, shieldEnabled);

//        randomSpeed = rand() % (MAX_THRUST + 1);
        randomSpeed = ((float)rand() / RAND_MAX) > 0.5 ? 0 : MAX_THRUST;
        randomAngle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
        PodOutputSim o2(randomSpeed, randomAngle, shieldEnabled);
        return PairOutput(o1, o2);
    }
};


class SimBot {
public:
    virtual void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) = 0;

    void apply(vector<PodState> &pods, PairOutput& control);

    void apply(PodState &pod, PodOutputSim& control);

    void apply(PodState &pod, PodOutput& control);
};

class CustomAI : public SimBot {
    vector<PairOutput> moves;
    int turn = 0;
public:
    CustomAI(vector<PairOutput> moves) : moves(moves) {}

    void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) {
        apply(ourPods, moves[turn++]);
    }
};

/**
 * Bot with very low computational requirements.
 */
class MinimalBot : public SimBot {
    Race &race;
    Physics physics;
    Navigation nav;
public:
    MinimalBot(Race &race) : race(race), physics(race), nav(race) {}

    /**
     * ourPods and enemyPods must be orderd by progress; it is assumed that ourPods[0] is our lead pod.
     */
    void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) {
        Vector target = race.checkpoints[ourPods[0].nextCheckpoint].pos - ourPods[0].vel * 4;
//        float cut_off = M_PI / 2 + MAX_ANGLE;
//        float angle_threshold = M_PI * (20.0f / 180.0f);
        float turn_angle = physics.turnAngle(ourPods[0], target);
//        float thrust;
//        float clippedAngle;
//        if(abs(turn_angle) > cut_off) {
//            thrust = 0;
//        } else if(abs(turn_angle) > angle_threshold) {
//            float spread = 0.3;
//            float x = abs(spread * (turn_angle - angle_threshold) / (cut_off - angle_threshold));
//            float factor =  1 - sqrt(1 - exp(-(2 / M_PI) * x * x));
//            thrust *= factor;
//        }
        float clippedAngle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : max(MAX_ANGLE, turn_angle);
        Vector force = Vector::fromMagAngle(MAX_THRUST, clippedAngle);
        ourPods[0].vel.x += force.x;
        ourPods[0].vel.y += force.y;
        ourPods[0].angle += clippedAngle;
//        PodOutput output = nav.turnSaturationAdjust(ourPods[0], PodOutput(MAX_THRUST, target));

        float targetx =  enemyPods[0].pos.x + (race.checkpoints[enemyPods[0].nextCheckpoint].pos.x - enemyPods[0].pos.x) * 0.80;
        float targety =  enemyPods[0].pos.y + (race.checkpoints[enemyPods[0].nextCheckpoint].pos.y - enemyPods[0].pos.y) * 0.80;
        target = Vector(targetx, targety);
        turn_angle = physics.turnAngle(ourPods[1],target);
        clippedAngle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : max(MAX_ANGLE, turn_angle);
        force = Vector::fromMagAngle(MAX_THRUST, clippedAngle);
        ourPods[1].vel.x += force.x;
        ourPods[1].vel.y += force.y;
        ourPods[1].angle += clippedAngle;
    };
};

class DuelBot {
public:
    virtual void init(Race& race) = 0;
    /**
     * Calculate the moves for both podracers.
     */
    virtual PairOutput move(const vector<PodState> &ourPods, const vector<PodState> &enemyPods) = 0;
};


class AnnealingBot : public DuelBot {
    Race &race;
    static const int turns = 5;
    static constexpr float maxScore = numeric_limits<float>::infinity();
    static constexpr float minScore = -numeric_limits<float>::infinity();
    Physics physics;

    vector<PairOutput> randomSolution();

    vector<PairOutput> train(const vector<PodState> &podsToTrain, const vector<PodState> &opponentPods);

    void simulate(vector<PodState> &pods1, SimBot *pods1Sim, vector<PodState> &pods2, SimBot *pods2Sim, int turns);

    float score(vector<PodState> &pods, vector<PodState> &enemyPods);

public:
    AnnealingBot(Race &race) : race(race), physics(race) {}

    PairOutput move(const vector<PodState> &ourPods, const vector<PodState> &enemyPods) {
        vector<PairOutput> solution = train(ourPods, enemyPods);
        return solution[0];
    };



    float score(vector<PodState> pods, vector<PairOutput> solution, vector<PodState> enemyPods);
};


#endif //CODERSSTRIKEBACKC_DUELBOT_H
