#include <limits>
#include <cstring>
#include <cstdlib>
#include <stdlib.h>

#include "AnnealingBot.h"
#include "State.h"

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

void AnnealingBot::randomEdit(PairOutput& po) {
    float sw = (float) rand() / RAND_MAX;
    if(sw < 5.0/32.0) {
        //pb4608's idea to generate between -100 and 400 then bound to [0, 200]
        po.o1.thrust = max(0, min(MAX_THRUST, (rand() % (500 + 1)) - 100));
        po.o1.shieldEnabled = false;
    } else if(sw < 10.0/32.0) {
        po.o2.thrust = max(0, min(MAX_THRUST, (rand() % (500 + 1)) - 100));
        po.o2.shieldEnabled = false;
    } else if(sw < 20.0/32.0) {
        float angle = max(-18, min(18, (-50 + rand() % (50 + 1))));
        po.o1.angle = Physics::degreesToRad(angle);
    } else if(sw < 30.0/32.0) {
        float angle = max(-18, min(18, (-50 + rand() % (50 + 1))));
        po.o2.angle = Physics::degreesToRad(angle);
    } else if(sw < 31.0/32.0) {
        po.o1.shieldEnabled = true;
    } else if(sw < 1.0) {
        po.o2.shieldEnabled = true;
    }
}

void AnnealingBot::randomSolution(PairOutput sol[]) {
    for(int i = 0; i < turns; i++) {
        sol[i] = random();
    }
}

void AnnealingBot::train(const PodState podsToTrain[], const PodState opponentPods[], PairOutput solution[]) {
    ourSimHistory[0][0] = podsToTrain[0];
    ourSimHistory[0][1] = podsToTrain[1];
    enemySimHistory[0][0] = podsToTrain[0];
    enemySimHistory[0][1] = podsToTrain[1];
    float temperature = 1;
    float K = 0.01; // Boltzman's constant.
    float coolingFraction = 0.93;
    float coolingSteps = 100;
    int stepsPerTemp = 175;
    float exponent;
    float merit, flip;
//    if(hasPrevious) {
//        memcpy(solution, previousSolution, turns * sizeof(PairOutput));
//    } else {
        randomSolution(solution);
//    }
    float currentScore = score(solution, 0) - podsToTrain[0].passedCheckpoints*10000;
    float updated_score;
    float startScore;
    float delta;
    int toEdit;
    PairOutput saved;

    for(int i = 1; i <= coolingSteps; i++) {
        temperature *= coolingFraction;
        startScore = currentScore;
        for(int j = 1; j <= stepsPerTemp; j++) {
            // Make edits to one turn of solution.
            toEdit = rand() % turns;
            saved = solution[toEdit];
            randomEdit(solution[toEdit]);
//            solution[toEdit] = random();
            updated_score =  score(solution, toEdit) - podsToTrain[0].passedCheckpoints*10000;
            delta = updated_score - currentScore;
            exponent = (-delta / currentScore) / (K * temperature);
            merit = exp(exponent);
            if(merit >= 1.0) {
                merit = 0.0;
            }
            if(delta < 0) {
                currentScore += delta;
            } else {
                // Used for random variable with mean 0.5.
                flip = ((float) rand() / (RAND_MAX));
                if(merit > flip) {
                    currentScore += delta;
                } else {
                    // transition back.
                    solution[toEdit] = saved;
                }
            }
        }
        if(currentScore - startScore < 0.0) {
            temperature /= coolingFraction;
        }
    }
    memcpy(previousSolution, solution, turns*sizeof(PairOutput));
}


float AnnealingBot::score(const PairOutput solution[], int startFromTurn) {
    CustomAI *customAI = new CustomAI(solution);
    simulate(customAI, &enemyBot, turns, startFromTurn);
    delete (customAI);
    const PodState* ourPods[] = {&ourSimHistory[turns][0], &ourSimHistory[turns][1]};
    const PodState* enemyPods[] = {&enemySimHistory[turns][0], &enemySimHistory[turns][1]};
    return score(ourPods, enemyPods);
}

float AnnealingBot::score(const PodState* pods[], const PodState* enemyPods[]) {
    int totalCPs = race.totalCPCount();
    if(pods[0]->passedCheckpoints == totalCPs) {
        return maxScore;
    } else if(enemyPods[0]->passedCheckpoints == totalCPs) {
        return minScore;
    }

    Vector& nextCP = race.checkpoints[pods[0]->nextCheckpoint];
    float racerScore = pods[0]->passedCheckpoints * 10000 - Vector::dist(nextCP, pods[0]->pos);
//    float racerScore =  - Vector::dist(nextCP, pods[0].pos);
//    racerScore -= 10*abs(physics.turnAngle(pods[0], nextCP));
    Vector& enemyNextCP = race.checkpoints[enemyPods[0]->nextCheckpoint];
//    float enemyRacerScore = - Vector::dist(enemyNextCP, enemyPods[0].pos);
//    racerScore -= enemyRacerScore;

//    float chaserScore = -Vector::dist(enemyNextCP, pods[1].pos);
//    chaserScore -= 2* Vector::dist(enemyPods[0].pos, pods[1].pos);
//    chaserScore -= abs(physics.turnAngle(pods[1], enemyPods[0].pos));
//    chaserScore -= enemyRacerScore;

//    return -(racerScore + chaserScore);
    return -racerScore;
}

void AnnealingBot::simulate(SimBot* pods1Sim, SimBot* pods2Sim, int turns, int startFromTurn) {
    // TODO: Organise where to order the pods.
    physics.orderByProgress(ourSimHistory[startFromTurn]);
    physics.orderByProgress(enemySimHistory[startFromTurn]);
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
        physics.orderByProgress(ourSimHistory[startFromTurn+1]);
        physics.orderByProgress(enemySimHistory[startFromTurn+1]);
        // Need to check for game over && make sure lead pod is in pos 0.
    }
}
