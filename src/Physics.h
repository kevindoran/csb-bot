#ifndef CODERSSTRIKEBACK_PHYSICS_H
#define CODERSSTRIKEBACK_PHYSICS_H

#include "State.h"
#include "Vector.h"
#include "Bot.h"

class Physics {
    Race race;
public:
    Physics() {}

    Physics(const Race &race) : race(race) {}

    /**
     * Simulates the movement of a pod for a given time.
     */
    PodState move(const PodState &pod, const PodOutputAbs &control, float time);

    /**
     * Determines if there in an intersetion between a travel path and a checkpoint (line-circle intersection).
     */
    bool passedCheckpoint(const Vector &beforePos, const Vector &afterPos, const Vector &checkpoint);

    static float radToDegrees(float radians);

    /**
     * Calculate the angle between two vectors; the angle in the wedge created by two vectors.
     */
    static float angleBetween(const Vector &from, const Vector &to);

    /**
     * Calculates the angle, from the reference frame of the pod, needed to rotate in order to face the target.
     */
    static float turnAngle(const PodState &pod, const Vector &target);

    /**
     * Calculates the angle, measured clockwise from the positive x-axis centered at fromPoint, made by the line
     * connecting fromPoint and toPoint.
     */
    static float angleTo(const Vector &fromPoint, const Vector &toPoint);

    static bool passedPoint(const Vector &beforePos, const Vector &afterPos, const Vector &target, float radius);

    bool isCollision(const PodState &podA, const PodOutputAbs &controlA,
                     const PodState &podB, const PodOutputAbs &controlB, float velThreshold);

    bool isCollision(const PodState &podA, const PodOutputAbs &controlA,
                     const PodState &podB, const PodOutputAbs &controlB, int turns, float velThreshold);

    /**
     * Guess the next pod's output based on its previous and current state.
     */
    PodOutputAbs expectedControl(const PodState &previous, const PodState &current);

    /**
     * Reapply the control to the pod for a number of turns and determine the resulting state. Note, the relative
     * force direction is recomputed each turn (no just moving the pod towards the initial thrust target point).
     */
    PodState extrapolate(const PodState &pod, const PodOutputAbs &control, int turns);

    static Vector closestPointOnLine(Vector lineStart, Vector lineEnd, Vector point);

    static Vector closestPointOnLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY, float pointX, float pointY);

    static float passedCircleAt(float beforePosX, float beforePosY, float afterPosX, float afterPosY, float targetX, float targetY,
                                float radius);

    static float degreesToRad(float degrees);

    static Vector forceFromTarget(const PodState &pod, Vector target, float thrust);

    static void apply(PodState &pod, PodOutputSim control);

    static void apply(PodState* pods, PairOutput control);

    static void applyWithoutChecks(PodState &pod, const PodOutputSim &control);

    static void apply(PodState &pod, const PodOutputAbs &control);

    void simulate(PodState **pods);

    bool orderByProgress(PodState *pods);

    int leadPodID(PodState *pods);
};

class Event {
public:
    virtual float time() const = 0;
    virtual void resolve() = 0;
};

class PassedCheckpoint : public Event {
    PodState* mPod;
    float mTime;
    int mNextCheckpoint;
    PassedCheckpoint(PodState& pod, float time, int nextCP) : mPod(&pod), mTime(time), mNextCheckpoint(nextCP) {}
public:
    PassedCheckpoint() {}
    bool static testForPassedCheckpoint(PodState& a, Race& r, PassedCheckpoint* event, bool isEnemy);

    float time() const {return mTime;}
    void resolve();
};

class Collision : public Event {
    float mTime;
public:
    PodState* a;
    PodState* b;

    Collision(){}
    Collision(PodState& a, PodState& b, float time) : a(&a), b(&b), mTime(time) {}
    bool static testForCollision(PodState& a, PodState& b, Collision* collision);

    void resolve();
    float time() const {return mTime;}
};



#endif //CODERSSTRIKEBACK_PHYSICS_H
