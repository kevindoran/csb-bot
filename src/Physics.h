//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_PHYSICS_H
#define CODERSSTRIKEBACKC_PHYSICS_H

#include "State.h"
#include "Vector.h"

class Physics {
public:
    static PodState move(const Race& race, const PodState& pod, double acc, double angle, double time);
    static bool passedCheckpoint(const Vector& beforePos, const Vector& afterPos, const Checkpoint& checkpoint);
    static double angleBetween(const Vector& from, const Vector& to);
    static double radToDegrees(double radians);
    static double turnAngle(const PodState& pod, const Vector& target);
    static double angleTo(const Vector &fromPoint, const Vector &toPoint);
};


#endif //CODERSSTRIKEBACKC_PHYSICS_H
