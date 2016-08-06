//
// Created by Kevin on 1/08/2016.
//

#include "gtest/gtest.h"
#include "InputParser.h"

using namespace std;

TEST(InputParseTest, basic) {
        /*
         * x  x  x  x  A2
         * x  A1 B1 x  B2
         * x  x  x  C1 x
         * x  C2 x  x  x
         *
         */
        //
        string inputStr = "3\n2\n" // 3 laps, 2 checkpoints.
                          "3\n2\n" // cp 1
                          "1\n3\n" // cp 2
                         //x y vx vy angle next checkpoint
                          "1 1 0 0 90 0\n" //  player 1 pod 1
                          "4 0 0 0 0  0\n" //  player 1 pod 2
                          "2 1 0 0 90 0\n" //  player 2 pod 1
                          "1 1 0 0 45 0\n"; //  player 2 pod 2
        istringstream stream(inputStr);
        InputParser ip(stream);
        ip.init();
        EXPECT_EQ(3, ip.getRace().laps);
        EXPECT_EQ(2, ip.getRace().checkpoints.size());
        Vector cp1(3, 2);
        Vector cp2(1, 3);
        EXPECT_EQ(cp1, ip.getRace().checkpoints[0].pos);
        EXPECT_EQ(cp2, ip.getRace().checkpoints[1].pos);
        GameState gs = ip.parseGameState();
        Vector p1pod1(1, 1);
        Vector p1pod2(4, 0);
        EXPECT_EQ(p1pod1, gs.ourState().pods[0].pos);
        EXPECT_EQ(p1pod2, gs.ourState().pods[1].pos);
        double angle1 = M_PI / 2;
        EXPECT_DOUBLE_EQ(angle1, gs.ourState().pods[0].angle);
}

