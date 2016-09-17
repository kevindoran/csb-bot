
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


Race race1(3, {Vector(6271,7739),Vector(14099,7732),Vector(13893,1242),Vector(10252,4891),Vector(6115,2174),Vector(3002,5192)}); // Large zigzag.
Race race2(3, {Vector(9114,1850), Vector(4995,5264), Vector(11502,6107)}); // Medium-small circle
Race race3(3, {Vector(12703,7107), Vector(4080,4634), Vector(13062,1891), Vector(6545,7845), Vector(7473,1372)}); // Large crossed circle


void gameRunner(Simulation sim, ScoreFactors sf, double* ans) {
    *ans = sim.fullGameParamSim(sf, false);
}

float runMultiGame(ScoreFactors sf) {
    Simulation sims[3] = {Simulation(race1), Simulation(race2), Simulation(race3)};
    thread workers[3];
    double scores[3] = {0};
    for(int i = 0; i < 3; i++) {
        workers[i] = thread(gameRunner, sims[i], sf, &scores[i]);
    }
    for(int i = 0; i < 3; i++) {
        workers[i].join();
    }
    double score = 0.0f;
    for(int i = 0; i < 3; i++) {
        score += scores[i];
    }
    return score/3.0f;
}

GameHistory runFullGameTest(ScoreFactors sf) {
    Simulation sim(race1);
    sim.fullGameParamSim(sf, true);
    return sim.history;
}

GameHistory testGame(ScoreFactors sf) {
    Race r1(1, {Vector(10000, 5000), Vector(0,5000)});
    PodState racer1;
    racer1.pos = Vector(0,5000);
    racer1.angle = 0;
    racer1.vel = Vector(0,0);
    racer1.nextCheckpoint = 0;
    racer1.passedCheckpoints = 1; // So that the game ends after the first CP.

    PodState racer2;
    racer2.pos = Vector(-200000, -200000);
    racer2.vel = Vector(0, 0);

    PodState bouncer1;
    bouncer1.pos = Vector(-400000, -400000);
    bouncer1.vel = Vector(0, 0);
    bouncer1.passedCheckpoints = 1;
    bouncer1.nextCheckpoint = 1;

    PodState bouncer2;
    bouncer2.pos = Vector(6000, 8000);
    bouncer2.vel = Vector(0, 0);
    bouncer2.passedCheckpoints = -1; // So that pod 2 acts as a bouncer.
    bouncer2.nextCheckpoint = 1;
    PodState aPods[] = {racer1, racer2};
    PodState bPods[] = {bouncer1, bouncer2};

    Simulation sim(r1);
    sim.parameterSim(aPods, bPods, sf, true);
    return sim.history;
}

float runGame(ScoreFactors sf) {
    Race r1(1, {Vector(10000,5000), Vector(0, 5000)});
    PodState racer1;
    racer1.pos = Vector(0,5000);
    racer1.angle = 0;
    racer1.vel = Vector(0,0);
    racer1.nextCheckpoint = 0;
    racer1.passedCheckpoints = 1; // So that the game ends after the first CP.

    PodState racer2;
    racer2.pos = Vector(-200000, -200000);
    racer2.vel = Vector(0, 0);

    PodState bouncer1;
    bouncer1.pos = Vector(-400000, -400000);
    bouncer1.vel = Vector(0, 0);
    bouncer1.passedCheckpoints = 1;
    bouncer1.nextCheckpoint = 1;

    PodState bouncer2;
    bouncer2.pos = Vector(6000, 8000);
    bouncer2.vel = Vector(0, 0);
    bouncer2.passedCheckpoints = -1; // So that pod 2 acts as a bouncer.
    bouncer2.nextCheckpoint = 1;

    PodState aPods[] = {racer1, racer2};
    PodState bPods[] = {bouncer1, bouncer2};

    Simulation sim(r1);
    int turns = sim.parameterSim(aPods, bPods, sf, false);
    return turns;
}


