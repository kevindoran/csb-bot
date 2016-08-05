//
// Created by Kevin on 4/08/2016.
//

#define _USE_MATH_DEFINES
#include <cmath>
#include "Navigation.h"
#include "Physics.h"
#include "State.h"


PodOutput Navigation::seek(const PodState& pod, const Vector& target, int max_acc) {
    Vector desired_vel = (target - pod.pos) * 0.5;
    Vector vel_diff = desired_vel - pod.vel;
    Vector thrust = vel_diff * (max_acc / vel_diff.getLength());
    return PodOutput(thrust.getLength(), vel_diff + pod.pos);
}

double normal_cdf_half(double x) {
    x = abs(x);
    return 1 - sqrt(1 - exp(-(2 / M_PI) * x*x));
}
