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


struct ScoreFactors {
    // Racer
    float overallRacer;
    float passCPBonus;
    float distToCP;
    float enemyProgress;
    // Bouncer
    float overallBouncer;
    float enemyDist;
    float enemyDistToCP;
    float bouncerDistToCP;
    float angleSeenByCP;
    float angleSeenByEnemy;
    float bouncerTurnAngle;
    float enemyTurnAngle;
    float checkpointPenalty;
};

static ScoreFactors defaultFactors = {
        1,    // overallRacer
        3700, // passCPBonus
        1,    // distToCP
        -0.3, // enemyProgress
        1,    // overallBouncer
        -0.4, // enemyDist
        0.8, // enemyDistToCP
        -1,  // bouncerDistToCP
        -0.1,  // angleSeenByCP
        -0,  // angleSeenByEnemy
        -0,  // bouncerTurnAngle
        -0,  // enemyTurn angle
        2000,
};


class CustomAI : public SimBot {
    const PairOutput* moves;
    Race& race;
    int turn = 0;
    int defaultAfter = -1;
public:
    CustomAI(Race& race, const PairOutput moves[], int startFromTurn) :
            race(race), moves(moves), turn(startFromTurn) {}

    void setTurn(int fromTurn) {
        turn = fromTurn;
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
        Vector drift = ourPods[0].vel * 3.5;
        Vector target = race.checkpoints[ourPods[0].nextCheckpoint] - drift;
        float turnAngle = physics.turnAngle(ourPods[0], target);
        Vector force;
        static constexpr float angleThreshold = MAX_ANGLE;
        static constexpr float cutOff = M_PI/2 + MAX_ANGLE;
        if(abs(turnAngle) > MAX_ANGLE) {
            float thrust = MAX_THRUST - int(((turnAngle - angleThreshold) / (cutOff - angleThreshold)) * MAX_THRUST);
            turnAngle = turnAngle < 0 ? max(-MAX_ANGLE, turnAngle) : min(MAX_ANGLE, turnAngle);
            force = Vector::fromMagAngle(thrust, turnAngle);
        } else {
            force = target.normalize() * MAX_THRUST;
        }
        ourPods[0].vel.x += force.x;
        ourPods[0].vel.y += force.y;
        ourPods[0].angle += turnAngle;

        // Bouncer
        float targetx;
        float targety;
        if((target.x - ourPods[1].pos.x)*(target.x - ourPods[1].pos.x) + (target.y - ourPods[1].pos.y) * (target.y - ourPods[1].pos.y) >
                (target.x - enemyPods[0].pos.x)*(target.x - enemyPods[0].pos.x) + (target.y - enemyPods[0].pos.y)*(target.y - enemyPods[0].pos.y)) {
            int nextNextCPID = race.followingCheckpoint(enemyPods[0].nextCheckpoint);
            targetx = race.checkpoints[nextNextCPID].x;
            targety = race.checkpoints[nextNextCPID].y;
        }
        else {
            targetx = enemyPods[0].pos.x + (race.checkpoints[enemyPods[0].nextCheckpoint].x - enemyPods[0].pos.x) * 0.80;
            targety = enemyPods[0].pos.y + (race.checkpoints[enemyPods[0].nextCheckpoint].y - enemyPods[0].pos.y) * 0.80;
        }
        target = Vector(targetx, targety)- drift;
        turnAngle = physics.turnAngle(ourPods[1],target);
        if(abs(turnAngle) > MAX_ANGLE) {
            float thrust = MAX_THRUST - int(((turnAngle - angleThreshold) / (cutOff - angleThreshold)) * MAX_THRUST);
            turnAngle = turnAngle < 0 ? max(-MAX_ANGLE, turnAngle) : min(MAX_ANGLE, turnAngle);
            force = Vector::fromMagAngle(thrust, turnAngle);
        } else {
            force = target.normalize() * MAX_THRUST;
        }
        ourPods[1].vel.x += force.x;
        ourPods[1].vel.y += force.y;
        ourPods[1].angle += turnAngle;
    };
};

class CustomAIWithBackup : public SimBot {
    const PairOutput* moves;
    Race& race;
    MinimalBot backup;
    int turn;
    int defaultAfter = -1;
public:
    CustomAIWithBackup(Race& race, const PairOutput moves[], int startFromTurn):
            race(race), moves(moves), backup(race), turn(startFromTurn) {}

    void setTurn(int fromTurn) {
        turn = fromTurn;
    }

    void setDefaultAfter(int turn) {
        defaultAfter = turn;
    }

    void move(PodState ourPods[], PodState enemyPods[]) {
        if(defaultAfter != -1 && turn >= defaultAfter) {
            backup.move(ourPods, enemyPods);
        } else {
            Physics::apply(ourPods, moves[turn++]);
        }
    }
};

class AnnealingBot : public DuelBot {
public:
    static const int TURNS = 6;
    ScoreFactors sFactors = defaultFactors;
private:
    static constexpr float maxScore = 70000;//numeric_limits<float>::infinity();
    static constexpr float minScore = 1000;//-numeric_limits<float>::infinity();
    // Loop control and timing.
    static const int reevalPeriodMilli = 10;
    static const int timeBufferMilli = 1;
    static constexpr float startTemp = 1;
    static constexpr float minTemperature = 1.0 * pow(0.945, 160);
    static constexpr float K = 0.1;//215.265;//203080;//0.01; // Boltzman's constant.
    static constexpr float stepsVsCoolRatio = 1.6;
    static const int initCoolingSteps = 90;
    static const int initStepsPerTemp = 150;
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
    float coolingFraction = UNSET;//initCoolingFraction;
    bool toDeleteEnemy = false;