string printScoreFactors(ScoreFactors sf) {
    stringstream out;
    out << "Overall racer: " << sf.overallRacer << endl;
    out << "Pass CP bonus: " << sf.passCPBonus << endl;
    out << "Progress to CP: " << sf.progressToCP << endl;
    out << "Enemy progress: " << sf.enemyProgress << endl;
    out << "Tangent vel bonus: " << sf.tangentVelBonus << endl;
    out << "Overall bouncer: " << sf.overallBouncer << endl;
    out << "Enemy dist: " << sf.enemyDist << endl;
    out << "Enemy dist to CP: " << sf.enemyDistToCP << endl;
    out << "Bouncer dist to CP: " << sf.bouncerDistToCP << endl;
    out << "Angle seen by checkpoint:" << sf.angleSeenByCP << endl;
    out << "Angle seen by enemy: " << sf.angleSeenByEnemy << endl;
    out << "Bouncer turn angle: " << sf.bouncerTurnAngle << endl;
    out << "Enemy turn angle: " << sf.enemyTurnAngle << endl;
    out << "Checkpoint penalty: " << sf.checkpointPenalty << endl;
    out << "Shield penalty: " << sf.shieldPenalty << endl;
    out << endl;
    return out.str();
}

ScoreFactors generateScoreFactor() {
    ScoreFactors sf;
    // Disable racer.
    sf.overallRacer = 1;
    sf.passCPBonus = rand() % (6000 + 1);
    sf.progressToCP = (2.0 * (float) rand()) / RAND_MAX;
    sf.enemyProgress = -(1.0 * (float) rand()) / RAND_MAX;
    sf.tangentVelBonus = (200 * (float) rand()) / RAND_MAX;
    sf.overallBouncer = 1;
    sf.enemyDist        = -(2.0 * (float) rand()) / RAND_MAX;
    sf.enemyDistToCP    =  (2.0 * (float) rand()) / RAND_MAX;
    sf.bouncerDistToCP  = -(2.0 * (float) rand()) / RAND_MAX;
    sf.angleSeenByEnemy = -(2.0 * (float) rand()) / RAND_MAX;
    sf.angleSeenByCP    = -(2.0 * (float) rand()) / RAND_MAX;
    sf.bouncerTurnAngle = -(2.0 * (float) rand()) / RAND_MAX;
    sf.enemyTurnAngle   = -(2.0 * (float) rand()) / RAND_MAX;
    sf.checkpointPenalty = -(rand() % (6000 + 1));
    sf.shieldPenalty = -(rand() % (1000 + 1));
    return sf;
}

ScoreFactors testScoreFactors() {
    ScoreFactors sf = {};
    sf.overallRacer = 1;
    sf.passCPBonus = 2000;
    sf.progressToCP = 1.0;
    sf.enemyProgress = -0.4;
    sf.tangentVelBonus = 40;
    sf.overallBouncer = 1;
    sf.enemyDist = -0.0522;
    sf.enemyDistToCP = 0.359;
    sf.bouncerDistToCP = -1.82;//-0.925;
    sf.enemyTurnAngle = -0.67;//.0670;
    sf.bouncerTurnAngle = -1.897;//.03651;
    sf.angleSeenByEnemy = -0.628;//.1214;
    sf.angleSeenByCP = -1.82;//.1246;
    sf.checkpointPenalty = -3644;
    sf.shieldPenalty = -600;
    return sf;
}

ScoreFactors startingSFs() {
    ScoreFactors sf = {};
    sf.overallRacer = 1;
    sf.passCPBonus = 4276;
    sf.progressToCP = 1.91;
    sf.enemyProgress = -0.932;
    sf.tangentVelBonus = 40;
    sf.overallBouncer = 1;
    sf.enemyDist = -0.079;
    sf.enemyDistToCP = 1.41;
    sf.bouncerDistToCP = -1.07;//-0.925;
    sf.enemyTurnAngle = -0.669;//.0670;
    sf.bouncerTurnAngle = -0.785;//.03651;
    sf.angleSeenByEnemy = -1.42;//.1214;
    sf.angleSeenByCP = -0.025;//.1246;
    sf.checkpointPenalty = -4031;
    sf.shieldPenalty = -89;
    return sf;
}

