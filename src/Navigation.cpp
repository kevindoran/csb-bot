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
    cerr << "Angle: " << pod.angle << "  Thrust: " << acc << "  Control dir: (" << control.target.x << ", " << control.target.y
         << ")  Turn angle: " << turn_angle << endl;
    if(turn_angle > cut_off) {
        acc = 0;
    } else if(turn_angle > angle_threshold) {
        // linear#speed = speed - int(((angle - angle_threshold) / (max_angle - angle_threshold)) * max_minus)
        // minus angle_threshold to delay the speed dip. Like a truncated normal distribution.
        // Times 0.3 to spread out the distribution (SD = 0.3). Otherwise, there is only a range of 0-1SD.
        double spread = 0.3;
        cerr << "Before acc: " << acc << endl;
        acc *= normal_cdf_half(spread * (turn_angle - angle_threshold) / (cut_off - angle_threshold));
        cerr << "After acc: " << acc << endl;
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


double Navigation::geometric_sum(double a, double r, int r1, int r2) {
    return (a * pow(r,r1) - a * pow(r,(r2 + 1))) / (1 - r);
}

// Use interpolation to estimate where the pod will glide to. It is incorrect to
// simulate the game to determine the number of moves, as the bot will change behaviour
// if the required moves is low enough.
//int Navigation::turnsUntilPassCP(const PodState& podInit, int cutoff) {
//    int cp = podInit.nextCheckpoint;
//    PodState pod = podInit;
//    int i = 1;
//    for(bool reachedCP = false; i <= cutoff && !reachedCP; i++) {
//        PodOutput control = turnSaturationAdjust(pod, seek(pod, race.checkpoints[cp].pos, 0));
//        pod = physics.move(pod, control, 1);
//        if(pod.nextCheckpoint != cp) {
//            reachedCP = true;
//        }
//    }
//    return i;
//}
