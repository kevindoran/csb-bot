//
// Created by Kevin on 4/08/2016.
//

#include "Navigation.h"


PodOutput Navigation::seek(PodState pod, Vector target, int max_acc) {
    Vector desired_vel = (target - pod.pos) * 0.5;
    Vector vel_diff = desired_vel - pod.vel;
    Vector thrust = vel_diff * (max_acc / vel_diff.getLength());
    return PodOutput(thrust.getLength(), vel_diff + pod.pos);
}

