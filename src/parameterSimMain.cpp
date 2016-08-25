
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <ctime>
#include <math.h>

#include "Simulation.h"
#include "BlockingQueue.h"

// Generate random position near (10000,0).
//PodState generateRacerPos() {
//    float radius = ((float) rand()) % (3000 + 1);
//    float angle = ((float) rand()) % (M_PI*2 + 1);
//    float x = 10000 + radius * cos(angle);
//    float y = radius * sin(angle);
//    float turnAngle = (())
//    return 0; // tofinish
//}


int runGame(ScoreFactors sf) {
    Race r1(1, {Vector(0,0), Vector(10000, 0)});
    PodState racer1;
    racer1.pos = Vector(0,0);
    racer1.angle = 0;
    racer1.vel = Vector(0,0);
    racer1.nextCheckpoint = 1;

    PodState racer2;
    racer2.pos = Vector(-200000, -200000);

    PodState bouncer1;
    bouncer1.pos = Vector(-200000, -200000);

    PodState bouncer2;
    bouncer2.pos = Vector(6000, 3000);
    PodState* pods[] = {&racer1, &racer2, &bouncer1, &bouncer2};

    Simulation sim(r1);
    int turns = sim.parameterSim(pods, sf, false);
    return turns;
}

ScoreFactors generateScoreFactor() {
    ScoreFactors sf;
    // Disable racer.
    sf.overallRacer = 0;
    sf.overallBouncer = 1;
    sf.enemyDist        = -(2.0 * (float) rand()) / RAND_MAX;
    sf.enemyDistToCP    =  (2.0 * (float) rand()) / RAND_MAX;
    sf.bouncerDistToCP  = -(2.0 * (float) rand()) / RAND_MAX;
    sf.angleSeenByEnemy = -(2.0 * (float) rand()) / RAND_MAX;
    sf.angleSeenByCP    = -(2.0 * (float) rand()) / RAND_MAX;
    sf.bouncerTurnAngle = -(2.0 * (float) rand()) / RAND_MAX;
    sf.enemyTurnAngle   = -(2.0 * (float) rand()) / RAND_MAX;
    sf.checkpointPenalty = ((float) rand()) % (6000 + 1);
}

ScoreFactors randomAlter(ScoreFactors sf) {
    float sw = (float) rand() / RAND_MAX;
    float flip = (float) rand() / RAND_MAX;
    if(sw < 1.0/8.0) {
        sf.enemyDist        = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 2.0/8.0) {
        sf.enemyDistToCP    =  (2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 3.0/8.0) {
        sf.enemyDistToCP    =  (2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 4.0/8.0) {
        sf.bouncerDistToCP  = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 5.0/8.0) {
        sf.angleSeenByEnemy = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 6.0/8.0) {
        sf.angleSeenByCP    = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 7.0/8.0) {
        sf.bouncerTurnAngle = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 8.0/8.0) {
        sf.checkpointPenalty = ((float) rand()) % (6000 + 1);
    }
    return sf;
}


/**
 * @param kthSD standard deviation of the cost function during the previous temperature
 * @param lambda constant between 0 and 1
 */
float nextTemperature(float currentTemp, float kthSD, float lambda) {
    return exp(-lambda * currentTemp / kthSD);
}

struct Job {
    ScoreFactors config;
    float temp;
    float currentScore;
};

struct Result {
    ScoreFactors config;
    float score;
    bool accepted;
};


BlockingQueue<Result> resultQueue;
BlockingQueue<Job> jobQueue;


void worker() {
    while(true) {
        // Blocking call.
        Job j = jobQueue.pop();
        int score = runGame(j.config);
        double scoreDiff = -(score - j.currentScore);
        double flip = (float) drand48() / RAND_MAX;
        double acc = exp(-scoreDiff / j.temp);
        bool accepted = scoreDiff < 0 || acc > flip;
        Result res = {
                j.config,
                score,
                accepted};
        resultQueue.push(res);
    }
}

const int WORKER_COUNT = 30;

const int K = 10;
// Estimated mean and neighborhood size.
const float mean = 20000;
const int neighorhoodSize = 160; // 320
const tempReductions = 200;
const lambda = 0.3;
constexpr float initialTemp = K*mean;
const smoothingFactor = 0.8;

ScoreFactors optimize() {
    ScoreFactors current = generateScoreFactor();
    float currentScore = runGame(current);
    float prevTemp = -1;
    float temp = initialTemp;
    float sdPrev = -1;
    for(int i = 0; i < tempReductions; i++) {
        int count = 0;
        double mean = 0.0;
        double delta = 0.0;
        double M2 = 0.0;
        // Feed the workers.
        for(int y = 0; y < WORKER_COUNT; y++) {
            ScoreFactors altered = randomAlter(current);
            Job job = {altered, temp, currentScore};
            jobQueue.push(job);
        }
        vector<Result> accepted;
        for(int j = 0; j < neighorhoodSize;) {
            for(int z = 0; z < WORKER_COUNT && j < neighorhoodSize; z++) {
                Result res = resultQueue.pop();
                if(res.accepted) {
                    accepted.push_back(res);
                }
                if(accepted.empty()) {
                    ScoreFactors altered = randomAlter(current);
                    Job job = {altered, temp, currentScore};
                    jobQueue.push(job);
                }
                // Online mean & variance.
                count++;
                delta = res.score - mean;
                mean += delta/count;
                M2 += delta*(res.score - mean);
                // Increment trial counter.
                j++;
            }
            // Choose random success, if any. Random produces better results than first in.
            if(!accepted.empty()) {
                int randomIdx = (int) drand48() * accepted.size();
                current = accepted[randomIdx].config;
                currentScore = accepted[randomIdx].score;
            }
        }
        float sd = sqrt(M2) / (count - 1);
        sd = sdPrev == -1 ? sd : (1-smoothingFactor) * sd + smoothingFactor * sdPrev * (temp / prevTemp);
        sdPrev = sd;
        prevTemp = temp;
        temp = nextTemperature(temp, sd, lambda);
    }
}


GameHistory testGame(ScoreFactors sf) {
    Race r1(1, {Vector(0,0), Vector(10000, 0)});
    PodState racer1;
    racer1.pos = Vector(0,0);
    racer1.angle = 0;
    racer1.vel = Vector(0,0);
    racer1.nextCheckpoint = 1;

    PodState racer2;
    racer2.pos = Vector(-200000, -200000);

    PodState bouncer1;
    bouncer1.pos = Vector(-200000, -200000);

    PodState bouncer2;
    bouncer2.pos = Vector(6000, 3000);
    PodState* pods[] = {&racer1, &racer2, &bouncer1, &bouncer2};

    Simulation sim(r1);
    sim.parameterSim(pods, sf, true);
    return sim.history;
}

int main(int argc, char* argv[]) {
    // Setup io
    ostream *out;
    ofstream fout;
    if (argc > 1) {
        fout.open(argv[1]);
        out = &fout;
    } else {
        out = &cout;
    }

    // Testing
    GameHistory gh = testGame(generateScoreFactor());
    gh.writeToStream(*out);

    // Run Sims
//    srand48(time(0));

}
