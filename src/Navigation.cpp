//
// Created by Kevin on 4/08/2016.
//

#define _USE_MATH_DEFINES
#include <cmath>
#include <math.h>
#include "Navigation.h"
#include "Physics.h"
#include "State.h"


double normal_cdf_half(double x) {
    x = abs(x);
    return 1 - sqrt(1 - exp(-(2 / M_PI) * x*x));
}

PodOutput Navigation::seek(const PodState& pod, const Vector& target) {
    Vector desired_vel = (target - pod.pos) * 0.5;
    Vector vel_diff = desired_vel - pod.vel;
    Vector thrust = vel_diff * (MAX_THRUST / max(1.0, vel_diff.getLength()));
    cerr << "1 " << (vel_diff + pod.pos) << endl;
    return PodOutput(thrust.getLength(), vel_diff + pod.pos);
}


PodOutput Navigation::turnSaturationAdjust(const PodState& pod, const PodOutput& control) {
    if(control.thrust == PodOutput::BOOST || control.thrust == PodOutput::SHIELD) {
        return control;
    }
    double cut_off = M_PI / 2 + MAX_ANGLE;
    double angle_threshold = M_PI * (20.0 / 180.0); // 15 degrees in radians.
    double turn_angle = abs(physics.turnAngle(pod, control.target));
    double acc = control.thrust;
    if(turn_angle > cut_off) {
        acc = 0;
    } else if(turn_angle > angle_threshold) {
        // linear#speed = speed - int(((angle - angle_threshold) / (max_angle - angle_threshold)) * max_minus)
        // minus angle_threshold to delay the speed dip. Like a truncated normal distribution.
        // Times 0.3 to spread out the distribution (SD = 0.3). Otherwise, there is only a range of 0-1SD.
        double spread = 0.3;
        acc *= normal_cdf_half(spread * (turn_angle - angle_threshold) / (cut_off - angle_threshold));
    }
    PodOutput adjusted(acc, control.target);
    return adjusted;
}

PodOutput Navigation::preemptSeek(const PodState& pod, Vector initialTarget, double radius, Vector nextTarget) {
    int defaultTurn = 6; // Disabled
    int defaultSwitch = 5;
    return preemptSeek(pod, initialTarget, radius, nextTarget, defaultTurn, defaultSwitch);
}
PodOutput Navigation::preemptSeek(const PodState& pod, Vector initialTarget, double radius, Vector nextTarget, int turnThreshold, int switchThreshold) {
    int i = 0;
    for(; i <= turnThreshold; i++) {
        int buffer = i <= switchThreshold ? 50 : 200;
        int future_x = pod.pos.x + geometric_sum(pod.vel.x, DRAG, 1, i);
        int future_y = pod.pos.y + geometric_sum(pod.vel.y, DRAG, 1, i);
        Vector future_pos(future_x, future_y);
        if ((initialTarget - future_pos).getLength() < radius - buffer) {
            break;
        }
    }
    if(i <= switchThreshold) {
        // On target very shortly; we can burn torward the next CP.
        return turnSaturationAdjust(pod, seek(pod, nextTarget));
    } else if(i <= turnThreshold){
        // We will drift towards current target, we can turn toward the next CP.
        PodOutput po = seek(pod, nextTarget);
        po.thrust = 0;
        return po;
    } else {
        return turnSaturationAdjust(pod, seek(pod, initialTarget));
    }
}

PodOutput Navigation::preemptSeek(const PodState& pod) {
    Checkpoint nextCP = race.checkpoints[(pod.nextCheckpoint + 1) % race.checkpoints.size()];
    return preemptSeek(pod, race.checkpoints[pod.nextCheckpoint].pos, CHECKPOINT_RADIUS, nextCP.pos);
}

PodOutput Navigation::intercept(const PodState& pod, const PodState& enemy) {
    return seek(pod, find_intercept(pod, enemy));
}


double Navigation::geometric_sum(double a, double r, int r1, int r2) {
    return (a * pow(r,r1) - a * pow(r,(r2 + 1))) / (1 - r);
}

Vector Navigation::find_intercept(const PodState& pod, const PodState& enemy) {
    Vector intercept_path = race.checkpoints[enemy.nextCheckpoint].pos - enemy.pos;
    int accept_range = intercept_path.getLength() > 3000 ? 900 : 600;
    double low = 0;
    double high = intercept_path.getLength();
    Vector midPoint;
    int close_enough = intercept_path.getLength() > 3000 ? 2 : 1;//max(1.0, intercept_path.getLength() / 1200);
    int ourTime = 0;
    int enemyTime = ourTime + close_enough + 1;
    int resolution = 5;
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
        if(high - low < resolution) break;
    }
    cerr << "Our turns: " << ourTime << "  their turns: " << enemyTime << endl;
    if (abs(ourTime - enemyTime) <= close_enough) {
        cerr << "2: " << midPoint << endl;
        return midPoint;
    } else {
        int enemyNextNextCP = (enemy.nextCheckpoint + 1) % race.checkpoints.size();
        return race.checkpoints[enemyNextNextCP].pos;
    }
}

int Navigation::turnsUntilReached(const PodState& podInit, Vector target, double withinDist) {
    int maxTurns = 30;
    PodState pod = podInit;
    PodState previous = podInit;
    PodOutput control;
    int i = 0;
    while((target - pod.pos).getLength() > withinDist &&  !physics.passedPoint(previous.pos, pod.pos, target, withinDist)) {
        control = turnSaturationAdjust(pod, seek(pod, target));
        pod = physics.move(pod, control, 1);
        i++;
        // Currently, a bit of a hack to guard agains poor circular seeking behaviour causing endless
        // circling around the target and causing this method to timeout the bot.
        if(i >= maxTurns) break;
    }
    return i;
}
