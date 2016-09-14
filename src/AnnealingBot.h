#ifndef CODERSSTRIKEBACK_ANNEALINGBOT_H
#define CODERSSTRIKEBACK_ANNEALINGBOT_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstring>
#include <assert.h>
#include <limits>
#include <cstdlib>
#include <chrono>

#include "State.h"
#include "Bot.h"
#include "Navigation.h"
#include "Physics.h"


struct ScoreFactors {
    // Racer
    float overallRacer;
    float passCPBonus;
    float progressToCP;
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
    float shieldPenalty;
};

static ScoreFactors defaultFactors = {
        1,    // overallRacer
        1789, // passCPBonus
        1.70,    // progressToCP
        -0.176, // enemyProgress
        1,    // overallBouncer
        -0.079, // enemyDist
        0.119, // enemyDistToCP
        -0.928,  // bouncerDistToCP
        -0.332,  // angleSeenByCP
        -0.240,  // angleSeenByEnemy
        -0.785,  // bouncerTurnAngle
        -0.669,  // enemyTurn angle
        -74   // checkpoint penalty
        -390   // shield penalty
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
            float thrust = (int) MAX_THRUST - int(((turnAngle - angleThreshold) / (cutOff - angleThreshold)) * MAX_THRUST);
            // Turn angle should be rounded to the nearest degree also.
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
        target = Vector(targetx, targety);
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

template<int TURNS>
class AnnealingBot : public DuelBot {
public:
    ScoreFactors sFactors = defaultFactors;
    bool isControl = false;
private:
    static constexpr float maxScore = 300000;//numeric_limits<float>::infinity();
    static constexpr float minScore = 1000;//-numeric_limits<float>::infinity();
    // Loop control and timing.
    static const int reevalPeriodMilli = 4;
    static const int timeBufferMilli = 1;
    static constexpr float initTemp = 10.0;
    static constexpr float initCoolingFraction = 0.95;
    static constexpr float startAcceptanceRate = 0.98;
    static constexpr float endAcceptanceRate = 0.02;
    static constexpr float stepsVsCoolRatio = 1.3;
    static const int initCoolingSteps = 160;
    static const int initStepsPerTemp = 140;
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
    float currentTemp = initTemp;
    float coolingFraction = UNSET;//initCoolingFraction;
    bool toDeleteEnemy = false;
    // SD & mean
    float mean;
    double M2;


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

    void randomEdit(PairOutput &po);//, int turnsRemaining, float algoProgress);

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
            // The update timer has elapsed, or we are on our second loop, so need to create a better estimate of
            // start temp, end temp and cooling fraction.
            lastUpdateTime = timeNow;
            float simRate = simsSinceUpdate / elapsed;
            int simsRemaining = simRate * timeRemaining;
            // A*2A = C
            // A = sqrt(C/2)
            coolingSteps = sqrt((simCount + simsRemaining) / (stepsVsCoolRatio));
            stepsPerTemp = coolingSteps * stepsVsCoolRatio;

//            cerr << "Tunnel %: " << (float) tunnelCount / (tunnelCount + nonTunnelCount) << endl;
            tunnelCount = 0;
            nonTunnelCount = 0;
            simsSinceUpdate = 0;
//            cerr << "alpha: " << coolingFraction << endl;
//            cerr << "Cooling steps: " << coolingSteps << endl;
//            cerr << "Steps per temp: " << stepsPerTemp << endl;
        }
        if(elapsed > reevalPeriodMilli || coolingIdx == 1) {
            // T0 = -sd/ln(startAcceptanceRate)    [from startAcceptanceRate = exp(-sd/T0)]
            float SD = sqrt(M2/simCount);
//            cerr << "SD: " << SD << endl;
            float startTemp = -SD/log(startAcceptanceRate);
            float endTemp = -SD/log(endAcceptanceRate);
            coolingFraction = pow(endTemp/startTemp, 1.0/(coolingSteps));
            currentTemp = startTemp*pow(coolingFraction, coolingIdx);
//            cerr << "Cooling steps: " << coolingSteps;
//            cerr << "Cooling Fraction: " << coolingFraction << endl;
        }
    }

    void init() {
        startTime = getTimeMilli();
        lastUpdateTime = startTime;
        currentTemp = initTemp;
        coolingSteps = allocatedTime == UNSET ? initCoolingSteps : allocatedTime * 1.2;
        coolingFraction = initCoolingFraction;
        stepsPerTemp = initStepsPerTemp;
        simCount = 0;
        tunnelCount = 0;
        nonTunnelCount = 0;
        simsSinceUpdate = 0;
        coolingIdx = 0;
        diffSum = 0;
        coolCount = 0;
    }