    Race race;
    Physics physics;
    SimBot* enemyBot;
    PairOutput previousSolution[TURNS];
    bool hasPrevious = false;
    PodState enemySimHistory[TURNS + 1][POD_COUNT];
    PodState ourSimHistory[TURNS + 1][POD_COUNT];

    void _train(const PodState podsToTrain[], const PodState opponentPods[], PairOutput solution[]);

    float score(const PairOutput solution[], int startFromTurn);

    void simulate(SimBot *pods1Sim, SimBot *pods2Sim, int turns, int startFromTurn);

    void randomSolution(PairOutput sol[]);

    PairOutput random();

    void randomEdit(PairOutput &po, int turnsRemaining, float algoProgress);

    long long getTimeMilli() {
        long long ms = chrono::duration_cast<chrono::milliseconds>(
                chrono::system_clock::now().time_since_epoch()).count();
        return ms;
    }

    void updateLoopControl() {
        if (allocatedTime == UNSET) return;
        long long timeNow = getTimeMilli();
        long elapsed = timeNow - lastUpdateTime;
        long timeRemaining = allocatedTime - (timeNow - startTime) - timeBufferMilli;
        if (timeRemaining < 0) {
            coolingSteps = 0;
            stepsPerTemp = 0;
        } else if (elapsed > reevalPeriodMilli) {
            lastUpdateTime = timeNow;
            float simRate = simsSinceUpdate / elapsed;
            int simsRemaining = simRate * timeRemaining;
            // A*2A = C
            // A = sqrt(C/2)
            coolingSteps = sqrt((simCount + simsRemaining) / (stepsVsCoolRatio));
            stepsPerTemp = coolingSteps * stepsVsCoolRatio;
//            coolingFraction = pow(minTemperature/currentTemp, 1.0/(coolingSteps - coolingIdx));
            cerr << "Tunnel %: " << (float) tunnelCount / (tunnelCount + nonTunnelCount) << endl;
            tunnelCount = 0;
            nonTunnelCount = 0;
            simsSinceUpdate = 0;
            cerr << "alpha: " << coolingFraction << endl;
            cerr << "Cooling steps: " << coolingSteps << endl;
//            cerr << "Steps per temp: " << stepsPerTemp << endl;
        }
    }

    void init() {
        coolingSteps = allocatedTime == UNSET ? 160 : allocatedTime * 1.2;
        coolingFraction = pow(minTemperature/currentTemp, 1.0/coolingSteps);
    }

public:
    AnnealingBot() {
        init();
    }

    AnnealingBot(Race &r) : race(r), physics(race) {
        init();
        enemyBot = new MinimalBot(race);
        toDeleteEnemy = true;
    }

    AnnealingBot(Race &r, long allocatedTimeMilli) : race(r), physics(race), allocatedTime(allocatedTimeMilli) {
        init();
        enemyBot = new MinimalBot(race);
        toDeleteEnemy = true;
    }

    AnnealingBot(Race &r, long allocatedTimeMilli, SimBot* enemyBot) :
            race(r), physics(race), allocatedTime(allocatedTimeMilli), enemyBot(enemyBot) {
        init();
    }

    ~AnnealingBot() {
        if(toDeleteEnemy) {
            delete (enemyBot);
        }
    }

    void setInnitialSolution(PairOutput po[]) {
        memcpy(previousSolution, po, sizeof(PairOutput) * TURNS);
        hasPrevious = true;
    }

    float score(const PodState *pods[], const PodState *podsPrev[], const PodState *enemyPods[],
                const PodState *enemyPodsPrev[]);

    void train(const PodState pods[], const PodState enemyPods[], PairOutput solution[]) {
        startTime = getTimeMilli();
        lastUpdateTime = startTime;
        PodState ourPodsCopy[POD_COUNT];
        PodState enemyPodsCopy[POD_COUNT];
        memcpy(ourPodsCopy, pods, sizeof(PodState) * POD_COUNT);
        memcpy(enemyPodsCopy, enemyPods, sizeof(PodState) * POD_COUNT);
        bool switched = physics.orderByProgress(ourPodsCopy);
        physics.orderByProgress(enemyPodsCopy);
        _train(ourPodsCopy, enemyPodsCopy, solution);
        if (switched) {
            PodOutputSim temp;
            for(int i = 0; i < TURNS; i++) {
                temp = solution[i].o1;
                solution[i].o1 = solution[i].o2;
                solution[i].o2 = temp;
            }
        }
    }

    PairOutput move(GameState& gameState) {
        PairOutput solution[TURNS];
        if(gameState.turn == 0) {
            gameState.ourState().pods[0].vel += (race.checkpoints[1] - gameState.ourState().pods[0].pos).normalize() * 550;
            gameState.ourState().pods[1].vel += (race.checkpoints[1] - gameState.ourState().pods[1].pos).normalize() * 500;
        }
        train(gameState.ourState().pods, gameState.enemyState().pods, solution);
        if(gameState.turn == 0) {
            solution[0].o1.boostEnabled = true;
            solution[0].o1.shieldEnabled = false;
            solution[0].o2.boostEnabled = true;
            solution[0].o2.shieldEnabled = false;
        }
        return solution[0];
    }

    float progress(PodState *pod, PodState *previous);

    float engagedScore(const PodState *bouncer, const PodState *target);

    float engagedScore(const PodState *bouncer, const PodState *target, const PodState *targetPrev);

    float progress(const PodState *pod, const PodState *previous);
};


#endif //CODERSSTRIKEBACKC_ANNEALINGBOT_H