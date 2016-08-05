//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_PHYSICS_H
#define CODERSSTRIKEBACKC_PHYSICS_H

#include "State.h"
#include "Vector.h"

class Physics {
    static const int MAX_THRUST = 200;
    static const int MAX_ANGLE = 18;
    static constexpr double DRAG = 0.85;
public:
    static PodState move(const Race& race, const PodState& pod, double acc, double angle, double time);
    static bool passedCheckpoint(Vector beforePos, Vector afterPos, Checkpoint checkpoint);
    static double angleBetween(Vector from, Vector to);
    static double radToDegrees(double radians);
    static double turnAngle(PodState pod, Vector target);
};


#endif //CODERSSTRIKEBACKC_PHYSICS_H
