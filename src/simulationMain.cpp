#include <iostream>
#include <fstream>

#include "Simulation.h"
#include "AnnealingBot.h"
#include "PodracerBot.h"

Race r1(3, {Vector(1000, 340), Vector(11200, 2400), Vector(14200, 7700), Vector(5400, 8700)});
Race r2(3, {Vector(12703,7107), Vector(4080,4634), Vector(13062,1891), Vector(6545,7845), Vector(7473,1372)}); // Large crossed circle
Race r3(3, {Vector(9114,1850), Vector(4995,5264), Vector(11502,6107)}); // Medium-small circle

ScoreFactors sfs = {
        1,    // overallRacer
        2000, // passCPBonus
        1.16,    // progressToCP
        -0.456, // enemyProgress
        1,    // overallBouncer
        -0.4, // enemyDist
        1.17, // enemyDistToCP
        -1.8,  // bouncerDistToCP
        -1.93,  // angleSeenByCP
        -0.331,  // angleSeenByEnemy
        -1.37,  // bouncerTurnAngle
        -1.28,  // enemyTurn angle
        -4000
};

int main(int argc, char* argv[]) {
    ostream* out;
    ofstream fout;
    if(argc > 1) {
        fout.open(argv[1]);
        out = &fout;
    } else {
        out = &cout;
    }
    DuelBot* b1 = new AnnealingBot<6>(r1, 120);
//    DuelBot* b1 = new TraditionalBot();
    DuelBot* b2 = new TraditionalBot();
    Simulation sim(r3);
//    GameHistory gh = sim.simulate(b1, b2);
    sim.fullGameParamSim(sfs, true);
    GameHistory gh = sim.history;
//    *out << "gameData = ";
//    gh.writeToStream(*out);
//    *out << ";";
}
