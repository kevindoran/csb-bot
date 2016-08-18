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
    if(sw < 7.0/32.0) {
        po.o1.thrust = rand() % (MAX_THRUST + 1);
        po.o1.shieldEnabled = false;
    } else if(sw < 14.0/32.0) {
        po.o2.thrust = rand() % (MAX_THRUST + 1);
        po.o2.shieldEnabled = false;
    } else if(sw < 22.0/32.0) {
        po.o1.angle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
    } else if(sw < 32.0/32.0) {
        po.o2.angle = Physics::degreesToRad(-18 + rand() % (MAX_ANGLE_DEG * 2 + 1));
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
    float temperature = 1;
    float K = 0.01; // Boltzman's constant.
    float coolingFraction = 0.95;
    float coolingSteps = 100;
    int stepsPerTemp = 160;
    float exponent;
    float merit, flip;
    if(hasPrevious) {
        memcpy(solution, previousSolution, turns * sizeof(PairOutput));
    }
    float currentScore = score(podsToTrain, solution, opponentPods);
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
            updated_score =  score(podsToTrain, solution, opponentPods);
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


float AnnealingBot::score(const PodState pods[], const PairOutput solution[], const PodState enemyPods[]) {
    PodState podsCopy[] = {pods[0], pods[1]};
    PodState enemyPodsCopy[] = {enemyPods[0], enemyPods[1]};
    CustomAI* customAI = new CustomAI(solution);
    simulate(podsCopy, customAI, enemyPodsCopy, &enemyBot, turns);
    delete(customAI);
    return score(podsCopy, enemyPodsCopy);
}

float AnnealingBot::score(const PodState pods[], const PodState enemyPods[]) {
    int totalCPs = race.totalCPCount();
    if(pods[0].passedCheckpoints == totalCPs) {
        return maxScore;
    } else if(enemyPods[0].passedCheckpoints == totalCPs) {
        return minScore;
    }

    Vector& nextCP = race.checkpoints[pods[0].nextCheckpoint];
    float racerScore = pods[0].passedCheckpoints * 50000 - (nextCP - pods[0].pos).getLength();
    racerScore -= 50*abs(physics.turnAngle(pods[0], nextCP));
    Vector& enemyNextCP = race.checkpoints[enemyPods[0].nextCheckpoint];
//    float enemyRacerScore = enemyPods[0].passedCheckpoints * 5000 - (enemyNextCP.pos - enemyPods[0].pos).getLength();


    float chaserScore = -(enemyNextCP - pods[1].pos).getLength();
    chaserScore -= 2* (enemyPods[0].pos - pods[1].pos).getLength();
    chaserScore -= abs(physics.turnAngle(pods[1], enemyPods[0].pos));
//    chaserScore -= enemyRacerScore;

    return -(racerScore + chaserScore);
//    return racerScore;
}

void AnnealingBot::simulate(PodState pods1[], SimBot* pods1Sim, PodState pods2[], SimBot* pods2Sim, int turns) {
    // TODO: Organise where to order the pods.
    physics.orderByProgress(pods1);
    physics.orderByProgress(pods2);
    PodState* allPods[] = {&pods1[0], &pods1[1], &pods2[0], &pods2[1]};
    for(int i = 0; i < turns; i++) {
        pods1Sim->move(pods1, pods2);
        pods2Sim->move(pods2, pods1);
        physics.simulate(allPods);
        physics.orderByProgress(pods1);
        physics.orderByProgress(pods2);
        // Need to check for game over && make sure lead pod is in pos 0.
    }
}
