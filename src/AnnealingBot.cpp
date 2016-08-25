#define _USE_MATH_DEFINES
#include <cmath>
#include <limits>
#include <cstring>
#include <cstdlib>

#include "AnnealingBot.h"
#include "State.h"
#include <assert.h>

PairOutput AnnealingBot::random() {
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

void AnnealingBot::randomEdit(PairOutput& po, int turnsRemaining, float algoProgress) {
    static const int MAX_DIST = 1000;
    static const int MIN_DIST = 30;
    float dist = MAX_DIST - algoProgress * (MAX_DIST - MIN_DIST);
    float sw = (float) rand() / RAND_MAX;
    float flip = (float) rand() /RAND_MAX;
    float thrustFactor = (1/(1-DRAG) - pow(DRAG, turnsRemaining)/(1-DRAG));
    static const int averageVel = 600;
    float angleFactor = M_PI  * averageVel * turnsRemaining;
    float angleDelta = dist / angleFactor;
    int thrustDelta = (int) dist / thrustFactor;
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
    } else if(sw < 20.0/32.0) {
        po.o1.angle = max(-MAX_ANGLE, min(MAX_ANGLE, physics.degreesToRad(-30 + rand() % (60 + 1))));
//        if(po.o1.angle == MAX_ANGLE || flip < 0.5) {
//            po.o1.angle = max(-MAX_ANGLE, po.o1.angle - angleDelta);
//        } else {
//            po.o1.angle = min(MAX_ANGLE, po.o1.angle + angleDelta);
//        }
    } else if(sw < 30.0/32.0) {
        po.o2.angle = max(-MAX_ANGLE, min(MAX_ANGLE, physics.degreesToRad(-30 + rand() % (60 + 1))));
//        if(po.o2.angle == MAX_ANGLE || flip < 0.5) {
//            po.o2.angle = max(-MAX_ANGLE, po.o2.angle - angleDelta);
//        } else {
//            po.o2.angle = min(MAX_ANGLE, po.o2.angle + angleDelta);
//        }
    } else if(sw < 31.0/32.0) {
        po.o1.shieldEnabled = true;
    } else if(sw < 1.0) {
        po.o2.shieldEnabled = true;
    }
}

void AnnealingBot::randomSolution(PairOutput sol[]) {
    for(int i = 0; i < TURNS; i++) {
        sol[i] = random();
    }
}

