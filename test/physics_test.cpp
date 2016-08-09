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
    Physics* physics;

    virtual void TearDown() {
    }

    virtual void SetUp() {
    }

public:
    PhysicsTest() : Test() {
        Checkpoint cp1(1000, 1000, 0);
        Checkpoint cp2(2000, 3000, 1);
        vector<Checkpoint> v{cp1, cp2};
        int laps = 2;
        race = new Race(laps, v);
        physics = new Physics(*race);
        podState = new PodState(3000, 0, 0, 0, 0, 0);
    }

    ~PhysicsTest() {
        delete race;
        delete podState;
        delete physics;
    }
};

TEST_F(PhysicsTest, simpleMove) {
        double acc = 100;
        PodState expected(3085, 0, 85, 0, 0, 0);
        PodOutput po(acc, podState->pos + Vector(1, 0)); // angle = 0.
        PodState afterMove = physics->move(*podState, po, 1);
        EXPECT_EQ(expected.pos, afterMove.pos);
        EXPECT_EQ(expected.vel, afterMove.vel);
        EXPECT_EQ(expected.angle, afterMove.angle);
        EXPECT_EQ(expected.nextCheckpoint, afterMove.nextCheckpoint);
}

TEST_F(PhysicsTest, angleTo) {
    Vector a(200, 200);
    Vector b(200, 400);
    int expectedAngle = M_PI / 2;
    int ans = physics->angleTo(a, b);
    EXPECT_EQ(expectedAngle, ans);

    Vector c(200, 0);
    expectedAngle = M_PI * 3/2;
    ans = physics->angleTo(a, c);
    EXPECT_EQ(expectedAngle, ans);
}

TEST_F(PhysicsTest, expectedControl) {
    Vector pos1(200,200);
    Vector vel1(0,0);
    double angle = 0;
    Vector pos2(285, 200);
    Vector vel2(85, 0);

    PodState ps1(pos1, vel1, angle);
    PodState ps2(pos2, vel2, angle);
    PodOutput expectedControl(100, Vector(300,200));
    double expectedAngle = physics->angleTo(pos1, expectedControl.target);
    PodOutput ans = physics->expectedControl(ps1, ps2);
    double ansAngle = physics->angleTo(pos1, ans.target);
    EXPECT_DOUBLE_EQ(expectedControl.thrust, ans.thrust);
    EXPECT_DOUBLE_EQ(expectedAngle, ansAngle);
}

TEST_F(PhysicsTest, turnAngle) {
    Vector pos(200, 200);
    Vector vel(100, 0);
    double facingAngle = M_PI / 2;
    PodState ps(pos, vel, facingAngle, 0);
    Vector targetA(200, 0); // Directly above, 180deg.
    Vector targetA2(200, 400); // Directly below, 0deg.
    Vector targetB(250, 0); // Above to the east (turn left), ~180deg.
    Vector targetC(150, 0); // Above to the west (turn right), ~180deg.
    Vector targetD(150, 400); // Below to the west (turn right), ~0deg.
    Vector targetE(250, 400); // Below to the east (turn left) ~ 0deg.

    double turnA = physics->turnAngle(ps, targetA);
    double turnA2 = physics->turnAngle(ps, targetA2);
    double turnB = physics->turnAngle(ps, targetB);
    double turnC = physics->turnAngle(ps, targetC);
    double turnD = physics->turnAngle(ps, targetD);
    double turnE = physics->turnAngle(ps, targetE);
    double abs_error = 0.000000001;
    EXPECT_NEAR(M_PI, abs(turnA), abs_error);
    EXPECT_NEAR(0, turnA2, abs_error);
    EXPECT_NEAR(-(M_PI - atan(50.0/200.0)), turnB, abs_error);
    EXPECT_NEAR(M_PI - atan(50.0/200.0), turnC, abs_error);
    EXPECT_NEAR(atan(50.0/200.0), turnD, abs_error);
    EXPECT_NEAR(-atan(50.0/200.0), turnE, abs_error);
}

//TEST_F(PhysicsTest, turnAngleRealExample) {
//    Vector pos(1063, 4163);
//    Vector target(7088, 3023);
//    Vector vel(-14, -2);
//    Vector controlDir(7088.5, 3023.5);
//}

TEST_F(PhysicsTest, isCollision) {
    // Pod at (0,0) moving at ~250 east and pod at (1200, 0) moving at ~250 west.
    Vector posA(0,0);
    Vector velA(200,0);
    double angleA = 0;
    PodState psA(posA, velA, angleA, 0);
    PodOutput controlA(100, posA + Vector(1,0));
    Vector posB(1200, 0);
    Vector velB(-200, 0);
    double angleB = M_PI;
    PodState psB(posB, velB, angleB, 0);
    PodOutput controlB(100, posB + Vector(-1,0));
    double velThreshold = 0; // Just testing any collision.
    bool isCollision = physics->isCollision(psA, controlA, psB, controlB, velThreshold);
    EXPECT_TRUE(isCollision);

    // Just out of reach; (v1 + v2 + 2*POD_RADIUS = 1300).
    posB = Vector(1400, 0);
    psB = PodState(posB, velB, angleB, 0);
    isCollision = physics->isCollision(psA, controlA, psB, controlB, velThreshold);
    EXPECT_FALSE(isCollision);
}
