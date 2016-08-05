//
// Created by Kevin on 2/08/2016.
//

#include "gtest/gtest.h"

#include "Physics.h"

using namespace std;

class PhysicsTest : public ::testing::Test {
protected:

    /**
     * x  x  x  P1 x  x
     * x  C1 x  x  x  x
     * x  x  x  x  x  x
     * x  x  C2 x  x  x
     * x  x  x  x  x  x
     *
     * P1 dir: up
     * P2 dir: left
     */
    Race *race;
    PodState *podState;
    Physics physics;

    virtual void TearDown() {
        delete race;
        delete podState;
    }

    virtual void SetUp() {
        Checkpoint cp1(1000, 1000, 0);
        Checkpoint cp2(2000, 3000, 1);
        vector<Checkpoint> v{cp1, cp2};
        int laps = 2;
        race = new Race(laps, v);
        podState = new PodState(3000, 0, 0, 0, 0, 0);
    }

public:
    PhysicsTest() : Test() {
    }
};

TEST_F(PhysicsTest, simpleMove) {
        double acc = 100;
        double angle = 0;
        PodState expected(3085, 0, 85, 0, 0, 0);
        PodState afterMove = physics.move(*race, *podState, acc, angle, 1);
        EXPECT_EQ(expected.pos, afterMove.pos);
        EXPECT_EQ(expected.vel, afterMove.vel);
        EXPECT_EQ(expected.angle, afterMove.angle);
        EXPECT_EQ(expected.nextCheckpoint, afterMove.nextCheckpoint);
}
