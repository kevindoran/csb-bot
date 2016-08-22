#ifndef CODERSSTRIKEBACK_ANNEALINGBOT_H
#define CODERSSTRIKEBACK_ANNEALINGBOT_H

#include <limits>
#include <cstdlib>
#include <chrono>
#include <cmath>

#include "State.h"
#include "Bot.h"
#include "Navigation.h"
#include "Physics.h"


class CustomAI : public SimBot {
    const PairOutput* moves;
    Race* race;
    int turn = 0;
public:
    CustomAI(const PairOutput moves[], int startFromTurn) : moves(moves), turn(startFromTurn) {}

    void init(Race& r) {
        race = &r;
    }
    void move(PodState ourPods[], PodState enemyPods[]) {
        Physics::apply(ourPods, moves[turn++]);
    }
};

/**
 * Bot with very low computational requirements.
 */
class MinimalBot : public SimBot {
    Race race;
    Physics physics;
    Navigation nav;
public:
    MinimalBot() {}

    MinimalBot(Race& race) {
        init(race);
    }

    void init(Race& r) {
        race = r;
        physics = Physics(race);
        nav = Navigation(race);
    }

    /**
     * ourPods and enemyPods must be orderd by progress; it is assumed that ourPods[0] is our lead pod.
     */
    void move(PodState* ourPods, PodState* enemyPods) {
        // Racer
        Vector target = race.checkpoints[ourPods[0].nextCheckpoint] - ourPods[0].vel * 4;
        float turn_angle = physics.turnAngle(ourPods[0], target);
        Vector force;
        if(abs(turn_angle) > MAX_ANGLE) {
            turn_angle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : min(MAX_ANGLE, turn_angle);
            force = Vector::fromMagAngle(MAX_THRUST, turn_angle);
        } else {
            force = target.normalize() * MAX_THRUST;
        }
        ourPods[0].vel.x += force.x;
        ourPods[0].vel.y += force.y;
        ourPods[0].angle += turn_angle;

        // Bouncer
        float targetx =  enemyPods[0].pos.x + (race.checkpoints[enemyPods[0].nextCheckpoint].x - enemyPods[0].pos.x) * 0.80;
        float targety =  enemyPods[0].pos.y + (race.checkpoints[enemyPods[0].nextCheckpoint].y - enemyPods[0].pos.y) * 0.80;
        target = Vector(targetx, targety);
        turn_angle = physics.turnAngle(ourPods[1],target);
        if(abs(turn_angle) > MAX_ANGLE) {
            turn_angle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : min(MAX_ANGLE, turn_angle);
            force = Vector::fromMagAngle(MAX_THRUST, turn_angle);
        } else {
            force = target.normalize() * MAX_THRUST;
        }
        ourPods[1].vel.x += force.x;
        ourPods[1].vel.y += force.y;
        ourPods[1].angle += turn_angle;
    };
};

class AnnealingBot : public DuelBot {
    static const int turns = 6;
    static constexpr float maxScore = 70000;//numeric_limits<float>::infinity();
    static constexpr float minScore = 1000;//-numeric_limits<float>::infinity();
    // Loop control and timing.
    static const int reevalPeriodMilli = 10;
    static const int timeBufferMilli = 1;
    static constexpr float startTemp = 1;
    static constexpr float minTemperature = /*tartTemp*/ 1.0 * pow(0.957, 160);
    static constexpr float K = 0.03;//215.265;//203080;//0.01; // Boltzman's constant.
    static constexpr float stepsVsCoolRatio = 1.6;
    static const int initCoolingSteps = 90;
    static const int initStepsPerTemp = 150;
    static constexpr float initCoolingFraction = 0.916;//0.9413;
    long long startTime;
    static const int UNSET = -1;
    long allocatedTime = UNSET;
    long long lastUpdateTime;
    double diffSum = 0;
    int simCount = 0;
    int tunnelCount = 0;
    int nonTunnelCount = 0;
    int simsSinceUpdate = 0;
    int coolCount = 0;
    int coolingSteps = initCoolingSteps;
    int coolingIdx = 0;
    int stepsPerTemp = initStepsPerTemp;
    float currentTemp = startTemp;
    float coolingFraction = initCoolingFraction;

