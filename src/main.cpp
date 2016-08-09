//
// Created by Kevin on 1/08/2016.
//

#include "InputParser.h"
#include "podracerBot.h"
#include "State.h"
#include <limits>

int main()
{
    // game loop
    InputParser inputParser(cin);
    Race race = inputParser.init();
    State state(race);

    while (1) {
        vector<PlayerState> players = inputParser.parseTurn();
        state.preTurnUpdate(players);
//        cerr << "Pod1: pos " << state.game().ourState().pods[1].pos << "  vel " << state.game().ourState().pods[0].vel << endl;
//        cerr << "Pod2: pos " << state.game().ourState().pods[1].pos << "  vel " << state.game().ourState().pods[1].vel << endl;
        int leadPodID = state.game().ourState().leadPodID;
        cerr << "Lead ID: " << leadPodID << endl;
        MinimalBot racer = MinimalBot();
        Bouncer bouncer = Bouncer();
        PodOutput po1 = 0 == leadPodID ? racer.move(state.game(), 0) : bouncer.move(state.game(), 0);
        PodOutput po2 = 1 == leadPodID ? racer.move(state.game(), 1) : bouncer.move(state.game(), 1);
        cout << po1.toString() << endl << po2.toString() << endl;
        state.postTurnUpdate(po1, po2);
    }
}