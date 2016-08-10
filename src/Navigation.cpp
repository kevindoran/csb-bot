#define _USE_MATH_DEFINES

#include <cmath>
#include <math.h>
#include "State.h"
#include "Physics.h"
#include "Navigation.h"


PodOutput Navigation::seek(const PodState &pod, const Vector &target) {
    Vector desired_vel = (target - pod.pos) * 0.5;
    Vector vel_diff = desired_vel - pod.vel;
    Vector thrust = vel_diff * (MAX_THRUST / max(1.0, vel_diff.getLength()));
    cerr << "1 " << (vel_diff + pod.pos) << endl;
    return PodOutput(thrust.getLength(), vel_diff + pod.pos);
}

// One side of the normal distribution's CDF.
double normal_cdf_half(double x) {
    x = abs(x);
    return 1 - sqrt(1 - exp(-(2 / M_PI) * x * x));
}

// Decrease the thrust if the turn angle in greater than the maximum turn angle.
PodOutput Navigation::turnSaturationAdjust(const PodState &pod, const PodOutput &control) {
    if (control.thrust == PodOutput::BOOST || control.thrust == PodOutput::SHIELD) {
        return control;
    }
    double cut_off = M_PI / 2 + MAX_ANGLE;
    double angle_threshold = M_PI * (20.0 / 180.0);
    double turn_angle = abs(physics.turnAngle(pod, control.target));
    double acc = control.thrust;
    if (turn_angle > cut_off) {
        acc = 0;
    } else if (turn_angle > angle_threshold) {
        // Trim the thrust. Two approaches:
        // 1. Linear ramp:
        // speed = speed - int(((angle - angle_threshold) / (max_angle - angle_threshold)) * max_minus)
        //
        // 2. Ramp down with the shape of the normal distribution:
        double spread = 0.3;
        acc *= normal_cdf_half(spread * (turn_angle - angle_threshold) / (cut_off - angle_threshold));
        // Notes:
        // Minus angle_threshold to delay the speed dip, like a truncated normal distribution.
        // Times 0.3 to spread out the distribution. With the spread at 0.3, the standard deviation ~= 3.33 (~= pi).
        // With the SD ~= pi, we are setting the minimum thrust to be the 2 * (normal_dist(1) - normal_dist(0)) (we
        // are only considering half the distribution- imagine the normal distribution folded on itself through 0).
        // 2 * (normal_dist(1) - normal_dist(0)) = 2 * normal_dist(-1) ~= 0.30
    }
    PodOutput adjusted(acc, control.target);
    return adjusted;
}

PodOutput Navigation::preemptSeek(const PodState &pod, Vector initialTarget, double radius, Vector nextTarget) {
    // If we are on target within 5 turns, we will thrust towards the next CP. If we are not on target within 5 turns
    // but are on target within 6, turn towards the next CP without thrusting.
    int defaultTurn = 6;
    int defaultSwitch = 5;
    return preemptSeek(pod, initialTarget, radius, nextTarget, defaultTurn, defaultSwitch);
}

PodOutput Navigation::preemptSeek(const PodState &pod, Vector initialTarget, double radius,
                                  Vector nextTarget, int turnThreshold, int switchThreshold) {
    int i = 0;
    for (; i <= turnThreshold; i++) {
        // Hardcoded heuristic buffers. Tighter for when we are going to switch to the next CP as the cost of error
        // is much greater. TODO: turn into parameters and search for optimal values.
        int buffer = i <= switchThreshold ? 50 : 200;
        // Extrapolate the pod's position under conditions of no thrust.
        int future_x = pod.pos.x + geometric_sum(pod.vel.x, DRAG, 1, i);
        int future_y = pod.pos.y + geometric_sum(pod.vel.y, DRAG, 1, i);
        Vector future_pos(future_x, future_y);
        // If the future position is close enough to the target, break.
        if ((initialTarget - future_pos).getLength() < radius - buffer) {
            break;
        }
    }
    if (i <= switchThreshold) {
        // On target very shortly; we can burn toward the next CP.
        return turnSaturationAdjust(pod, seek(pod, nextTarget));
    } else if (i <= turnThreshold) {
        // We will drift towards current target and turn toward the next CP.
        PodOutput po = seek(pod, nextTarget);
        po.thrust = 0;
        return po;
    } else {
        // Otherwise, just burn towards the current target.
        return turnSaturationAdjust(pod, seek(pod, initialTarget));
    }
}