ScoreFactors randomAlter(ScoreFactors sf) {
    float sw = (float) rand() / RAND_MAX;
    float flip = (float) rand() / RAND_MAX;
    if(sw < 1.0/11.0) {
        sf.passCPBonus = rand() % (6000 + 1);
    } else if(sw < 2.0/13.0) {
        sf.progressToCP = (2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 3.0/13.0) {
        sf.enemyProgress = -(1.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 4.0/13.0) {
        sf.tangentVelBonus = (200 * (float) rand()) / RAND_MAX;
    } else if(sw < 5.0/13.0) {
        sf.enemyDist    =     -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 6.0/13.0) {
        sf.enemyDistToCP    =  (2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 7.0/13.0) {
        sf.bouncerDistToCP  = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 8.0/13.0) {
        sf.angleSeenByEnemy = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 9.0/13.0) {
        sf.angleSeenByCP    = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 10.0/13.0) {
        sf.bouncerTurnAngle = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 11.0/13.0) {
        sf.enemyTurnAngle = -(2.0 * (float) rand()) / RAND_MAX;
    } else if(sw < 12.0/13.0) {
        sf.checkpointPenalty = -(rand() % (6000 + 1));
    } else if(sw < 13.0/13.0) {
        sf.shieldPenalty = -(rand() % (1000 + 1));
    }
    return sf;
}


/**
 * @param kthSD standard deviation of the cost function during the previous temperature
 * @param lambda constant between 0 and 1
 */
double nextTemperature(double currentTemp, double kthSD, double lambda) {
    return currentTemp * exp((-lambda * currentTemp) / kthSD);
}

struct Job {
    bool shutdown;
    ScoreFactors config;
    double temp;
    double currentScore;
};

struct Result {
    ScoreFactors config;
    double score;
    bool accepted;
};


BlockingQueue<Result> resultQueue;
BlockingQueue<Job> jobQueue;


void worker() {
    cerr << "Thread #" << this_thread::get_id() << " starting up." << endl;
    while(true) {
        // Blocking call.
        Job j = jobQueue.pop();
        if(j.shutdown) {
            cerr << "Thread #" << this_thread::get_id() << " shutting down." << endl;
            return;
        }
        cerr << "Thread #" << this_thread::get_id() << " starting job." << endl;
        double score = runGame(j.config);
        double scoreDiff = -(score - j.currentScore);
        double flip = (float) drand48() / RAND_MAX;
        double acc = exp(-scoreDiff / j.temp);
        bool accepted = scoreDiff < 0 || acc > flip;
        Result res = {
                j.config,
                score,
                accepted};
        resultQueue.push(res);
        cerr << "Thread #" << this_thread::get_id() << " finished job." << endl;
    }
}

void multiGameWorker() {
    cerr << "Thread #" << this_thread::get_id() << " starting up." << endl;
    while(true) {
        // Blocking call.
        Job j = jobQueue.pop();
        if(j.shutdown) {
            cerr << "Thread #" << this_thread::get_id() << " shutting down." << endl;
            return;
        }
        cerr << "Thread #" << this_thread::get_id() << " starting job." << endl;
        double score = runMultiGame(j.config);
        double scoreDiff = -(score - j.currentScore);
        double flip = (float) drand48() / RAND_MAX;
        double acc = exp(-scoreDiff / j.temp);
        bool accepted = scoreDiff < 0 || acc > flip;
        Result res = {
                j.config,
                score,
                accepted};
        resultQueue.push(res);
        cerr << "Thread #" << this_thread::get_id() << " finished job. Score: " << score << endl;
    }
}

//const int WORKER_COUNT = 31;
const int WORKER_COUNT = 11;

const int K = 5;
// Estimated mean and neighborhood size.
constexpr double initialSD = 1584;//2000;//80;//500;//0.5;
const int neighorhoodSize = 150; // 320
const int tempReductions = 600;//80;//200;
constexpr double lambda = 0.4f;
constexpr double initialTemp = 600;//K*initialSD;
constexpr double smoothingFactor = 0.7;

float finalScore = 0;
ScoreFactors optimize() {
    // Start up the workers.
    vector<thread> workers;
    for(int i = 0; i < WORKER_COUNT; i++) {
        workers.push_back(thread(multiGameWorker));
    }
    ScoreFactors current = AnnealingBot().sFactors;//startingSFs();
    double currentScore = runMultiGame(current);
    double bestScore = currentScore;
    ScoreFactors bestFactors = current;
    double prevTemp = initialTemp;
    double temp = initialTemp;
    double sdPrev = initialSD;
    for(int i = 0; i < tempReductions; i++) {
        int acceptCount = 0;
        int count = 1;
        double mean = currentScore;
        double delta = currentScore - 0.0;
        double M2 = 0.0 + delta*(currentScore-mean);
        vector<Result> accepted;
        for(int j = 0; j < neighorhoodSize;) {
            // Feed the workers.
            for(int y = 0; y < WORKER_COUNT; y++) {
                ScoreFactors altered = randomAlter(current);
                Job job = {false, altered, temp, currentScore};
                jobQueue.push(job);
            }
            for(int z = 0; z < WORKER_COUNT && j < neighorhoodSize; z++) {
                Result res = resultQueue.pop();
                if(res.accepted) {
                    acceptCount++;
                    cerr << "Master accepted update." << endl;
                    accepted.push_back(res);
                }
                // Start up more jobs until one is accepted.
                if(accepted.empty()) {
                    ScoreFactors altered = randomAlter(current);
                    Job job = {false, altered, temp, currentScore};
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
                int randomIdx = (int) (drand48() * accepted.size());
                current = accepted[randomIdx].config;
                currentScore = accepted[randomIdx].score;
                if(currentScore > bestScore) {
                    bestScore = currentScore;
                    bestFactors = current;
                }
                accepted.clear();
            }
            cerr << "Round " << j << " of " << i << endl;
        }
        cerr << "M2: " << M2 << "  count: " << count << endl;
        double sd = sqrt(M2) / (count - 1);
        // Minus 1 from count when calculating accept ratio as the count includes the starting score/config.
        cerr << "Accept %:" << (double)acceptCount / (double)(count-1) << "  at temperature: " << temp << "   and SD: " << sd << endl;
        sd = (1-smoothingFactor) * sd + smoothingFactor * sdPrev * (temp / prevTemp);
        sdPrev = sd;
        prevTemp = temp;
        temp = nextTemperature(temp, sd, lambda);
        cerr << "Current score: " << currentScore << endl;
        cerr << "Current score factors: " << endl << printScoreFactors(current) << endl;
        cerr << "Best score: " << bestScore << endl;
        cerr << "Best factors: " << endl << printScoreFactors(bestFactors) << endl;
    }
    for(int i = 0; i < WORKER_COUNT; i++) {
        jobQueue.push({true, 0, 0, 0});
    }
    for(int i = 0; i < WORKER_COUNT; i++) {
        workers[i].join();
    }
    finalScore = currentScore;
    return current;
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
    // Optimizing
    srand48(time(0));
    ScoreFactors finalSF = optimize();
    cout << "Final score: " << endl << finalScore << endl;
    cout << "Final score factors: " << endl << printScoreFactors(finalSF) << endl;

    // Testing
//    GameHistory gh = runFullGameTest(testScoreFactors());
//    *out << "gameData = ";
//    gh.writeToStream(*out);
//    *out << ";";
}
