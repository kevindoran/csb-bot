#include "gtest/gtest.h"
#include "InputParser.h"

#include "AnnealingBot.h"

using namespace std;

class DuelBotTest : public ::testing::Test {
protected:
    /*
 * x  x  x  x  A2
 * x  A1 B1 x  B2
 * x  x  x  C1 x
 * x  C2 x  x  x
 *
*/
    string inputStr = "3\n2\n" // 3 laps, 2 checkpoints.
            "3\n2\n" // cp 1
            "1\n3\n" // cp 2
            //x y vx vy angle next checkpoint
            "1000 1000 0 0 90 0\n" //  player 1 pod 1
            "4000 0   0 0 0  0\n" //  player 1 pod 2
            "2000 1000 0 0 90 0\n" //  player 2 pod 1
            "4000 1000 0 0 45 0\n"; //  player 2 pod 2

    Race r;
    GameState gs;
    vector<PlayerState> playerStates;

    DuelBotTest() {
        istringstream stream(inputStr);
        InputParser ip(stream);
        r = ip.init();
        PlayerState playerStates[PLAYER_COUNT];
        ip.parseTurn(playerStates);
        gs = GameState(r, playerStates, 0);
    }
};

TEST_F(DuelBotTest, annealing_bot_test) {
    AnnealingBot bot(r);
    for(int i = 0; i < 50; i++) {
        cout << "#" << i << endl;
        PairOutput control = bot.move(gs);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