public:
    AnnealingBot() {
    }

    AnnealingBot(Race &r) : race(r), physics(race) {
        enemyBot = new MinimalBot(race);
        toDeleteEnemy = true;
    }

    AnnealingBot(Race &r, long allocatedTimeMilli) : race(r), physics(race), allocatedTime(allocatedTimeMilli) {
        enemyBot = new MinimalBot(race);
        toDeleteEnemy = true;
    }

    AnnealingBot(Race &r, long allocatedTimeMilli, SimBot* enemyBot) :
            race(r), physics(race), allocatedTime(allocatedTimeMilli), enemyBot(enemyBot) {
    }

    ~AnnealingBot() {
        if(toDeleteEnemy) {
            delete (enemyBot);
        }
    }

    void setEnemyAI(SimBot* enemyAI) {
        if(toDeleteEnemy) delete(enemyBot);
        enemyBot = enemyAI;
        toDeleteEnemy = false;

    }

    void setInnitialSolution(PairOutput po[]) {
        memcpy(previousSolution, po, sizeof(PairOutput) * TURNS);
        hasPrevious = true;
    }

    float score(const PodState *pods[], const PodState *podsPrev[], const PodState *enemyPods[],
                const PodState *enemyPodsPrev[]);

    void train(const PodState pods[], const PodState enemyPods[], PairOutput solution[]) {
        init();
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
            gameState.ourState().pods[0].vel += (race.checkpoints[1] - gameState.ourState().pods[0].pos).normalize() * BOOST_ACC;
            gameState.ourState().pods[1].vel += (race.checkpoints[1] - gameState.ourState().pods[1].pos).normalize() * BOOST_ACC;
            gameState.enemyState().pods[0].vel += (race.checkpoints[1] - gameState.enemyState().pods[0].pos).normalize() * BOOST_ACC;
            gameState.enemyState().pods[1].vel += (race.checkpoints[1] - gameState.enemyState().pods[1].pos).normalize() * BOOST_ACC;
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


    float progress(const PodState *pod, const PodState *previous);

    float scoreBenchmark(const PodState **pods, const PodState **podsPrev, const PodState **enemyPods,
                         const PodState **enemyPodsPrev);

    float bouncerScore(const PodState *bouncer, const PodState *target, const PodState *targetPrev);
};


template<int TURNS>
PairOutput AnnealingBot<TURNS>::random() {
    int randomSpeed = rand() % (MAX_THRUST + 1);
//    int randomSpeed = ((float)rand() / RAND_MAX) > 0.5 ? 0 : MAX_THRUST;
    float randomAngle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
    bool shieldEnabled = false;
    PodOutputSim o1(randomSpeed, randomAngle, shieldEnabled, false);

    randomSpeed = rand() % (MAX_THRUST + 1);
//    randomSpeed = ((float)rand() / RAND_MAX) > 0.5 ? 0 : MAX_THRUST;
    randomAngle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
    PodOutputSim o2(randomSpeed, randomAngle, shieldEnabled, false);
    return PairOutput(o1, o2);
}

