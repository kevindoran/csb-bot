
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <math.h>

#include "State.h"
#include "Physics.h"

void Physics::apply(PodState& pod, PodOutputSim control) {
    if(abs(control.angle) > MAX_ANGLE) {
        control.angle = control.angle < -MAX_ANGLE ? -MAX_ANGLE : MAX_ANGLE;
    }
    pod.angle += control.angle;
    if(control.shieldEnabled) {
        pod.shieldEnabled = true;
        pod.turnsSinceShield = 0;
    } else {
        pod.shieldEnabled = false;
        if(pod.turnsSinceShield <= SHIELD_COOLDOWN) {
            control.thrust = 0;
        } else if(pod.boostAvailable && control.boostEnabled) {
            control.thrust = BOOST_ACC;
            pod.boostAvailable = false;
        }
        else if(control.thrust > MAX_THRUST) {
            control.thrust = MAX_THRUST;
        } else if(control.thrust < 0) {
            control.thrust = 0;
        }
        Vector force = Vector::fromMagAngle(control.thrust, pod.angle);
        pod.vel.x = pod.vel.x + force.x;
        pod.vel.y = pod.vel.y + force.y;
        pod.vel.resetLengths();
    }
}

void Physics::applyWithoutChecks(PodState& pod, const PodOutputSim& control) {
    pod.angle += control.angle;
    if(control.shieldEnabled) {
        pod.shieldEnabled = true;
    } else {
        pod.vel += Vector::fromMagAngle(control.thrust, pod.angle);
    }
}

void Physics::apply(PodState pods[], PairOutput control) {
    apply(pods[0], control.o1);
    apply(pods[1], control.o2);
}

void Physics::apply(PodState& pod, const PodOutputAbs& control) {
    Vector force = Physics::forceFromTarget(pod, control.target, control.thrust);
    float turn = Physics::turnAngle(pod, force);
    PodOutputSim po = PodOutputSim::fromAbsolute(pod, control);
    apply(pod, po);
}

bool PassedCheckpoint::testForPassedCheckpoint(PodState& a, Race& race, PassedCheckpoint* event, bool isEnemy) {
    float CP_BUFFER = isEnemy ? 0 : -15;
    float time = Physics::passedCircleAt(a.pos.x, a.pos.y, a.pos.x + a.vel.x, a.pos.y + a.vel.y,
                 race.checkpoints[a.nextCheckpoint].x, race.checkpoints[a.nextCheckpoint].y, CHECKPOINT_RADIUS + CP_BUFFER);
    if(time == -1) return false;
    else {
        *event = PassedCheckpoint(a, time, race.followingCheckpoint(a.nextCheckpoint));
        return true;
    }
}

void PassedCheckpoint::resolve() {
    mPod->nextCheckpoint = mNextCheckpoint;
    mPod->passedCheckpoints++;
    mPod->turnsSinceCP = 0;
}

