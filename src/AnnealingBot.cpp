#include <limits>

#include <cstdlib>
#include <stdlib.h>
#include "DuelBot.h"
#include "State.h"


void SimBot::apply(vector<PodState>& pods, PairOutput& control) {
    apply(pods[0], control.o1);
    apply(pods[1], control.o2);
}

void SimBot::apply(PodState& pod, PodOutputSim& control) {
    pod.angle += control.angle;
    if(control.shieldEnabled) {
        pod.shieldEnabled = true;
    } else {
        pod.vel += Vector::fromMagAngle(control.thrust, pod.angle);
    }
}

void SimBot::apply(PodState& pod, PodOutput& control) {
    Vector force = Physics::forceFromTarget(pod, control.target, control.thrust);
    float turn = Physics::turnAngle(pod, force);
    pod.angle += turn;
    if(control.thrust == PodOutput::SHIELD) {
        pod.shieldEnabled = true;
    } else {
        pod.vel += force;
    }
}

vector<PairOutput> AnnealingBot::randomSolution() {
    vector<PairOutput> sol;
    for(int i = 0; i < turns; i++) {
        sol.push_back(PairOutput::random());
    }
    return sol;
}

vector<PairOutput> AnnealingBot::train(const vector<PodState>& podsToTrain, const vector<PodState>& opponentPods) {
    float temperature = 1;
    float K = 0.01; // Boltzman's constant.
    float coolingFraction = 0.92;
    float coolingSteps = 75;
    int stepsPerTemp = 150;
    float exponent;
    float merit, flip;
    vector<PairOutput> solution = randomSolution();
    float currentScore = score(podsToTrain, solution, opponentPods);
    float updated_score;
    float startScore;
    float delta;
    PairOutput saved;

    for(int i = 1; i <= coolingSteps; i++) {
        temperature *= coolingFraction;
        startScore = currentScore;
        for(int j = 1; j <= stepsPerTemp; j++) {
            // Make edits to one turn of solution.
            int toEdit = rand() % turns;
            saved = solution[toEdit];
            solution[toEdit] = PairOutput::random();
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
    return solution;
}


float AnnealingBot::score(vector<PodState> pods, vector<PairOutput> solution, vector<PodState> enemyPods) {
    CustomAI* customAI = new CustomAI(solution);
    MinimalBot* minimalBot = new MinimalBot(race);
    simulate(pods, customAI, enemyPods, minimalBot, turns);
    delete(customAI);
    delete(minimalBot);
    return score(pods, enemyPods);
}

float AnnealingBot::score(vector<PodState>& pods, vector<PodState>& enemyPods) {
    int totalCPs = race.totalCPCount();
    if(pods[0].passedCheckpoints == totalCPs) {
        return maxScore;
    } else if(enemyPods[0].passedCheckpoints == totalCPs) {
        return minScore;
    }

    Checkpoint& nextCP = race.checkpoints[pods[0].nextCheckpoint];
    float racerScore = pods[0].passedCheckpoints * 50000 - (nextCP.pos - pods[0].pos).getLength();
    racerScore -= abs(physics.turnAngle(pods[0], nextCP.pos));
//    Checkpoint& enemyNextCP = race.checkpoints[enemyPods[0].nextCheckpoint];
//    float enemyRacerScore = enemyPods[0].passedCheckpoints * 5000 - (enemyNextCP.pos - enemyPods[0].pos).getLength();


//    float chaserScore = -(enemyNextCP.pos - pods[1].pos).getLength();
//    chaserScore -= (pods[0].pos - pods[1].pos).getLength();
//    chaserScore -= physics.turnAngle(pods[1], enemyPods[0].pos);

    return -racerScore;
}

void AnnealingBot::simulate(vector<PodState>& pods1, SimBot* pods1Sim, vector<PodState>& pods2, SimBot* pods2Sim, int turns) {
    vector<PodState*> allPods;
    allPods.push_back(&pods1[0]);
    allPods.push_back(&pods1[1]);
    allPods.push_back(&pods2[0]);
    allPods.push_back(&pods2[1]);
    for(int i = 0; i < turns; i++) {
        pods1Sim->move(pods1, pods2);
        pods2Sim->move(pods2, pods1);
        physics.simulate(allPods);
        physics.orderByProgress(pods1);
        physics.orderByProgress(pods2);
        // Need to check for game over && make sure lead pod is in pos 0.
    }
}
