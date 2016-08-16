#ifndef CODERSSTRIKEBACK_ANNEALINGBOT_H
#define CODERSSTRIKEBACK_ANNEALINGBOT_H

#include <limits>
#include <cstdlib>

#include "State.h"
#include "Bot.h"
#include "Navigation.h"
#include "Physics.h"


class CustomAI : public SimBot {
    vector<PairOutput> moves;
    Race* race;
    int turn = 0;
public:
    CustomAI(vector<PairOutput> moves) : moves(moves) {}

    void init(Race& r) {
        race = &r;
    }
    void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) {
        Physics::apply(ourPods, moves[turn++]);
    }
};

/**
 * Bot with very low computational requirements.
 */
class MinimalBot : public SimBot {
    Race race;
    Physics physics;
    Navigation nav;
public:
    MinimalBot() {}

    MinimalBot(Race& race) {
        init(race);
    }

    void init(Race& r) {
        race = r;
        physics = Physics(race);
        nav = Navigation(race);
    }

    /**
     * ourPods and enemyPods must be orderd by progress; it is assumed that ourPods[0] is our lead pod.
     */
    void move(vector<PodState> &ourPods, vector<PodState> &enemyPods) {
        Vector target = race.checkpoints[ourPods[0].nextCheckpoint].pos - ourPods[0].vel * 4;
//        float cut_off = M_PI / 2 + MAX_ANGLE;
//        float angle_threshold = M_PI * (20.0f / 180.0f);
        float turn_angle = physics.turnAngle(ourPods[0], target);
//        float thrust;
//        float clippedAngle;
//        if(abs(turn_angle) > cut_off) {
//            thrust = 0;
//        } else if(abs(turn_angle) > angle_threshold) {
//            float spread = 0.3;
//            float x = abs(spread * (turn_angle - angle_threshold) / (cut_off - angle_threshold));
//            float factor =  1 - sqrt(1 - exp(-(2 / M_PI) * x * x));
//            thrust *= factor;
//        }
        float clippedAngle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : max(MAX_ANGLE, turn_angle);
        Vector force = Vector::fromMagAngle(MAX_THRUST, clippedAngle);
        ourPods[0].vel.x += force.x;
        ourPods[0].vel.y += force.y;
        ourPods[0].angle += clippedAngle;
//        PodOutputAbs output = nav.turnSaturationAdjust(ourPods[0], PodOutputAbs(MAX_THRUST, target));

        float targetx =  enemyPods[0].pos.x + (race.checkpoints[enemyPods[0].nextCheckpoint].pos.x - enemyPods[0].pos.x) * 0.80;
        float targety =  enemyPods[0].pos.y + (race.checkpoints[enemyPods[0].nextCheckpoint].pos.y - enemyPods[0].pos.y) * 0.80;
        target = Vector(targetx, targety);
        turn_angle = physics.turnAngle(ourPods[1],target);
        clippedAngle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : max(MAX_ANGLE, turn_angle);
        force = Vector::fromMagAngle(MAX_THRUST, clippedAngle);
        ourPods[1].vel.x += force.x;
        ourPods[1].vel.y += force.y;
        ourPods[1].angle += clippedAngle;
    };
};

class AnnealingBot : public DuelBot {
    Race race;
    Physics physics;
    static const int turns = 5;
    static constexpr float maxScore = numeric_limits<float>::infinity();
    static constexpr float minScore = -numeric_limits<float>::infinity();

    vector<PairOutput> randomSolution();

    vector<PairOutput> train(const vector<PodState> &podsToTrain, const vector<PodState> &opponentPods);

    void simulate(vector<PodState> &pods1, SimBot *pods1Sim, vector<PodState> &pods2, SimBot *pods2Sim, int turns);

    float score(vector<PodState> &pods, vector<PodState> &enemyPods);

    float score(vector<PodState> pods, vector<PairOutput> solution, vector<PodState> enemyPods);

    PairOutput random();

public:
    AnnealingBot() {}

    AnnealingBot(Race& r) {
        init(r);
    }

    void init(Race& r) {
        race = r;
        physics = Physics(race);
    }

    PairOutput move(GameState& gameState) {
        const vector<PodState> &ourPods = gameState.ourState().pods;
        const vector<PodState> &enemyPods = gameState.enemyState().pods;
        vector<PairOutput> solution = train(ourPods, enemyPods);
        return solution[0];
    };
};


#endif //CODERSSTRIKEBACKC_ANNEALINGBOT_H