PodOutput Navigation::preemptSeek(const PodState &pod) {
    Checkpoint nextCP = race.checkpoints[(pod.nextCheckpoint + 1) % race.checkpoints.size()];
    return preemptSeek(pod, race.checkpoints[pod.nextCheckpoint].pos, CHECKPOINT_RADIUS, nextCP.pos);
}

PodOutput Navigation::intercept(const PodState &pod, const PodState &enemy) {
    return seek(pod, find_intercept(pod, enemy));
}


double Navigation::geometric_sum(double a, double r, int r1, int r2) {
    return (a * pow(r, r1) - a * pow(r, (r2 + 1))) / (1 - r);
}

// Binary search along the path between the enemy and its next checkpoint- search for a point where our bot
// and the enemy bot will arrive at the same time.
Vector Navigation::find_intercept(const PodState &pod, const PodState &enemy) {
    Vector intercept_path = race.checkpoints[enemy.nextCheckpoint].pos - enemy.pos;
    // TODO: hardcoded heuristic- convert to parameter and search for optimum.
    // The distance between two points on the path below which is acceptable to be considered the same 'area' or
    // place where the bots are likely to collide.
    int accept_range = intercept_path.getLength() > 3000 ? 900 : 600;
    double low = 0;
    double high = intercept_path.getLength();
    Vector midPoint;
    // TODO: hardcoded heuristic- convert to parameter and search for optimum.
    // The difference in turns which is acceptable for the bots to arrive at the collision area.
    int close_enough = intercept_path.getLength() > 3000 ? 2 : 1;//max(1.0, intercept_path.getLength() / 1200);
    int ourTime = 0;
    int enemyTime = ourTime + close_enough + 1;
    int resolution = 5; // The search will end if the distance between the points is less than 5.
    while (abs(ourTime - enemyTime) > 0 && low < high) {
        double mid = low + (high - low) / 2;
        midPoint = enemy.pos + intercept_path * (mid / intercept_path.getLength());
        ourTime = turnsUntilReached(pod, midPoint, accept_range);
        enemyTime = turnsUntilReached(enemy, midPoint, accept_range);
//        cerr << "Our time: " << ourTime << "  Enemy time: " << enemyTime << endl;
        if (ourTime > enemyTime) {
            low = mid;
        } else if (ourTime < enemyTime) {
            high = mid;
        }
        if (high - low < resolution) break;
    }
    if (abs(ourTime - enemyTime) <= close_enough) {
        return midPoint;
    } else {
        // If there doesn't seem to be a place where the two bots will arrive at the same time, target the enemy bot's
        // next next checkpoint in preparation.
        int enemyNextNextCP = (enemy.nextCheckpoint + 1) % race.checkpoints.size();
        return race.checkpoints[enemyNextNextCP].pos;
    }
}

int Navigation::turnsUntilReached(const PodState &podInit, Vector target, double withinDist) {
    int maxTurns = 30;
    PodState pod = podInit;
    PodState previous = podInit;
    PodOutput control;
    int i = 0;
    while ((target - pod.pos).getLength() > withinDist &&
           !physics.passedPoint(previous.pos, pod.pos, target, withinDist)) {
        control = turnSaturationAdjust(pod, seek(pod, target));
        pod = physics.move(pod, control, 1);
        i++;
        // Currently, a bit of a hack to guard agains poor circular seeking behaviour causing endless
        // circling around the target and causing this method to timeout the bot.
        if (i >= maxTurns) break;
    }
    return i;
}