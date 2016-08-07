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
    Vector thrust = vel_diff * (MAX_THRUST / vel_diff.getLength());
    return PodOutput(thrust.getLength(), vel_diff + pod.pos);
}


PodOutput Navigation::turnSaturationAdjust(const PodState& pod, const PodOutput& control) {
    if(control.thrust == PodOutput::BOOST || control.thrust == PodOutput::SHIELD) {
        return control;
    }
    double cut_off = M_PI / 2 + MAX_ANGLE;
    double angle_threshold = M_PI * (15.0 / 180.0); // 15 degrees in radians.
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

PodOutput Navigation::preemptSeek(const PodState& pod) {
    int turnThreshold = 8;
    int switchThreshold = 3;
    int future_x = pod.pos.x + geometric_sum(pod.vel.x, DRAG, 1, turnThreshold);
    int future_y = pod.pos.y + geometric_sum(pod.vel.y, DRAG, 1, turnThreshold);
    Vector future_pos(future_x, future_y);
    int buffer = 100;
    if((race.checkpoints[pod.nextCheckpoint].pos - future_pos).getLength() < CHECKPOINT_RADIUS - buffer) {
        // Coast and turn towards next CP.
        int nextNextCP = (pod.nextCheckpoint + 1) % race.checkpoints.size();
        int future_x = pod.pos.x + geometric_sum(pod.vel.x, DRAG, 1, switchThreshold);
        int future_y = pod.pos.y + geometric_sum(pod.vel.y, DRAG, 1, switchThreshold);
        future_pos = Vector(future_x, future_y);
        if((race.checkpoints[pod.nextCheckpoint].pos - future_pos).getLength() < CHECKPOINT_RADIUS - buffer) {
            // On target very shortly; we can burn torward the next CP.
            return turnSaturationAdjust(pod, seek(pod, race.checkpoints[nextNextCP].pos));
        } else {
            PodOutput po = seek(pod, race.checkpoints[nextNextCP].pos);
            po.thrust = 0;
            return po;
        }
    } else {
        return turnSaturationAdjust(pod, seek(pod, race.checkpoints[pod.nextCheckpoint].pos));
    }
}

PodOutput Navigation::intercept(const PodState& pod, const PodState& enemy) {
    return seek(pod, find_intercept(pod, enemy));
}


double Navigation::geometric_sum(double a, double r, int r1, int r2) {
    return (a * pow(r,r1) - a * pow(r,(r2 + 1))) / (1 - r);
}

Vector Navigation::find_intercept(const PodState& pod, const PodState& enemy) {
    int accept_range = 700;
    Vector intercept_path = race.checkpoints[enemy.nextCheckpoint].pos - enemy.pos;
    double low = 0;
    double high = intercept_path.getLength();
    Vector midPoint;
    int ourLastTime = -1;
    int enemyLastTime = -1;
    int close_enough = 0;
    int ourTime = 0;
    int enemyTime = ourTime + close_enough + 1;
    while (abs(ourTime - enemyTime) > close_enough && low < high) {
        double mid = low + (high - low) / 2;
        midPoint = enemy.pos + intercept_path * (mid / intercept_path.getLength());
        ourTime = turnsUntilReached(pod, midPoint, accept_range);
        // I am using an approximation of the enemy's movement algorithm.
        enemyTime = turnsUntilReached(enemy, midPoint, accept_range);
        cerr << "Our time: " << ourTime << "  Their time: " << enemyTime << endl;
        if (ourTime > enemyTime) {
            low = mid;
        } else if (ourTime < enemyTime) {
            high = mid;
        }
        if(ourTime == ourLastTime && enemyTime == enemyLastTime) break;
        ourLastTime = ourTime;
        enemyLastTime = enemyTime;
    }
    if (abs(ourTime - enemyTime) <= close_enough) {
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
