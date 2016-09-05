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
    AnnealingBot<6> bot(r);
    for(int i = 0; i < 50; i++) {
        cout << "#" << i << endl;
        PairOutput control = bot.move(gs);
    }
}

TEST(DuelBotTestNoFixture, annealing_bot_score) {
    Race race(3, {Vector(0,0), Vector(15000,0), Vector(17000, 10000)});
    AnnealingBot<6> bot(race);
    PodState p1pod1Pre(Vector(5000, 0), Vector(0,0), 0, 1);
    PodState p1pod2Pre(Vector(5000, 0), Vector(0,0), 0, 1);
    PodState p2pod1Pre(Vector(5000, 0), Vector(0,0), 0, 1);
    PodState p2pod2Pre(Vector(5000, 0), Vector(0,0), 0, 1);

    // Option 1 (Should be lower score)
    PodState p1pod1(Vector(15000-POD_RADIUS, 0), Vector(0, 0), 0, 1);
    PodState p1pod2(Vector(0, 0), Vector(0, 0), 0, 1);
    PodState p2pod1(Vector(7000, 0), Vector(0, 0), 0, 1);
    PodState p2pod2(Vector(8000, 0), Vector(0, 0), 0, 1);

    // Option 2
    PodState p1pod12(Vector(15001-POD_RADIUS, 0), Vector(0, 0), 0, 2);

    const PodState* podsPre[] = {&p1pod1Pre, &p1pod2Pre};
    const PodState* enemyPodsPre[] = {&p2pod1Pre, &p2pod2Pre};
    const PodState* enemyPods1[] = {&p2pod1, &p2pod2};
    const PodState* pods1[] = {&p1pod1, &p1pod2};
    const PodState* pods2[] = {&p1pod12, &p1pod2};

    float outsideCP = bot.score(pods1, podsPre, enemyPods1, enemyPodsPre);
    float insideCP = bot.score(pods2, podsPre, enemyPods1, enemyPodsPre);
    ASSERT_GT(outsideCP, insideCP);
}


TEST(DuelBotTestNoFixture, annealing_bot_progess) {
    Race race(3, {Vector(0,0), Vector(15000, 0)});
    AnnealingBot<6> bot(race);
    PodState prev;
    PodState cur;
    float prevScore = -1000;
    for(int x = 0; x <= 15000; x++) {
        prev = PodState(Vector(x-400-500, 0), Vector(0, 0), 0, 1);
        cur = PodState(Vector(x-400, 0), Vector(0, 0), 0, 1);
        ASSERT_GT(bot.progress(&cur, &prev), prevScore);
    }
    prev = PodState(Vector(15000-400-100, 0), Vector(0, 0), 0, 1);
    cur = PodState(Vector(15001-400, 0), Vector(0, 0), 0, 0);
    ASSERT_GT(bot.progress(&cur, &prev), prevScore);
    prev = PodState(Vector(15000-400-50, 0), Vector(0, 0), 0, 1);
    cur = PodState(Vector(15100-400, 0), Vector(0, 0), 0, 0);
    ASSERT_GT(bot.progress(&cur, &prev), prevScore);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