template<int TURNS>
void AnnealingBot<TURNS>::randomEdit(PairOutput& po) {//, int turnsRemaining, float algoProgress) {
    static const int MAX_DIST = 1000;
    static const int MIN_DIST = 30;
//    float dist = MAX_DIST - algoProgress * (MAX_DIST - MIN_DIST);
    float sw = (float) rand() / RAND_MAX;
    float flip = (float) rand() /RAND_MAX;
//    float thrustFactor = (1/(1-DRAG) - pow(DRAG, turnsRemaining)/(1-DRAG));
//    static const int averageVel = 600;
//    float angleFactor = M_PI  * averageVel * turnsRemaining;
//    float angleDelta = dist / angleFactor;
//    int thrustDelta = (int) dist / thrustFactor;
//    float angle;
    if(sw < 5.0/32.0) {
        po.o1.thrust = max(0, min(MAX_THRUST, (rand() % (500 + 1) - 100)));
//        if(po.o1.thrust == MAX_THRUST || flip < 0.5) {
//            po.o1.thrust = max(0, po.o1.thrust - thrustDelta);
//        } else {
//            po.o1.thrust = min(MAX_THRUST, po.o1.thrust + thrustDelta);
//        }
        po.o1.shieldEnabled = false;
    } else if(sw < 10.0/32.0) {
        po.o2.thrust = max(0, min(MAX_THRUST, (rand() % (500 + 1) - 100)));
//        if(po.o2.thrust == MAX_THRUST || flip < 0.5) {
//            po.o2.thrust = max(0, po.o2.thrust - thrustDelta);
//        } else {
//            po.o2.thrust = min(MAX_THRUST, po.o2.thrust + thrustDelta);
//        }
        po.o2.shieldEnabled = false;
    } else if(sw < 19.0/32.0) {
        po.o1.angle = max(-MAX_ANGLE, min(MAX_ANGLE, physics.degreesToRad(-30 + rand() % (60 + 1))));
//        if(po.o1.angle == MAX_ANGLE || flip < 0.5) {
//            po.o1.angle = max(-MAX_ANGLE, po.o1.angle - angleDelta);
//        } else {
//            po.o1.angle = min(MAX_ANGLE, po.o1.angle + angleDelta);
//        }
    } else if(sw < 28.0/32.0) {
        po.o2.angle = max(-MAX_ANGLE, min(MAX_ANGLE, physics.degreesToRad(-30 + rand() % (60 + 1))));
//        if(po.o2.angle == MAX_ANGLE || flip < 0.5) {
//            po.o2.angle = max(-MAX_ANGLE, po.o2.angle - angleDelta);
//        } else {
//            po.o2.angle = min(MAX_ANGLE, po.o2.angle + angleDelta);
//        }
    } else if(sw < 30.0/32.0) {
        po.o1.shieldEnabled = true;
    } else if(sw < 1.0) {
        po.o2.shieldEnabled = true;
    }
}

template<int TURNS>
void AnnealingBot<TURNS>::randomSolution(PairOutput sol[]) {
    for(int i = 0; i < TURNS; i++) {
        sol[i] = random();
    }
}

template<int TURNS>
void AnnealingBot<TURNS>::_train(const PodState podsToTrain[], const PodState opponentPods[], PairOutput solution[]) {
    ourSimHistory[0][0] = podsToTrain[0];
    ourSimHistory[0][1] = podsToTrain[1];
    enemySimHistory[0][0] = opponentPods[0];
    enemySimHistory[0][1] = opponentPods[1];
    double exponent;
    double merit, flip;
    if(hasPrevious) {
        for (int i = 0; i < TURNS - 1; i++) {
            solution[i] = previousSolution[i + 1];
        }
        randomEdit(solution[TURNS - 1]);
    } else {
        randomSolution(solution);
    }
    float currentScore = score(solution, 0);
    float bestScore = currentScore;
    float updated_score;
    float startScore;
    float delta;
    int toEdit = 0;
    PairOutput saved;
    PairOutput best[TURNS];
    // SD & mean
    mean = currentScore;
    float d = 0;
    M2 = 0;
    float meanPrev;

    for(; coolingIdx <= coolingSteps; coolingIdx++) {
        updateLoopControl();
        startScore = currentScore;
        for(int j = 1; j <= stepsPerTemp; j++) {
            // Make edits to one turn of solution.
            toEdit = (toEdit + 1) % TURNS;//rand() % TURNS;
            saved = solution[toEdit];
            randomEdit(solution[toEdit]);//, TURNS - toEdit, ((float)coolingIdx)/coolingSteps);
            updated_score =  score(solution, toEdit);
            // Stats
            d = updated_score - mean;
            // simCount is updated at the end of the loop, so need to add 1 here.
            meanPrev = mean;
            mean += d / (simCount+1);
            M2 += d * (updated_score - mean);
            if(!(M2 >= 0 || M2 <= 0)) {
                cerr << "Updated score: " << updated_score << endl;
                cerr << "M2: " << M2 << endl;
                cerr << "Sim count: " << simCount << endl;
                cerr << "Mean: " << mean << endl;
                cerr << "d: " << d << endl;
                M2 = 0;
                mean = meanPrev;
            }
            if(updated_score < 0) {
                cerr << "Score below zero  " << updated_score << endl;
            }
            if(updated_score < bestScore) {
                bestScore = updated_score;
                memcpy(best, solution, TURNS * sizeof(PairOutput));
            }
            delta = updated_score - currentScore;
            if(delta > 0) diffSum += delta;
            exponent = (-delta /*/currentScore*/) / (currentTemp);
            merit = exp(exponent);
            if(merit > 1.0) {
                merit = 0.0;
            }
            if(delta < 0) {
                currentScore += delta;
            } else {
                // Used for random variable with mean 0.5.
                flip = ((float) rand() / (RAND_MAX));
                if(merit > flip) {
                    currentScore += delta;
                    tunnelCount++;
                } else {
                    nonTunnelCount++;
                    // transition back.
                    solution[toEdit] = saved;
                }
            }
            simCount++;
            simsSinceUpdate++;
        }
        if(currentScore - startScore < 0.0) {
            currentTemp /= coolingFraction;
        }
        coolCount++;
        currentTemp *= coolingFraction;
    }
//    cerr << "End score: " << currentScore << endl;
//    cerr << "Sim count:" << simCount << endl;
//    cerr << "Average score diff: " << diffSum / simCount << endl;
    memcpy(solution, best, TURNS*sizeof(PairOutput));
//    cerr << "Current (pos, vel)   " << podsToTrain[0].pos << "   " << podsToTrain[0].vel << endl;
//    cerr << "Moving: (thrust, angle)  " << " (" << solution[0].o1.thrust << ", " << physics.radToDegrees(solution[0].o1.angle) << ")   " << endl
//         << "Expecting state: (pos, vel, cp)   " << next.pos << "   " << next.vel << "   " << next.nextCheckpoint << endl;
    memcpy(previousSolution, solution, TURNS*sizeof(PairOutput));
    hasPrevious = true;
}

