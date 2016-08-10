#include "State.h"
#include "InputParser.h"
#include "PodracerBot.h"

int main() {
    InputParser inputParser(cin);
    Race race = inputParser.init();
    State state(race);

    // Game loop.
    while (1) {
        vector<PlayerState> players = inputParser.parseTurn();
        state.preTurnUpdate(players);
        int leadPodID = state.game().ourState().leadPodID;
        MinimalBot racer = MinimalBot();
        Bouncer bouncer = Bouncer();
        PodOutput po1 = 0 == leadPodID ? racer.move(state.game(), 0) : bouncer.move(state.game(), 0);
        PodOutput po2 = 1 == leadPodID ? racer.move(state.game(), 1) : bouncer.move(state.game(), 1);
        cout << po1.toString() << endl << po2.toString() << endl;
        state.postTurnUpdate(po1, po2);
    }
}