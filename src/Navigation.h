//
// Created by Kevin on 4/08/2016.
//

#ifndef CODERSSTRIKEBACKC_NAVIGATION_H
#define CODERSSTRIKEBACKC_NAVIGATION_H

#include "State.h"

class Navigation {
public:
    PodOutput seek(const PodState& pod, const Vector& target, int max_acc);

    PodOutput turnSaturationAdjust(const PodState &pod, const PodOutput &control);
};


#endif //CODERSSTRIKEBACKC_NAVIGATION_H
