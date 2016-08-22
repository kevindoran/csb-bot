#include <iostream>
#include <fstream>

#include "Simulation.h"
#include "AnnealingBot.h"
#include "PodracerBot.h"

Race r1(3, {Vector(1000, 340), Vector(11200, 2400), Vector(14200, 7700), Vector(5400, 8700)});

int main(int argc, char* argv[]) {
    ostream* out;
    ofstream fout;
    if(argc > 1) {
        fout.open(argv[1]);
        out = &fout;
    } else {
        out = &cout;
    }
    DuelBot* b1 = new AnnealingBot(r1, 140);
//    DuelBot* b1 = new TraditionalBot();
    DuelBot* b2 = new TraditionalBot();
    Simulation sim(r1);
    GameHistory gh = sim.simulate(b1, b2);
    *out << "gameData = ";
    gh.writeToStream(*out);
    *out << ";";
}
