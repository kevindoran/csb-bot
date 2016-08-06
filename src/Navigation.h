//
// Created by Kevin on 4/08/2016.
//

#ifndef CODERSSTRIKEBACKC_NAVIGATION_H
#define CODERSSTRIKEBACKC_NAVIGATION_H

#include "State.h"
#include "Physics.h"

class Navigation {
    Race race;
    Physics physics;
    double geometric_sum(double a, double r, int r1, int r2);
public:
    Navigation(const Race& race) : race(race), physics(race) {}

    PodOutput seek(const PodState& pod, const Vector& target);

    PodOutput turnSaturationAdjust(const PodState &pod, const PodOutput &control);

    PodOutput preemptSeek(const PodState &pod);

};


#endif //CODERSSTRIKEBACKC_NAVIGATION_H
