//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_PHYSICS_H
#define CODERSSTRIKEBACKC_PHYSICS_H

#include "State.h"
#include "Vector.h"

class Physics {
    Race race;
public:
    Physics(const Race& race) : race(race) {}
    PodState move(const PodState& pod, const PodOutput& control, double time);
    bool passedCheckpoint(const Vector& beforePos, const Vector& afterPos, const Checkpoint& checkpoint);
    double angleBetween(const Vector& from, const Vector& to);
    double radToDegrees(double radians);
    double turnAngle(const PodState& pod, const Vector& target);
    double angleTo(const Vector &fromPoint, const Vector &toPoint);
    Vector forceVector(const PodState &appliedTo, const PodOutput &control);

};


#endif //CODERSSTRIKEBACKC_PHYSICS_H
