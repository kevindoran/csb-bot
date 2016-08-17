#include <iostream>
#include <fstream>

#include "Simulation.h"
#include "AnnealingBot.h"
#include "PodracerBot.h"

Race r1(3, {Checkpoint(10000, 3400, 0), Checkpoint(11200, 5400, 1), Checkpoint(7200, 6700, 2), Checkpoint(5400, 2700, 3)});

int main(int argc, char* argv[]) {
    ostream* out;
    ofstream fout;
    if(argc > 1) {
        fout.open(argv[1]);
        out = &fout;
    } else {
        out = &cout;
    }
    DuelBot* b1 = new AnnealingBot();
    DuelBot* b2 = new TraditionalBot();
    Simulation sim(r1);
    GameHistory gh = sim.simulate(b1, b2);
    gh.writeToStream(*out);
}
