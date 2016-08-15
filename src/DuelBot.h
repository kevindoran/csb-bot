#ifndef CODERSSTRIKEBACK_DUELBOT_H
#define CODERSSTRIKEBACK_DUELBOT_H

#include <limits>
#include <cstdlib>

#include "State.h"
#include "Navigation.h"
#include "Physics.h"

struct PodOutputSim {
    int thrust;
    double angle;
    bool shieldEnabled;

    PodOutputSim() {}

    PodOutputSim(int thrust, double angle, bool shieldEnabled) : thrust(thrust), angle(angle),
                                                                 shieldEnabled(shieldEnabled) {}

    PodOutput absolute(const PodState& pod) {
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
        int randomSpeed = rand() % (MAX_THRUST + 1);
        double randomAngle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
        bool shieldEnabled = false;
        PodOutputSim o1(randomSpeed, randomAngle, shieldEnabled);

        randomSpeed = rand() % (MAX_THRUST + 1);
        randomAngle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
        PodOutputSim o2(randomSpeed, randomAngle, shieldEnabled);
        return PairOutput(o1, o2);
    }
};


class SimBot {
public:
    virtual void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) = 0;

    void apply(vector<PodState> &pods, PairOutput control);

    void apply(PodState &pod, PodOutputSim control);

    void apply(PodState &pod, PodOutput control);
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
        PodOutput output = nav.turnSaturationAdjust(ourPods[0], PodOutput(MAX_THRUST, target));
        apply(ourPods[0], output);
        PodOutput bouncerOut = PodOutput(MAX_THRUST, enemyPods[0].pos +
                                                     (race.checkpoints[enemyPods[0].nextCheckpoint].pos -
                                                      enemyPods[0].pos) * 0.80);
        apply(ourPods[1], bouncerOut);
    };
};

class DuelBot {
public:
    /**
     * Calculate the moves for both podracers.
     */
    virtual PairOutput move(const vector<PodState> &ourPods, const vector<PodState> &enemyPods) = 0;
};


class AnnealingBot : public DuelBot {
    Race &race;
    static const int turns = 5;
    static constexpr double maxScore = numeric_limits<double>::infinity();
    static constexpr double minScore = -numeric_limits<double>::infinity();
    Physics physics;

    vector<PairOutput> randomSolution();

    vector<PairOutput> train(const vector<PodState> podsToTrain, const vector<PodState> opponentPods);

    void simulate(vector<PodState> &pods1, SimBot *pods1Sim, vector<PodState> &pods2, SimBot *pods2Sim, int turns);

    double score(vector<PodState> pods, vector<PodState> enemyPods);

    double score(vector<PodState> pods, vector<PairOutput> solution, vector<PodState> enemyPods, MinimalBot race);

public:
    AnnealingBot(Race &race) : race(race), physics(race) {}

    PairOutput move(const vector<PodState> &ourPods, const vector<PodState> &enemyPods) {
        vector<PairOutput> solution = train(ourPods, enemyPods);
        return solution[0];
    };
};


#endif //CODERSSTRIKEBACKC_DUELBOT_H
