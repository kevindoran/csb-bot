//
// Created by Kevin on 1/08/2016.
//

#include "gtest/gtest.h"
#include "InputParser.h"

using namespace std;

class InputParserTest : public ::testing::Test {
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
                      "1 1 0 0 90 0\n" //  player 1 pod 1
                      "4 0 0 0 0  0\n" //  player 1 pod 2
                      "2 1 0 0 90 0\n" //  player 2 pod 1
                      "4 1 0 0 45 0\n"; //  player 2 pod 2

    string inputStr2 =
                      //x y vx vy angle next checkpoint
                      "1 86 0 85 90 1\n" //  player 1 pod 1
                      "89 0 85 0 0  0\n" //  player 1 pod 2
                      "2 86 0 85 90 0\n" //  player 2 pod 1
                      "5 2 1 1 45 0\n"; //  player 2 pod 2


};


TEST_F(InputParserTest, basic) {
        istringstream stream(inputStr);
        InputParser ip(stream);
        Race race = ip.init();
        EXPECT_EQ(3, race.laps);
        EXPECT_EQ(2, race.checkpoints.size());
        Vector cp1(3, 2);
        Vector cp2(1, 3);
        EXPECT_EQ(cp1, race.checkpoints[0].pos);
        EXPECT_EQ(cp2, race.checkpoints[1].pos);
        vector<PlayerState> players = ip.parseTurn();
        Vector p1pod1(1, 1);
        Vector p1pod2(4, 0);
        EXPECT_EQ(p1pod1, players[OUR_PLAYER_ID].pods[0].pos);
        EXPECT_EQ(p1pod2, players[OUR_PLAYER_ID].pods[1].pos);
        float angle1 = M_PI / 2;
        EXPECT_FLOAT_EQ(angle1, players[OUR_PLAYER_ID].pods[0].angle);
}

