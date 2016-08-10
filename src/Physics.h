#ifndef CODERSSTRIKEBACK_PHYSICS_H
#define CODERSSTRIKEBACK_PHYSICS_H

#include "State.h"
#include "Vector.h"

class Physics {
    Race race;
public:
    Physics(const Race &race) : race(race) {}

    PodState move(const PodState &pod, const PodOutput &control, double time);

    /**
     * Determines if there in an intersetion between a travel path and a checkpoint (line-circle intersection).
     */
    bool passedCheckpoint(const Vector &beforePos, const Vector &afterPos, const Checkpoint &checkpoint);

    double radToDegrees(double radians);

    /**
     * Calculate the angle between two vectors; the angle in the wedge created by two vectors.
     */
    double angleBetween(const Vector &from, const Vector &to);

    /**
     * Calculates the angle, from the reference frame of the pod, needed to rotate in order to face the target.
     */
    double turnAngle(const PodState &pod, const Vector &target);

    /**
     * Calculates the angle, measured clockwise from the positive x-axis centered at fromPoint, made by the line
     * connecting fromPoint and toPoint.
     */
    double angleTo(const Vector &fromPoint, const Vector &toPoint);

    bool passedPoint(const Vector &beforePos, const Vector &afterPos, const Vector &target, double radius);

    bool isCollision(const PodState &podA, const PodOutput &controlA,
                     const PodState &podB, const PodOutput &controlB, double velThreshold);

    bool isCollision(const PodState &podA, const PodOutput &controlA,
                     const PodState &podB, const PodOutput &controlB, int turns, double velThreshold);

    /**
     * Guess the next pod's output based on its previous and current state.
     */
    PodOutput expectedControl(const PodState &previous, const PodState &current);

    /**
     * Reapply the control to the pod for a number of turns and determine the resulting state. Note, the relative
     * force direction is recomputed each turn (no just moving the pod towards the initial thrust target point).
     */
    PodState extrapolate(const PodState &pod, const PodOutput &control, int turns);

};


#endif //CODERSSTRIKEBACK_PHYSICS_H
