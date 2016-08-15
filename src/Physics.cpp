
#define _USE_MATH_DEFINES
#include <cmath>
#include <algorithm>
#include <math.h>

#include "State.h"
#include "Physics.h"


PassedCheckpoint PassedCheckpoint::testForPassedCheckpoint(PodState& a, Race& race) {
    float time = Physics::passedCircleAt(a.pos, a.pos + a.vel, race.checkpoints[a.nextCheckpoint].pos, CHECKPOINT_RADIUS);
    if(time == -1) return invalid(a);
    else return PassedCheckpoint(a, time, race.followingCheckpoint(a.nextCheckpoint));
}

void PassedCheckpoint::resolve() {
    if(occurred()) {
        mPod.nextCheckpoint = mNextCheckpoint;
    }
}

void Physics::simulate(vector<PodState*> pods) {
    float time = 0;
    vector<PassedCheckpoint> pCPEvents;
    Collision earliest;
    bool started = false;
    bool hasCollision = false;
    while(time < 1) {
        for (int i = 0; i < pods.size() - 1; i++) {
            // Collision or checkpoint passing first?
            for (int j = i + 1; j < pods.size(); j++) {
                Collision collision = Collision::testForCollision(*pods[i], *pods[j]);
                if (collision.occurred() && (!hasCollision || earliest.time() > collision.time())) {
                    hasCollision = true;
                    earliest = collision;
                }
            }
            // Can optimize by keeping list of pc events and resolving all those before the earliest collision.
            PassedCheckpoint cpEvent = PassedCheckpoint::testForPassedCheckpoint(*pods[i], race);
            if (cpEvent.occurred()) {
                pCPEvents.push_back(cpEvent);
            }
        }
        for (auto& pcp : pCPEvents) {
            if (!hasCollision || pcp.time() < earliest.time()) {
                pcp.resolve();
            }
        }
        float moveTime = hasCollision ? earliest.time() : 1;

        for(auto& pod : pods) {
            pod->pos += pod->vel * (moveTime - time);
        }
        if (hasCollision) {
            earliest.resolve();
            hasCollision = false;
        }
        time = moveTime;
    }

    // Drag and rounding.
    for(auto& pod : pods) {
        pod->vel = pod->vel * DRAG;
        pod->vel.x = (int) pod->vel.x;
        pod->vel.y = (int) pod->vel.y;
        pod->pos.x = (int) pod->pos.x;
        pod->pos.y = (int) pod->pos.y;
    }
}

Collision Collision::testForCollision(PodState& a, PodState& b) {
    // Change reference frame to a. a pos = (0, 0)
//    Vector pathStart = b.pos - a.pos;
//    Vector vel = b.vel - a.vel;
//    Vector pathEnd = pathStart + vel;
    float pathStartX = b.pos.x - a.pos.x;
    float pathStartY = b.pos.y - a.pos.y;
    float velX = b.vel.x - a.vel.x;
    float velY = b.vel.y - a.vel.y;
    float pathEndX = pathStartX + velX;
    float pathEndY = pathStartY + velY;

    Vector closestPoint = Physics::closestPointOnLine(pathStartX, pathStartY, pathEndX, pathEndY, Vector(0,0));
    if(closestPoint.getLengthSq() > (2*POD_RADIUS)*(2*POD_RADIUS)) {
        return invalidCollision();
    } else {
        float backDist = sqrt((POD_RADIUS*2)*(POD_RADIUS*2) - closestPoint.getLengthSq());
        float velLength = sqrt(velX * velX + velY * velY);
        float bCenterX = closestPoint.x - backDist * (velX / velLength);
        float bCenterY = closestPoint.y - backDist * (velY / velLength);
        float travelX = b.pos.x - bCenterX;
        float travelY = b.pos.y - bCenterY;
        float travelDist = sqrt(travelX*travelX + travelY * travelY);
        float time = travelDist / velLength;
//        Vector collisionPoint = a.vel * time + bCenter * 0.5;
        // Reset to normal reference frame.
        // Turns out that the actual collision point is not important, as the bots move to it before resolving the
        // collision.
//        collisionPoint = collisionPoint + a.pos;
        return Collision(a, b, time);
    }
}