void Physics::simulate(PodState* pods[POD_COUNT*2]) {
    // Update counters.
    for(int i = 0; i < POD_COUNT*2; i++) {
        pods[i]->turnsSinceCP++;
        pods[i]->turnsSinceShield++;
    }
    float time = 0;
    vector<PassedCheckpoint> pCPEvents;
    PassedCheckpoint cpEvent;
    Collision earliest;
    int earliestIdx[2];
    int skip[2] = {-1, -1};
    bool started = false;
    bool hasCollision = false;
    bool occurred = false;
    Collision collision;
    while(time < 1) {
        for (int i = 0; i < POD_COUNT*2; i++) {
            // Collision or checkpoint passing first?
            for (int j = i + 1; j < POD_COUNT*2; j++) {
                // Shortcut for performance: the previous two to collide can't collide next.
                if (skip[0] == i && skip[1] == j) {
                    continue;
                }
                // These two had the previous collision.
                occurred = Collision::testForCollision(*pods[i], *pods[j], &collision);
                // TODO: can the second zero time collision be ignored?

                if (occurred && collision.time() + time < 1.0 &&
                    (!hasCollision || earliest.time() > collision.time())) {
                    hasCollision = true;
                    earliest = collision;
                    earliestIdx[0] = i;
                    earliestIdx[1] = j;
                }
            }
            // Can optimize by keeping list of pc events and resolving all those before the earliest collision.
            occurred = PassedCheckpoint::testForPassedCheckpoint(*pods[i], race, &cpEvent, 1 > 1);
            if (occurred && cpEvent.time() + time < 1.0) {
                pCPEvents.push_back(cpEvent);
            }
        }
        for (auto& pcp : pCPEvents) {
            if (!hasCollision || pcp.time() < earliest.time()) {
                pcp.resolve();
            }
        }
        pCPEvents.clear();
        float moveTime = hasCollision ? earliest.time() : 1.0 - time;

        for(int i = 0; i < POD_COUNT*2; i++) {
            pods[i]->pos.x += pods[i]->vel.x * moveTime;
            pods[i]->pos.y += pods[i]->vel.y * moveTime;
        }
        if (hasCollision) {
            earliest.resolve();
            hasCollision = false;
            skip[0] = earliestIdx[0];
            skip[1] = earliestIdx[1];
        }
        time += moveTime;
    }

    // Drag and rounding.
    for(int i = 0; i < POD_COUNT*2; i++) {
        pods[i]->vel.x = pods[i]->vel.x * DRAG;
        pods[i]->vel.y = pods[i]->vel.y * DRAG;
        pods[i]->vel.x = (int) pods[i]->vel.x;
        pods[i]->vel.y = (int) pods[i]->vel.y;
        // The instructions say that position is rounded, however, on inspection, truncating seems to be closer
        // to reality. In addition, rounding is a non-insignificant performance cost.
        pods[i]->pos.x = (int) pods[i]->pos.x;//round(pods[i]->pos.x);
        pods[i]->pos.y = (int) pods[i]->pos.y;//round(pods[i]->pos.y);
        pods[i]->pos.resetLengths();
        pods[i]->vel.resetLengths();
    }
}

bool Collision::testForCollision(PodState& a, PodState& b, Collision* collision) {
    // Vectors are cleaner, but slower.
    float pathStartX = b.pos.x - a.pos.x;
    float pathStartY = b.pos.y - a.pos.y;
    float velX = b.vel.x - a.vel.x;
    float velY = b.vel.y - a.vel.y;
    // If the bots have the same speed.
    if(velX == 0 && velY == 0) {
        return false;
    }
    float pathEndX = pathStartX + velX;
    float pathEndY = pathStartY + velY;

    Vector closestPoint = Physics::closestPointOnLine(pathStartX, pathStartY, pathEndX, pathEndY, 0, 0);
    if(closestPoint.getLengthSq() > (2*POD_RADIUS)*(2*POD_RADIUS)) {
        return false;
    } else {
        float velLength = sqrt(velX * velX + velY * velY);
        float parallelDist = abs((pathStartX * (pathEndX - pathStartX) + pathStartY * (pathEndY - pathStartY)) / velLength);
        float tangentDistSq = (pathStartX*pathStartX + pathStartY*pathStartY) - (parallelDist*parallelDist);

        float backDist = sqrt((POD_RADIUS*2)*(POD_RADIUS*2) - tangentDistSq);//closestPoint.getLengthSq());

        float travelDist = parallelDist - backDist;
        // Rounding can cause the pods to overlap. If the pods are going to collide, resolve the collision at t=0,
        // otherwise ignore (ignore if the pods have no velocity component at odds. The fastest way to check is
        // is indirectly by testing if the closest point is actually the current point- if so, the pods must be
        // moving away from each other).
        if(travelDist < 0) {
            if (closestPoint.x == pathStartX && closestPoint.y == pathStartY) {
                // Pods are moving away from each other.
                return false;
            } else {
                travelDist = 0;
            }
        }
        float time = travelDist / velLength;
        *collision = Collision(a, b, time);
        return true;
    }
}

