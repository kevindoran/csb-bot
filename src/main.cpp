#include <cstdlib>
#include <ctime>
#include <stdlib.h>
#include "State.h"
#include "InputParser.h"
#include "PodracerBot.h"
#include "Physics.h"
#include "AnnealingBot.h"

int main() {
    InputParser inputParser(cin);
    Race race = inputParser.init();
    State state(race);
    Physics physics(race);
    AnnealingBot bot;
    bot.init(race);
    std::srand(std::time(0));

    // Game loop.
    while (1) {
        vector<PlayerState> players = inputParser.parseTurn();
        state.preTurnUpdate(players);
        physics.orderByProgress(players[0].pods);
        PairOutput control = bot.move(state.game());
//         Where are the lead/lagging pods ordered.
        PodOutputAbs po1 = control.o1.absolute(state.game().ourState().pods[0]);
        PodOutputAbs po2 = control.o2.absolute(state.game().ourState().pods[1]);
        cout << po1.toString() << endl
             << po2.toString() << endl;
        state.postTurnUpdate(po1, po2);
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