void Collision::resolve() {
    float m1 = a->shieldEnabled ? 10 : 1;
    float m2 = b->shieldEnabled ? 10 : 1;
    float mCoeff = (m1 * m2) / (m1 + m2);
//    Vector normal = (collisionPoint - a->pos).normalize();
    Vector normal = (b->pos - a->pos).normalize();
    Vector relativeVel = a->vel - b->vel;
    Vector impactNormal = normal.project(relativeVel) * mCoeff;
    a->vel -= impactNormal * (1/m1);
    b->vel += impactNormal * (1/m2);
    // There is a half impulse minimum of 120.
    float impulse = impactNormal.getLength();
    if(impulse < 120.0) {
        impulse *= 120.0 / impulse;
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

PodState Physics::move(const PodState& pod, const PodOutput& control, float time) {
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
    // Need rounding somewhere, or maybe just truncating.
    pos.x = (int) pos.x;
    pos.y = (int) pos.y;
    newSpeed.x = (int) (newSpeed.x * DRAG);
    newSpeed.y = (int) (newSpeed.y * DRAG);
    PodState nextState = PodState(pos, newSpeed, newAngle, nextCheckpoint);
    return nextState;
}


bool Physics::passedCheckpoint(const Vector& beforePos, const Vector& afterPos, const Checkpoint& checkpoint) {
    return passedPoint(beforePos, afterPos, checkpoint.pos, CHECKPOINT_RADIUS);
}
bool Physics::passedPoint(const Vector& beforePos, const Vector& afterPos, const Vector& target, float radius) {
    float time = passedCircleAt(beforePos, afterPos, target, radius);
    return time != -1;
}

float Physics::passedCircleAt(const Vector &beforePos, const Vector &afterPos, const Vector &target, float radius) {
    Vector D = afterPos -beforePos;
    Vector F = beforePos - target;
    // t^2(D*D) + 2t(F*D) + (F*F - r^2) = 0
    long a = D.dotProduct(D);
    long b = 2 * F.dotProduct(D);
    long c = F.dotProduct(F) - radius * radius;
    long discrimiminant = b * b - 4 * a * c;
    if(discrimiminant < 0) {
        return -1;
    } else {
        float disc = sqrt(discrimiminant);
        float t1 = (-b - disc)/(2*a);
        float t2 = (-b + disc)/(2*a);
        return (t1 >= 0 && t1 <= 1) ? t1 : -1; // Ignoring t2, which represents passing the circle from the inside || (t2 >= 0 && t2 <= 1);
    }
}

Vector Physics::closestPointOnLine(Vector lineStart, Vector lineEnd, Vector point) {
    return closestPointOnLine(lineStart.x, lineStart.y, lineEnd.x, lineEnd.y, point);
}
Vector Physics::closestPointOnLine(float lineStartX, float lineStartY, float lineEndX, float lineEndY, Vector point) {
    // Test if the closest point is one of the line segment end points.
    float pathX = lineEndX - lineStartX;
    float pathY = lineEndY - lineStartY;
//    if(path.dotProduct(point - lineEnd) > 0) {
    if((pathX * (point.x - lineEndX) + pathY * (point.y - lineEndY)) > 0) {
        // Closest point is on the further side of lineEnd.
        return Vector(lineEndX, lineEndY);
//    } else if((-path).dotProduct(point - lineStart) > 0) {
    } else if((pathX * (lineStartX - point.x) + pathY * (lineStartY - point.y)) > 0) {
        // Closest point is on the further side of lineStart.
        return Vector(lineStartX, lineStartY);
    }
    // Closest point is on line segment.
    // Line equation: Ax + By = C1
    // Pependicular line though point equation: -Bx + Ay = C2
    float A = lineEndY - lineStartY;
    float B = lineStartX - lineEndX;
    float C1 = A * lineStartX + B * lineStartY;
    float C2 = -B * point.x + A * point.y;
    // Solve simultaneous equations for intersection.
    float det  = A*A - -B*B;
    if(det == 0) {
        // Point is on the line, and thus, is the closest point.
        return point;
    } else {
        // By Cramer's Rule:
        float px = (A*C1 - B * C2) / det;
        float py = (A*C2 - -B*C1) / det;
        return Vector(px, py);
    }
}

bool Physics::isCollision(const PodState& podA, const PodOutput& controlA,
                          const PodState& podB, const PodOutput& controlB, float velThreshold) {
    return isCollision(podA, controlA, podB, controlB, 1, velThreshold);
}

bool Physics::isCollision(const PodState &podA, const PodOutput &controlA, const PodState &podB,
                          const PodOutput &controlB, int turns, float velThreshold) {
    // Need to repeatedly apply the control. To do this, the relative force must be extracted and then reapplied.
    Vector relativeForceA = controlA.target - podA.pos;
    Vector relativeForceB = controlB.target - podB.pos;
    PodOutput cA = controlA;
    PodOutput cB = controlB;
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

PodOutput Physics::expectedControl(const PodState& previous, const PodState& current) {
    Vector force = (current.vel * (1/DRAG)) - previous.vel;
    PodOutput control(force.getLength(), current.pos + force);
    return control;
}

PodState Physics::extrapolate(const PodState& pod, const PodOutput& control, int turns) {
    Vector relativeForce = control.target - pod.pos;
    PodOutput updatedControl;
    // Can either use simulation or geometric sums.
    PodState p = pod;
    for(int i = 0; i < turns; i++) {
        updatedControl = PodOutput(control.thrust, pod.pos + relativeForce);
        p = move(p, updatedControl, turns);
    }
    return p;
}

void Physics::orderByProgress(vector<PodState>& pods) {
    if(pods[0].passedCheckpoints > pods[1].passedCheckpoints) {
        // Already in order.
        return;
    } else if(pods[1].passedCheckpoints > pods[0].passedCheckpoints
              || (race.checkpoints[pods[0].nextCheckpoint].pos - pods[0].pos).getLengthSq()
                 > (race.checkpoints[pods[1].nextCheckpoint].pos - pods[1].pos).getLengthSq()) {
        // Swap.
        PodState& temp = pods[0];
        pods[0] = pods[1];
        pods[1] = temp;
    }
}

float Physics::angleTo(const Vector& fromPoint, const Vector& toPoint) {
    Vector diff = toPoint - fromPoint;
    float angle = acos(diff.x / diff.getLength());
    if(diff.y < 0) {
        angle = 2 * M_PI - angle;
    }
    return angle;
}

float Physics::angleBetween(const Vector& from, const Vector& to) {
    float angle = acos(from.dotProduct(to) / (float) (from.getLength() * to.getLength()));
    if(to.y < from.y) {
        angle = 2 * M_PI - angle;
    }
    return angle;
}

float Physics::radToDegrees(float radians) {
    return 180 * radians / M_PI;
}

float Physics::degreesToRad(float degrees) {
    return M_PI * (degrees / 180);
}
