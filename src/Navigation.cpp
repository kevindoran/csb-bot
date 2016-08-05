//
// Created by Kevin on 4/08/2016.
//

#include "Navigation.h"


PodOutput Navigation::seek(PodState pod, Vector target, int max_acc) {
    Vector dir = (target - pod.pos).normalize();
    Vector good_component = dir.project(pod.vel);
    Vector bad_component = pod.vel - good_component;
    double remainderSq = max_acc*max_acc - bad_component.getLengthSq();
    Vector thrust = -bad_component;
    if(remainderSq > 0) {
        thrust += dir * sqrt(remainderSq);
    } else {
        thrust *= ((double)max_acc) / thrust.getLength();
    }
    return PodOutput(thrust.getLength(), thrust * 10 + pod.pos);
}
