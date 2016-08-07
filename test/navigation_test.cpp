//
// Created by Kevin on 5/08/2016.
//

#include "gtest/gtest.h"
#include "Navigation.h"

TEST(NavigationTest, seek_thrust_below_max) {
    Race r(1, {});
    Navigation nav(r);
    Vector pos(200, 200);
    Vector vel(100, 100);
    int angle = M_PI * 30 / 180;
    int next_cp = 1;
    PodState ps(pos, vel, angle, next_cp);

    Vector target(500, 0);
    int max_acc = 100;

    for(int i = 0; i < 1000; i+= 50) {
        for(int j = 0; j < 1000; j+=50) {
            target = Vector(i, j);
            PodOutput po2 = nav.seek(ps, target);
            EXPECT_LE((int)po2.thrust, max_acc) << "Thrust should not exceed max acceleration";
        }
    }
}

TEST(NavigationTest, turn_saturation_adjusted) {
    Navigation nav(Race(1, {}));
    Race r(1, {});
    Vector pos(200, 200);
    Vector vel(200, 0);
    Vector target(400, 400);
    int acc = 100;
    PodOutput po(acc, target);
    int angle = 0;
    PodState pod(pos, vel, angle, 0);
    PodOutput adjusted = nav.turnSaturationAdjust(pod, po);
    EXPECT_LT(adjusted.thrust, acc) << "Thrust should have been reduced.";

    Vector targetObtuse(100, 300);
    PodOutput po2(acc, targetObtuse);
    PodOutput adjusted2 = nav.turnSaturationAdjust(pod, po2);
    EXPECT_EQ(0, adjusted2.thrust) << "Thrust should have been completely cut after 118deg.";

    Vector targetAcute(400, 235); // ~10deg turn right (south east).
    PodOutput po3(acc, targetAcute);
    PodOutput adjusted3 = nav.turnSaturationAdjust(pod, po3);
    EXPECT_EQ(acc, adjusted3.thrust) << "Turn angle is below max, and thrust should not be affected.";
}


TEST(NavigationTest, turnsUntilReached) {
    // Need to specific at least 1 checkpoint or the physic's move method will have undefined behaviour.
    Navigation nav(Race(1, {Checkpoint(0,0,0)}));
    Vector pos(200, 0);
    Vector vel(100, 0);
    double angle = 0;
    Vector target(1000, 0);
    PodState ps(pos, vel, angle, 0);
    double withinDist = 100;
    int turns = nav.turnsUntilReached(ps, target, withinDist);
    int precalculatedAns = 4;
    EXPECT_EQ(precalculatedAns, turns);
}

TEST(NavigationTest, findIntecept) {
    Navigation nav(Race(1, {Checkpoint(0,0,0)}));
    Vector pos(9335, 977);
    Vector vel(0, 0);
    double angle = M_PI * (145 / 180);
    PodState ps(pos, vel, angle, 0);

    // enemy
    Vector posE(10238, 2761);
    PodState psE(posE, vel, angle, 0);
    ASSERT_NO_FATAL_FAILURE(nav.find_intercept(ps, psE));
}