template<int TURNS>
float AnnealingBot<TURNS>::score(const PairOutput solution[], int startFromTurn) {
    CustomAI *customAI = new CustomAI(race, solution, startFromTurn);
    enemyBot->setTurn(startFromTurn);
    simulate(customAI, enemyBot, TURNS, startFromTurn);
    delete (customAI);
    const PodState* ourPods[] = {&ourSimHistory[TURNS][0], &ourSimHistory[TURNS][1]};
    const PodState* ourPodsPrev[] = {&ourSimHistory[0][0], &ourSimHistory[0][1]};
    const PodState* enemyPods[] = {&enemySimHistory[TURNS][0], &enemySimHistory[TURNS][1]};
    const PodState* enemyPodsPrev[] = {&enemySimHistory[0][0], &enemySimHistory[0][1]};
    if(isControl) {
        return scoreBenchmark(ourPods, ourPodsPrev, enemyPods, enemyPodsPrev);
    } else {
        return score(ourPods, ourPodsPrev, enemyPods, enemyPodsPrev);
    }
}

template<int TURNS>
float AnnealingBot<TURNS>::score(const PodState* pods[], const PodState* podsPrev[], const PodState* enemyPods[], const PodState* enemyPodsPrev[]) {
    const int totalCPs = race.totalCPCount();

    if(pods[0]->passedCheckpoints == totalCPs) {
        for(int i = 1; i <= TURNS; i++) {
            if(ourSimHistory[0][i].passedCheckpoints == totalCPs) {
                return minScore + i * 500;
            }
        }
    }
    if(enemyPods[0]->passedCheckpoints == totalCPs) {
        for(int i = 0; i < TURNS; i++) {
            if(enemySimHistory[0][i].passedCheckpoints == totalCPs) {
                return maxScore  - i * 500;
            }
        }
    }
    if(pods[0]->turnsSinceCP >= WANDER_TIMEOUT && pods[1]->turnsSinceCP >= WANDER_TIMEOUT) {
        return maxScore;
    }
    if(enemyPods[0]->turnsSinceCP >= WANDER_TIMEOUT && enemyPods[1]->turnsSinceCP >= WANDER_TIMEOUT) {
        return minScore;
    }

    // Racer
    float racerScore = progress(pods[0], podsPrev[0]) + sFactors.enemyProgress*progress(enemyPods[0], enemyPodsPrev[0]);
    float chaserScore = 0;
    if(pods[0]->turnsSinceCP > 60) {
        chaserScore -=  0.3*Vector::dist(pods[0]->pos, pods[1]->pos);
        chaserScore += 0.3*Vector::dist(enemyPods[1]->pos, pods[0]->pos);
    } else {
        chaserScore = bouncerScore(pods[1], enemyPods[0], enemyPodsPrev[0]);
    }
    float score = 200000-(racerScore*sFactors.overallRacer + chaserScore*sFactors.overallBouncer);
    if(!(score <= 0 || score >= 0)) {
        cerr << "Score NaN, but from where?" << endl;
    }
    return score;
}