void Collision::resolve() {
    // TODO: some vector usage could be converted to basic types.
    float m1 = a->shieldEnabled ? 10 : 1;
    float m2 = b->shieldEnabled ? 10 : 1;
    float mCoeff = (m1 * m2) / (m1 + m2);
    Vector normal = (b->pos - a->pos).normalize();
    Vector relativeVel = a->vel - b->vel;
    Vector impactNormal = normal.project(relativeVel) * mCoeff;
    a->vel -= impactNormal * (1/m1);
    b->vel += impactNormal * (1/m2);
    // There is a half impulse minimum of 120 (stated game mechanics).
    float impulse = impactNormal.getLength();
    // We ignore the min impulse rule for cases where the objects intersected
    // and have very little relative velocity. There is a decent chance that these
    // objects didn't touch.
    if(impulse < 120.0 && !(time() == 0 && relativeVel.getLengthSq() < 2)) {
        impactNormal *= 120.0 / impulse;
    }
    a->vel -= impactNormal * (1/m1);
    b->vel += impactNormal * (1/m2);
}

float Physics::turnAngle(const PodState& pod, const Vector& target) {
    float target_angle = Physics::angleTo(pod.pos, target);
    // Need to have >= and <= instead of > and < as we want 0 degrees instead of 360 when there is no turning to do.
    float turn_left = pod.angle >= target_angle ? pod.angle - target_angle : 2*M_PI - target_angle + pod.angle;
    float turn_right = pod.angle <= target_angle ? target_angle - pod.angle : 2*M_PI - pod.angle + target_angle;
    if(turn_right < turn_left) {
        return turn_right;
    } else {
        // Turning left is achieved by outputting a negative angle.
        return -turn_left;
    }
}

Vector Physics::forceFromTarget(const PodState& pod, Vector target, float thrust) {
    // TODO: check- should this be angleTo or turnAngle?
    float angle = Physics::angleTo(pod.pos, target) - pod.angle;
    if(angle < - MAX_ANGLE) angle = -MAX_ANGLE;
    else if(angle > MAX_ANGLE) angle = MAX_ANGLE;
    float newAngle = pod.angle + angle;
    Vector force = Vector::fromMagAngle(thrust, newAngle);
    return force;
}

PodState Physics::move(const PodState& pod, const PodOutputAbs& control, float time) {
    // TODO: check- should this be angleTo or turnAngle?
    float angle = Physics::angleTo(pod.pos, control.target) - pod.angle;
    if(angle < - MAX_ANGLE) angle = -MAX_ANGLE;
    else if(angle > MAX_ANGLE) angle = MAX_ANGLE;
    float newAngle = pod.angle + angle;
    Vector force = Vector::fromMagAngle(control.thrust, newAngle);
    Vector newSpeed = (pod.vel + force); //* DRAG; Drag should be applied after moving.
    Vector pos = pod.pos + newSpeed * time;
    // Passed checkpoint test.
    int nextCheckpoint = pod.nextCheckpoint;
    // This checkpoint check can be removed the move method slit into two types (as many uses of
    // move don't need this reasonably expensive check).
    if(passedCheckpoint(pod.pos, pos, race.checkpoints[pod.nextCheckpoint])) {
        nextCheckpoint = (nextCheckpoint + 1) % race.checkpoints.size();
    }
    pos.x = (int) pos.x;//round(pos.x);
    pos.y = (int) pos.y;//round(pos.y);
    newSpeed.x = (int) (newSpeed.x * DRAG);
    newSpeed.y = (int) (newSpeed.y * DRAG);
    PodState nextState = PodState(pos, newSpeed, newAngle, nextCheckpoint);
    return nextState;
}

bool Physics::passedCheckpoint(const Vector& beforePos, const Vector& afterPos, const Vector& checkpoint) {
    return passedPoint(beforePos, afterPos, checkpoint, CHECKPOINT_RADIUS);
}

