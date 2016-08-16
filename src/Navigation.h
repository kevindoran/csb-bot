#ifndef CODERSSTRIKEBACK_NAVIGATION_H
#define CODERSSTRIKEBACK_NAVIGATION_H

#include "State.h"
#include "Physics.h"

class Navigation {
    Race race;
    Physics physics;

    // Si = ar^0 + ar^1 + ar^2...ar^i
    float geometric_sum(float a, float r, int r1, int r2);
public:
    Navigation() {}

    Navigation(const Race &race) : race(race), physics(race) {}

    Vector find_intercept(const PodState &pod, const PodState &enemy);

    PodOutputAbs seek(const PodState &pod, const Vector &target);

    PodOutputAbs turnSaturationAdjust(const PodState &pod, const PodOutputAbs &control);

    PodOutputAbs preemptSeek(const PodState &pod);

    PodOutputAbs intercept(const PodState &pod, const PodState &enemy);

    PodOutputAbs preemptSeek(const PodState &pod, Vector intialTarget, float radius, Vector nextTarget);

    PodOutputAbs preemptSeek(const PodState &pod, Vector initialTarget, float radius, Vector nextTarget,
                          int turnThreshold, int switchThreshold);

    int turnsUntilReached(const PodState &podInit, Vector target, float withinDist);
};

#endif //CODERSSTRIKEBACK_NAVIGATION_H
