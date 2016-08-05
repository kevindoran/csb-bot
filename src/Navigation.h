//
// Created by Kevin on 4/08/2016.
//

#ifndef CODERSSTRIKEBACKC_NAVIGATION_H
#define CODERSSTRIKEBACKC_NAVIGATION_H

#include "State.h"

class Navigation {
public:
    PodOutput seek(PodState pod, Vector target, int max_acc);
};


#endif //CODERSSTRIKEBACKC_NAVIGATION_H