bool Physics::passedPoint(const Vector& beforePos, const Vector& afterPos, const Vector& target, float radius) {
    float time = passedCircleAt(beforePos.x, beforePos.y, afterPos.x, afterPos.y, target.x, target.y, radius);
    return time != -1;
}

float Physics::passedCircleAt(float beforePosX, float beforePosY, float afterPosX, float afterPosY, float targetX, float targetY, float radius) {
    float Dx = afterPosX -beforePosX;
    float Dy = afterPosY -beforePosY;
    float Fx = beforePosX - targetX;
    float Fy = beforePosY - targetY;
    // t^2(D*D) + 2t(F*D) + (F*F - r^2) = 0
    float a = Dx*Dx + Dy*Dy;
    float b = 2 * (Fx*Dx + Fy*Dy);
    // This could get up to 8.29e16. Long and even long long are two small.
    float c = (Fx*Fx + Fy*Fy) - radius * radius;
    float  discrimiminant = b * b - 4 * a * c;
    if(discrimiminant < 0) {
        return -1;
    } else if(discrimiminant == 0) {
        // The pod's path skims the radius.
        return - 1;
    } else {
        float disc = sqrt(discrimiminant);
        float t1 = (-b - disc)/(2*a);
        // Ignoring t2, which represents passing the circle from the inside || (t2 >= 0 && t2 <= 1);
        // float t2 = (-b + disc)/(2*a);
        // Ignoring t1 == 0 or t1 == 1 case. This seems reasonable, but may not be how the game deals with it.
        return (t1 > 0 && t1 < 1) ? t1 : -1;
    }
}

Vector Physics::closestPointOnLine(Vector lineStart, Vector lineEnd, Vector point) {
    return closestPointOnLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, point.x, point.y);
}

Vector Physics::closestPointOnLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY, float pointX, float pointY) {
    // Test if the closest point is one of the line segment end points.
    float pathX = lineEndX - lineStartX;
    float pathY = lineEndY - lineStartY;
    // While this next bit works for line segments, we need to calculate the collision point, which
    // needs to treat this method as closest point on
    if((pathX * (pointX - lineEndX) + pathY * (pointY - lineEndY)) > 0) {
        // Closest point is on the further side of lineEnd.
        return Vector(lineEndX, lineEndY);
    } else if((pathX * (lineStartX - pointX) + pathY * (lineStartY - pointY)) > 0) {
        // Closest point is on the further side of lineStart.
        return Vector(lineStartX, lineStartY);
    }
    // Closest point is on line segment.
    // Line equation: Ax + By = C1
    // Pependicular line though point equation: -Bx + Ay = C2
    float A = lineEndY - lineStartY;
    float B = lineStartX - lineEndX;
    float C1 = A * lineStartX + B * lineStartY;
    float C2 = -B * pointX + A * pointY;
    // Solve simultaneous equations for intersection.
    float det  = A*A - -B*B;
    if(det == 0) {
        // Point is on the line, and thus, is the closest point.
        return Vector(pointX, pointY);
    } else {
        // By Cramer's Rule:
        float px = (A*C1 - B * C2) / det;
        float py = (A*C2 - -B*C1) / det;
        return Vector(px, py);
    }
}

bool Physics::isCollision(const PodState& podA, const PodOutputAbs& controlA,
                          const PodState& podB, const PodOutputAbs& controlB, float velThreshold) {
    return isCollision(podA, controlA, podB, controlB, 1, velThreshold);
}

