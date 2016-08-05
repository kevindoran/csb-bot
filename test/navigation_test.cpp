//
// Created by Kevin on 5/08/2016.
//

#include "gtest/gtest.h"
#include "Navigation.h"

TEST(NavigationTest, seek) {
    Navigation nav;
    Vector pos(200, 200);
    Vector vel(100, 100);
    int angle = 30;
    int next_cp = 1;
    PodState ps(pos, vel, angle, next_cp);

    Vector target(500, 0);
    int max_acc = 100;
    PodOutput po = nav.seek(ps, target, max_acc);
    Vector pre_computed_ans(-55, -83);
    double po_x = po.direction.x * po.thrust / po.direction.getLength();
    double po_y = po.direction.y * po.thrust / po.direction.getLength();
    double po_x_rounded = round(po_x);
    double po_y_rounded = round(po_y);
    EXPECT_EQ(po_x_rounded, pre_computed_ans.x);
    EXPECT_EQ(po_y_rounded, pre_computed_ans.y);


    for(int i = 0; i < 1000; i+= 50) {
        for(int j = 0; j < 1000; j+=50) {
            target = Vector(i, j);
            PodOutput po2 = nav.seek(ps, target, max_acc);
            EXPECT_LE(po2.thrust, max_acc) << "Thrust should not exceed max acceleration";
        }
    }
}