template<int TURNS>
float AnnealingBot<TURNS>::progress(const PodState* pod, const PodState* previous) {
    // Range: [0, 20000]
    static const int PASS_CP_BONUS = sFactors.passCPBonus;
    int ourNextCPID = pod->nextCheckpoint;
    int ourCurCPID = previous->nextCheckpoint;
    Vector ourNextCP = race.checkpoints[ourNextCPID];
    Vector ourCurCP = race.checkpoints[ourCurCPID];
    float progress = sFactors.progressToCP * (race.distFromPrevCP(ourNextCPID) - Vector::dist(pod->pos, ourNextCP));
    int i = ourCurCPID;
    while(i != ourNextCPID) {
        progress += sFactors.passCPBonus;
        progress += sFactors.progressToCP * race.distFromPrevCP(i);
        i = race.followingCheckpoint(i);
    }
    if(!(progress <= 0 || progress >= 0)) {
        cerr << "Progress NaN: " << progress << endl;
    }
    return progress;
}


static float timeFromDVA(float distance, float velocity, float acc) {
    return (-velocity + sqrt(velocity*velocity - 2*acc*distance)) / acc;
}

static float sigmoid(float x) {
    return x / (3*(1 + abs(x)));
}

static constexpr float MAX_DIST = 30000.0f;

template<int TURNS>
float AnnealingBot<TURNS>::bouncerScore(const PodState *bouncer, const PodState *target, const PodState *targetPrev) {
    float score = 0;
    int targetCP = target->nextCheckpoint;
    Vector enemyCPDiff = target->pos - race.checkpoints[target->nextCheckpoint];
    Vector bouncerCPDiff = bouncer->pos - race.checkpoints[target->nextCheckpoint];
    Vector enemyBouncerDiff = bouncer->pos - target->pos;
    float angleSeenByCP = bouncerCPDiff.getLength() == 0 ? 0 : 637.0f * (abs(physics.angleBetween(enemyCPDiff, bouncerCPDiff)) - M_PI/2.0f);
    float angleSeenByEnemy = 637.0f * (abs(physics.angleBetween(race.checkpoints[targetCP] - target->pos, bouncer->pos - target->pos)) - M_PI/2.0f);
    float bouncerTurnAngle = 637.0f * (abs(physics.turnAngle(*bouncer, target->pos)) - M_PI/2.0f);
    float enemyTurnAngle = 637.0f * (abs(physics.turnAngle(*target, bouncer->pos)) - M_PI/2.0f);
    float checkpointPenalty = target->passedCheckpoints > targetPrev->passedCheckpoints ? 1 : 0;

    score += sFactors.enemyDistToCP * (-4000 + min(MAX_DIST, enemyCPDiff.getLength())) +
             sFactors.bouncerDistToCP * (-4000 + min(MAX_DIST, bouncerCPDiff.getLength())) +
             sFactors.enemyDist * (-3000 + min(MAX_DIST, enemyBouncerDiff.getLength())) +
             sFactors.angleSeenByCP * angleSeenByCP +
             sFactors.angleSeenByEnemy * angleSeenByEnemy +
             sFactors.bouncerTurnAngle * bouncerTurnAngle +
             sFactors.enemyTurnAngle * enemyTurnAngle +
             sFactors.checkpointPenalty * checkpointPenalty;

    // Test
    score += max(0, 6-bouncer->turnsSinceShield) * sFactors.shieldPenalty;
    if(!(score <= 0 || score >= 0)) {
        cerr << "NaN for bouncer score." << endl;
        cerr << "angleSeenByCP " << angleSeenByCP << endl;
        cerr << "angleSeenByEnemy " << angleSeenByEnemy << endl;
        cerr << "bouncerTurnAngle"  << bouncerTurnAngle << endl;
        cerr << "enemyTurnAngle " << enemyTurnAngle << endl;
        cerr << "enemyCPDiff " << enemyCPDiff.getLength() << endl;
        cerr << "bouncerCPDiff " << bouncerCPDiff.getLength() << endl;
        cerr << "enemyBouncerDiff " << enemyBouncerDiff.getLength() << endl;
        cerr << "bouncer pos " << bouncer->pos << endl;
        cerr << "target pos " << target->pos << endl;
    }
    return score;
}


