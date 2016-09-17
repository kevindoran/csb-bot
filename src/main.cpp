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
    State enemyState(race);
    Physics physics(race);
    std::srand(std::time(0));

    AnnealingBot<6> bot(race, 108);
//    AnnealingBot<4> botFake(race, 30);
    AnnealingBot<4> enemyBot(race, 39);
    // Game loop.
    while (1) {
        long long startTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        PlayerState players[PLAYER_COUNT];
        inputParser.parseTurn(players);
        state.preTurnUpdate(players);
        // Train opponent.
        PairOutput enemySolution[4];
        PodState ourStateExpectedByEnemy[4][2];
        enemyBot.train(state.game().enemyState().pods, state.game().ourState().pods, enemySolution, ourStateExpectedByEnemy[0], true);
        static int startFromTurn = 0;
        CustomAIWithBackup<4> enemyAI(race, enemySolution, ourStateExpectedByEnemy, startFromTurn);
        enemyAI.setDefaultAfter(4);

//        PairOutput ourFirstSol[4];
//        AnnealingBot<4> ourFirstBot(race, 20);
//        ourFirstBot.train(state.game().ourState().pods, state.game().enemyState().pods, ourFirstSol);
//        CustomAIWithBackup fakeBotAI(race, ourFirstSol, 0);
//        fakeBotAI.setDefaultAfter(4);
//
//        enemyBot.setEnemyAI(&fakeBotAI);
//        PairOutput enemySolution[5];
//        enemyBot.train(state.game().enemyState().pods, state.game().ourState().pods, enemySolution);
//        CustomAIWithBackup enemyAI(race, enemySolution, 0);
//        enemyAI.setDefaultAfter(5);


        // Train our bot.
        bot.setEnemyAI(&enemyAI);
        PairOutput control = bot.move(state.game());
        PodOutputAbs po1 = control.o1.absolute(state.game().ourState().pods[0]);
        PodOutputAbs po2 = control.o2.absolute(state.game().ourState().pods[1]);
        cout << po1.toString() << endl
             << po2.toString() << endl;
        state.postTurnUpdate(po1, po2);
        long long endTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
        cerr << "Runtime: " << endTime-startTime << endl;
    }
}