bool Physics::isCollision(const PodState &podA, const PodOutputAbs &controlA, const PodState &podB,
                          const PodOutputAbs &controlB, int turns, float velThreshold) {
    // Need to repeatedly apply the control. To do this, the relative force must be extracted and then reapplied.
    Vector relativeForceA = controlA.target - podA.pos;
    Vector relativeForceB = controlB.target - podB.pos;
    PodOutputAbs cA = controlA;
    PodOutputAbs cB = controlB;
    PodState a = podA;
    PodState b = podB;
    for(int i = 0; i < turns; i++) {
        cA.target = a.pos + relativeForceA;
        cB.target = b.pos + relativeForceB;
        a = move(a, cA, 1);
        b = move(b, cB, 1);
        Vector velDiff = a.vel - b.vel;
        bool isCollision = (a.pos - b.pos).getLength() < 2 * POD_RADIUS;
        bool meetsVelThreshold = abs(velDiff.getLength()) >= abs(velThreshold);
        if(isCollision && meetsVelThreshold) {
            return true;
        }
    }
    return false;
}

// Used for legacy pod.
PodOutputAbs Physics::expectedControl(const PodState& previous, const PodState& current) {
    Vector force = (current.vel * (1/DRAG)) - previous.vel;
    PodOutputAbs control(force.getLength(), current.pos + force);
    return control;
}

// Used for legacy pod.
PodState Physics::extrapolate(const PodState& pod, const PodOutputAbs& control, int turns) {
    Vector relativeForce = control.target - pod.pos;
    PodOutputAbs updatedControl;
    // Can either use simulation or geometric sums.
    PodState p = pod;
    for(int i = 0; i < turns; i++) {
        updatedControl = PodOutputAbs(control.thrust, pod.pos + relativeForce);
        p = move(p, updatedControl, turns);
    }
    return p;
}

int Physics::leadPodID(PodState pods[]) {
    if(pods[0].passedCheckpoints > pods[1].passedCheckpoints) {
        // Already in order.
        return 0;
    } else {
        // Another bottleneck spot, so resorting to manual computation.
        float diffX1 = race.checkpoints[pods[0].nextCheckpoint].x - pods[0].pos.x;
        float diffY1 = race.checkpoints[pods[0].nextCheckpoint].y - pods[0].pos.y;
        float diffX2 = race.checkpoints[pods[1].nextCheckpoint].x - pods[1].pos.x;
        float diffY2 = race.checkpoints[pods[1].nextCheckpoint].y - pods[1].pos.y;
        if(pods[1].passedCheckpoints > pods[0].passedCheckpoints ||
           (diffX1*diffX1 + diffY1*diffY1) > (diffX2*diffX2 + diffY2*diffY2)) {
            return 1;
        }
    }
    return 0;
}

// Return true if the pods needed reordering.
bool Physics::orderByProgress(PodState pods[]) {
    if(leadPodID(pods) == 1) {
        // Swap.
        PodState temp = pods[0];
        pods[0] = pods[1];
        pods[1] = temp;
        return true;
    }
    return false;
}

inline float safeAcos(float arg) {
    if(arg <= -1.0f) arg = -1.0f;
    else if(arg >= 1.0f) arg = 1.0f;
    return acos(arg);
}

float Physics::angleTo(const Vector& fromPoint, const Vector& toPoint) {
    // This method is a bottleneck, so using Vector is avoided.
    float diffX = toPoint.x - fromPoint.x;
    float diffY = toPoint.y - fromPoint.y;
    float length = sqrt(diffX*diffX + diffY*diffY);
    float angle = safeAcos(diffX / length);
//    float angle = acos(diffX / length);
    if(diffY < 0) {
        angle = 2 * M_PI - angle;
    }
    return angle;
}

float Physics::angleBetween(const Vector& from, const Vector& to) {
//    float angle = acos(from.dotProduct(to) / (float) (from.getLength() * to.getLength()));
    float angle = safeAcos(from.dotProduct(to) / (float) (from.getLength() * to.getLength()));
    if(to.y < from.y) {
        angle = 2 * M_PI - angle;
    }
    return angle;
}

float Physics::radToDegrees(float radians) {
    return 180.0 * radians / M_PI;
}

float Physics::degreesToRad(float degrees) {
    return M_PI * (degrees / 180.0);
}