// Score used by the first 10th place bot (with annealing K at 0.02).
template<int TURNS>
float AnnealingBot<TURNS>::scoreBenchmark(const PodState* pods[], const PodState* podsPrev[], const PodState* enemyPods[], const PodState* enemyPodsPrev[]) {
    const int totalCPs = race.totalCPCount();

    if(pods[0]->passedCheckpoints == totalCPs) {
        for(int i = 1; i <= TURNS; i++) {
            if(ourSimHistory[0][i].passedCheckpoints == totalCPs) {
                return minScore + i * 500;
            }
        }
    }
    if(enemyPods[0]->passedCheckpoints == totalCPs) {
        for(int i = 0; i < TURNS; i++) {
            if(enemySimHistory[0][i].passedCheckpoints == totalCPs) {
                return maxScore  - i * 500;
            }
        }
    }
    if(pods[0]->turnsSinceCP >= WANDER_TIMEOUT && pods[1]->turnsSinceCP >= WANDER_TIMEOUT) {
        return maxScore;
    }
    if(enemyPods[0]->turnsSinceCP >= WANDER_TIMEOUT && enemyPods[1]->turnsSinceCP >= WANDER_TIMEOUT) {
        return minScore;
    }

    float racerScore = -Vector::dist(pods[0]->pos, race.checkpoints[pods[0]->nextCheckpoint]) + 20000 * (pods[0]->passedCheckpoints - podsPrev[0]->passedCheckpoints);
    float chaserScore = 0;
    const int WANDER_BUFFER = 7;
    racerScore -= 0.85*(-Vector::dist(enemyPods[0]->pos, race.checkpoints[enemyPods[0]->nextCheckpoint]) + 10000 * (enemyPods[0]->passedCheckpoints - enemyPodsPrev[0]->passedCheckpoints));
    if(pods[0]->turnsSinceCP > 65) {
        chaserScore -=  Vector::dist(pods[0]->pos, pods[1]->pos);
        chaserScore += Vector::dist(enemyPods[1]->pos, pods[0]->pos);
    } else {
        chaserScore += +0.6*(100*(M_PI/2 - abs(physics.angleBetween(race.checkpoints[enemyPods[0]->nextCheckpoint]-enemyPods[0]->pos, race.checkpoints[enemyPods[0]->nextCheckpoint]-pods[1]->pos)))
                             - 0.4*Vector::dist(enemyPods[0]->pos, pods[1]->pos) - Vector::dist(race.checkpoints[enemyPods[0]->nextCheckpoint], pods[1]->pos));
    }
    return 100000-(racerScore + chaserScore);
}

template<int TURNS>
void AnnealingBot<TURNS>::simulate(SimBot* pods1Sim, SimBot* pods2Sim, int turns, int startFromTurn) {
    PodState* allPods[POD_COUNT*2];
    for(int i = startFromTurn; i < turns; i++) {
        memcpy(ourSimHistory[i+1], ourSimHistory[i], POD_COUNT*sizeof(PodState));
        memcpy(enemySimHistory[i+1], enemySimHistory[i], POD_COUNT*sizeof(PodState));
        allPods[0] = &ourSimHistory[i+1][0];
        allPods[1] = &ourSimHistory[i+1][1];
        allPods[2] = &enemySimHistory[i+1][0];
        allPods[3] = &enemySimHistory[i+1][1];
        pods1Sim->move(ourSimHistory[i+1], enemySimHistory[i+1]);
        pods2Sim->move(enemySimHistory[i+1], ourSimHistory[i+1]);
        physics.simulate(allPods);
    }
}
#endif //CODERSSTRIKEBACKC_ANNEALINGBOT_H