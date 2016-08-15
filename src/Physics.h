#ifndef CODERSSTRIKEBACK_PHYSICS_H
#define CODERSSTRIKEBACK_PHYSICS_H

#include "State.h"
#include "Vector.h"

class Physics {
    Race race;
public:
    Physics(const Race &race) : race(race) {}

    /**
     * Simulates the movement of a pod and updates the pod itself.
     */
    void move(PodState& pod, double time);

    /**
     * Simulates the movement of a pod for a given time.
     */
    PodState move(const PodState &pod, const PodOutput &control, double time);

    /**
     * Determines if there in an intersetion between a travel path and a checkpoint (line-circle intersection).
     */
    bool passedCheckpoint(const Vector &beforePos, const Vector &afterPos, const Checkpoint &checkpoint);

    static double radToDegrees(double radians);

    /**
     * Calculate the angle between two vectors; the angle in the wedge created by two vectors.
     */
    static double angleBetween(const Vector &from, const Vector &to);

    /**
     * Calculates the angle, from the reference frame of the pod, needed to rotate in order to face the target.
     */
    static double turnAngle(const PodState &pod, const Vector &target);

    /**
     * Calculates the angle, measured clockwise from the positive x-axis centered at fromPoint, made by the line
     * connecting fromPoint and toPoint.
     */
    static double angleTo(const Vector &fromPoint, const Vector &toPoint);

    static bool passedPoint(const Vector &beforePos, const Vector &afterPos, const Vector &target, double radius);

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

    static Vector closestPointOnLine(Vector lineStart, Vector lineEnd, Vector point);

    static Vector closestPointOnLine(double lineStartX, double lineStartY, double lineEndX, double lineEndY, Vector point);
    void simulate(vector<PodState*> pods);

    static double passedCircleAt(const Vector &beforePos, const Vector &afterPos, const Vector &target, double radius);

    void orderByProgress(vector<PodState> &pods);

    static double degreesToRad(double degrees);

    static Vector forceFromTarget(const PodState &pod, Vector target, double thrust);

};

class Event {
public:
    virtual bool occurred() const = 0;
    virtual double time() const = 0;
    virtual void resolve() = 0;
};

class PassedCheckpoint : public Event {
    PodState& mPod;
    double mTime;
    int mNextCheckpoint;
    static const int INVALID = -1;

    PassedCheckpoint(PodState& pod, double time, int nextCP) : mPod(pod), mTime(time), mNextCheckpoint(nextCP) {}
    static PassedCheckpoint invalid(PodState& pod) {return PassedCheckpoint(pod, INVALID, 0);}
public:
    static PassedCheckpoint testForPassedCheckpoint(PodState& a, Race& r);
    double time() const {return mTime;}
    bool occurred() const {return mTime != INVALID;}
    void resolve();
};

class Collision : public Event {
    // Pointers to allow default constructor.
    PodState* a;
    PodState* b;
//    Vector collisionPoint;
    static const int INVALID = -1;
    double mTime = INVALID;
public:
    Collision(){}
    Collision(PodState& a, PodState& b, double time) : a(&a), b(&b), mTime(time) {}
    static Collision testForCollision(PodState& a, PodState& b);

    static Collision& invalidCollision() {
        static Collision invalidCollision;
        return invalidCollision;
    }

    void resolve();
    double time() const {return mTime;}
    bool occurred() const {return mTime != -1;}
};



#endif //CODERSSTRIKEBACK_PHYSICS_H
