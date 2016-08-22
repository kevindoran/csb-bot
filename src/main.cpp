#include <cstdlib>
#include <ctime>
#include <stdlib.h>
#include "State.h"
#include "InputParser.h"
#include "PodracerBot.h"
#include "Physics.h"
#include "AnnealingBot.h"
#include <chrono>

int main() {
    InputParser inputParser(cin);
    Race race = inputParser.init();
    State state(race);
    Physics physics(race);
    std::srand(std::time(0));

    // Game loop.
    while (1) {
        long long startTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        AnnealingBot bot(race, 150);
        PlayerState players[PLAYER_COUNT];
        inputParser.parseTurn(players);
        state.preTurnUpdate(players);

        int leadPodID = physics.leadPodID(players[0].pods);
        
        PairOutput control = bot.move(state.game());
        PodOutputAbs po1 = control.o1.absolute(state.game().ourState().pods[0]);
        PodOutputAbs po2 = control.o2.absolute(state.game().ourState().pods[1]);
        cout << po1.toString() << endl
             << po2.toString() << endl;
        state.postTurnUpdate(po1, po2);
        long long endTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        cerr << "Runtime: " << endTime-startTime << endl;
//
//        int leadPodID = state.game().ourState().leadPodID;
//        Racer racer = Racer();
//        Bouncer bouncer = Bouncer();
//        PodOutputAbs po1 = 0 == leadPodID ? racer.move(state.game(), 0) : bouncer.move(state.game(), 0);
//        PodOutputAbs po2 = 1 == leadPodID ? racer.move(state.game(), 1) : bouncer.move(state.game(), 1);
//        cout << po1.toString() << endl << po2.toString() << endl;
//        state.postTurnUpdate(po1, po2);
    }
}