    Race race;
    Physics physics;
    MinimalBot enemyBot;
    PairOutput previousSolution[turns];
    bool hasPrevious = false;
    PodState enemySimHistory[turns + 1][POD_COUNT];
    PodState ourSimHistory[turns + 1][POD_COUNT];

    void train(const PodState podsToTrain[], const PodState opponentPods[], PairOutput solution[]);

    float score(const PairOutput solution[], int startFromTurn);

    void simulate(SimBot *pods1Sim, SimBot *pods2Sim, int turns, int startFromTurn);

    void randomSolution(PairOutput sol[]);

    PairOutput random();

    void randomEdit(PairOutput &po, int turnsRemaining, float algoProgress);

    long long getTimeMilli() {
        long long ms = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        return ms;
    }

    void updateLoopControl() {
        if(allocatedTime == UNSET) return;
        long long timeNow = getTimeMilli();
        long elapsed = timeNow - lastUpdateTime;
        long timeRemaining = allocatedTime - (timeNow - startTime) - timeBufferMilli;
        if(timeRemaining < 0) {
            coolingSteps = 0;
            stepsPerTemp = 0;
        } else if(elapsed > reevalPeriodMilli) {
            lastUpdateTime = timeNow;
            float simRate = simsSinceUpdate / elapsed;
            int simsRemaining = simRate * timeRemaining;
            // A*2A = C
            // A = sqrt(C/2)
            coolingSteps = sqrt((simCount + simsRemaining) / (stepsVsCoolRatio));
            stepsPerTemp = coolingSteps * stepsVsCoolRatio;
//            coolingFraction = pow(minTemperature/currentTemp, 1.0/(coolingSteps - coolingIdx));
            cerr << "Tunnel %: " << (float)tunnelCount/(tunnelCount + nonTunnelCount) << endl;
            tunnelCount = 0;
            nonTunnelCount = 0;
            simsSinceUpdate = 0;
            cerr << "alpha: " << coolingFraction << endl;
            cerr << "Cooling steps: " << coolingSteps << endl;
//            cerr << "Steps per temp: " << stepsPerTemp << endl;
        }
    }

public:
    AnnealingBot() {}

    AnnealingBot(Race& r) : race(r), physics(race), enemyBot(race) {}

    AnnealingBot(Race& r, long allocatedTimeMilli) : race(r), physics(race),
                                                     enemyBot(race), allocatedTime(allocatedTimeMilli){}

    float score(const PodState* pods[], const PodState* podsPrev[], const PodState* enemyPods[], const PodState* enemyPodsPrev[]);

    void init(Race& r) {
        race = r;
        physics = Physics(race);
        enemyBot = MinimalBot(race);
    }


    PairOutput move(GameState& gameState) {
        startTime = getTimeMilli();
        lastUpdateTime = startTime;
        PodState ourPods[POD_COUNT];
        PodState enemyPods[POD_COUNT];
        memcpy(ourPods, gameState.ourState().pods, sizeof(PodState) * POD_COUNT);
        memcpy(enemyPods, gameState.enemyState().pods, sizeof(PodState) * POD_COUNT);
        bool switched = physics.orderByProgress(ourPods);
        physics.orderByProgress(enemyPods);
        PairOutput solution[turns];
        train(ourPods, enemyPods, solution);
        PairOutput sol = solution[0];
        if(switched) {
            PodOutputSim temp = sol.o1;
            sol.o1 = sol.o2;
            sol.o2 = temp;
        }
        return sol;
    };
};


#endif //CODERSSTRIKEBACKC_ANNEALINGBOT_H