void AnnealingBot::_train(const PodState podsToTrain[], const PodState opponentPods[], PairOutput solution[]) {
    ourSimHistory[0][0] = podsToTrain[0];
    ourSimHistory[0][1] = podsToTrain[1];
    enemySimHistory[0][0] = opponentPods[0];
    enemySimHistory[0][1] = opponentPods[1];
    double exponent;
    double merit, flip;
    if(hasPrevious) {
        memcpy(solution, previousSolution, TURNS * sizeof(PairOutput));
    } else {
        randomSolution(solution);
    }
    float currentScore = score(solution, 0);
    float bestScore = currentScore;
#ifdef SIMOUT
    cout << currentScore << ", ";
#endif
    float updated_score;
    float startScore;
    float delta;
    int toEdit;
    PairOutput saved;
    PairOutput best;

    for(coolingIdx = 1; coolingIdx <= coolingSteps; coolingIdx++) {
        updateLoopControl();
        currentTemp *= coolingFraction;
        startScore = currentScore;
        for(int j = 1; j <= stepsPerTemp; j++) {
            // Make edits to one turn of solution.
            toEdit = rand() % TURNS;
            saved = solution[toEdit];
            randomEdit(solution[toEdit], TURNS - toEdit, ((float)coolingIdx)/coolingSteps);
            updated_score =  score(solution, toEdit);
            if(updated_score < 0) {
//                cerr << "Score below zero  " << updated_score << endl;
            }
            if(updated_score < bestScore) {
                bestScore = updated_score;
                best = solution[0];
            }
#ifdef SIMOUT
            cout << updated_score << ", ";
#endif
            delta = updated_score - currentScore;
            if(delta > 0) diffSum += delta;
            exponent = (-delta /currentScore) / (K * currentTemp);
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
    }
    cerr << "End score: " << currentScore << endl;
    cerr << "Sim count:" << simCount << endl;
    cerr << "Average score diff: " << diffSum / simCount << endl;
    solution[0] = best;
//    cerr << "Current (pos, vel)   " << podsToTrain[0].pos << "   " << podsToTrain[0].vel << endl;
//    cerr << "Moving: (thrust, angle)  " << " (" << solution[0].o1.thrust << ", " << physics.radToDegrees(solution[0].o1.angle) << ")   " << endl
//         << "Expecting state: (pos, vel, cp)   " << next.pos << "   " << next.vel << "   " << next.nextCheckpoint << endl;
#ifdef SIMOUT
    cout << endl;
#endif
//    memcpy(previousSolution, solution, TURNS*sizeof(PairOutput));
}




float AnnealingBot::score(const PairOutput solution[], int startFromTurn) {
    CustomAI *customAI = new CustomAI(race, solution, startFromTurn);
    enemyBot->setTurn(startFromTurn);
    simulate(customAI, enemyBot, TURNS, startFromTurn);
    delete (customAI);
    const PodState* ourPods[] = {&ourSimHistory[TURNS][0], &ourSimHistory[TURNS][1]};
    const PodState* ourPodsPrev[] = {&ourSimHistory[0][0], &ourSimHistory[0][1]};
    const PodState* enemyPods[] = {&enemySimHistory[TURNS][0], &enemySimHistory[TURNS][1]};
    const PodState* enemyPodsPrev[] = {&enemySimHistory[0][0], &enemySimHistory[0][1]};
    return score(ourPods, ourPodsPrev, enemyPods, enemyPodsPrev);
}

float AnnealingBot::score(const PodState* pods[], const PodState* podsPrev[], const PodState* enemyPods[], const PodState* enemyPodsPrev[]) {
    const int totalCPs = race.totalCPCount();

//    if(pods[0]->passedCheckpoints == totalCPs) {
//        for(int i = 1; i <= TURNS; i++) {
//            if(ourSimHistory[0][i].passedCheckpoints == totalCPs) {
//                return minScore + i * 500;
//            }
//        }
//    }
//    if(enemyPods[0]->passedCheckpoints == totalCPs) {
//        for(int i = 0; i < TURNS; i++) {
//            if(enemySimHistory[0][i].passedCheckpoints == totalCPs) {
//                return maxScore  - i * 500;
//            }
//        }
//    }
    if(pods[0]->turnsSinceCP >= WANDER_TIMEOUT && pods[1]->turnsSinceCP >= WANDER_TIMEOUT) {
        return maxScore;
    }
    if(enemyPods[0]->turnsSinceCP >= WANDER_TIMEOUT && enemyPods[1]->turnsSinceCP >= WANDER_TIMEOUT) {
        return minScore;
    }

    // Racer
    float racerScore = progress(pods[0], podsPrev[0]) - sFactors.enemyProgress*progress(enemyPods[0], enemyPodsPrev[0]);
    racerScore -= 0.5*engagedScore(enemyPods[1], pods[0], podsPrev[0]);
    float chaserScore = 0;
    if(pods[0]->turnsSinceCP > 60) {
            chaserScore -=  Vector::dist(pods[0]->pos, pods[1]->pos);
            chaserScore += Vector::dist(enemyPods[1]->pos, pods[0]->pos);
        } else {
        chaserScore = engagedScore(pods[1], enemyPods[0], enemyPodsPrev[1]);
    }
    return -(racerScore*sFactors.overallRacer + chaserScore*sFactors.overallBouncer);
}

float AnnealingBot::progress(const PodState* pod, const PodState* previous) {
    // Range: [0, 20000]
    static const int PASS_CP_BONUS = sFactors.passCPBonus;
    int ourNextCPID = pod->nextCheckpoint;
    int ourCurCPID = previous->nextCheckpoint;
    Vector ourNextCP = race.checkpoints[ourCurCPID];
    Vector ourCurCP = race.checkpoints[ourCurCPID];
    float progress = race.distFromPrevCP(ourNextCPID) - Vector::dist(pod->pos, ourNextCP);
    int i = ourCurCPID;
    while(i != ourNextCPID) {
        progress += PASS_CP_BONUS;
        progress += sFactors.distToCP * race.distToNextCP(i);
        i = race.followingCheckpoint(i);
    }
    return progress;
}

static float timeFromDVA(float distance, float velocity, float acc) {
    return (-velocity + sqrt(velocity*velocity - 2*acc*distance)) / acc;
}

static float sigmoid(float x) {
    return x / (3*(1 + abs(x)));
}

float AnnealingBot::engagedScore(const PodState* bouncer, const PodState* target, const PodState* targetPrev) {
    float score = 0;
    int targetCP = target->nextCheckpoint;
    Vector enemyCPDiff = race.checkpoints[target->nextCheckpoint] - target->pos;
    Vector bouncerCPDiff = race.checkpoints[target->nextCheckpoint] - bouncer->pos;
    Vector enemyBouncerDiff = bouncer->pos - target->pos;
    float angleSeenByCP = 637.0f * (M_PI/2.0f - abs(physics.angleBetween(target->pos -race.checkpoints[targetCP], bouncer->pos-race.checkpoints[targetCP])));
    float angleSeenByEnemy = 637.0f * (M_PI/2.0f - abs(physics.angleBetween(race.checkpoints[targetCP] - target->pos, bouncer->pos - target->pos)));
    float bouncerTurnAngle = 637.0f * (M_PI/2.0f - abs(physics.turnAngle(*bouncer, target->pos)));
    float enemyTurnAngle = 637.0f * (M_PI/2.0f - abs(physics.turnAngle(*target, bouncer->pos)));
    float checkpointPenalty = target->passedCheckpoints > targetPrev->passedCheckpoints ? 1 : 0;

    score += sFactors.enemyDistToCP * enemyCPDiff.getLength() +
            sFactors.bouncerDistToCP * bouncerCPDiff.getLength() +
            sFactors.enemyDist * enemyBouncerDiff.getLength() +
            sFactors.angleSeenByCP * angleSeenByCP +
            sFactors.angleSeenByEnemy * angleSeenByEnemy +
            sFactors.bouncerTurnAngle * bouncerTurnAngle +
            sFactors.enemyTurnAngle * enemyTurnAngle +
            sFactors.checkpointPenalty * checkpointPenalty;
    return score;
}

void AnnealingBot::simulate(SimBot* pods1Sim, SimBot* pods2Sim, int turns, int startFromTurn) {
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
