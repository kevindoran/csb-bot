#ifndef CODERSSTRIKEBACK_NAVIGATION_H
#define CODERSSTRIKEBACK_NAVIGATION_H

#include "State.h"
#include "Physics.h"

class Navigation {
    Race race;
    Physics physics;

    // Si = ar^0 + ar^1 + ar^2...ar^i
    double geometric_sum(double a, double r, int r1, int r2);
public:

    Vector find_intercept(const PodState &pod, const PodState &enemy);

    Navigation(const Race &race) : race(race), physics(race) {}

    PodOutput seek(const PodState &pod, const Vector &target);

    PodOutput turnSaturationAdjust(const PodState &pod, const PodOutput &control);

    PodOutput preemptSeek(const PodState &pod);

    PodOutput intercept(const PodState &pod, const PodState &enemy);

    PodOutput preemptSeek(const PodState &pod, Vector intialTarget, double radius, Vector nextTarget);

    PodOutput preemptSeek(const PodState &pod, Vector initialTarget, double radius, Vector nextTarget,
                          int turnThreshold, int switchThreshold);

    int turnsUntilReached(const PodState &podInit, Vector target, double withinDist);
};

#endif //CODERSSTRIKEBACK_NAVIGATION_H
