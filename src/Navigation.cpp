//
// Created by Kevin on 4/08/2016.
//

#define _USE_MATH_DEFINES
#include <cmath>
#include "Navigation.h"
#include "Physics.h"
#include "State.h"


double normal_cdf_half(double x) {
    x = abs(x);
    return 1 - sqrt(1 - exp(-(2 / M_PI) * x*x));
}

PodOutput Navigation::seek(const PodState& pod, const Vector& target, int max_acc) {
    Vector desired_vel = (target - pod.pos) * 0.5;
    Vector vel_diff = desired_vel - pod.vel;
    Vector thrust = vel_diff * (max_acc / vel_diff.getLength());
    return PodOutput(thrust.getLength(), vel_diff + pod.pos);
}


PodOutput Navigation::turnSaturationAdjust(const PodState &pod, const PodOutput &control) {
    if(control.thrust == PodOutput::BOOST || control.thrust == PodOutput::SHIELD) {
        return control;
    }
    double cut_off = M_PI / 2 + MAX_ANGLE;
    double angle_threshold = M_PI * (15.0 / 180.0); // 15 degrees in radians.
    double turn_angle = abs(Physics::turnAngle(pod, control.dir));
    double acc = control.thrust;
    cerr << "Angle: " << pod.angle << "  Thrust: " << acc << "  Control dir: (" << control.dir.x << ", " << control.dir.y
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
    PodOutput adjusted(acc, control.dir);
    return adjusted;
}
