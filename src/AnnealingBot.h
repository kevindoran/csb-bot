#ifndef CODERSSTRIKEBACK_ANNEALINGBOT_H
#define CODERSSTRIKEBACK_ANNEALINGBOT_H

#include <limits>
#include <cstdlib>

#include "State.h"
#include "Bot.h"
#include "Navigation.h"
#include "Physics.h"


class CustomAI : public SimBot {
    const PairOutput* moves;
    Race* race;
    int turn = 0;
public:
    CustomAI(const PairOutput moves[]) : moves(moves) {}

    void init(Race& r) {
        race = &r;
    }
    void move(PodState ourPods[], PodState enemyPods[]) {
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
    void move(PodState* ourPods, PodState* enemyPods) {
        // Racer
        Vector target = race.checkpoints[ourPods[0].nextCheckpoint] - ourPods[0].vel * 4;
        float turn_angle = physics.turnAngle(ourPods[0], target);
        Vector force;
        if(abs(turn_angle) > MAX_ANGLE) {
            turn_angle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : min(MAX_ANGLE, turn_angle);
            force = Vector::fromMagAngle(MAX_THRUST, turn_angle);
        } else {
            force = target.normalize() * MAX_THRUST;
        }
        ourPods[0].vel.x += force.x;
        ourPods[0].vel.y += force.y;
        ourPods[0].angle += turn_angle;

        // Bouncer
        float targetx =  enemyPods[0].pos.x + (race.checkpoints[enemyPods[0].nextCheckpoint].x - enemyPods[0].pos.x) * 0.80;
        float targety =  enemyPods[0].pos.y + (race.checkpoints[enemyPods[0].nextCheckpoint].y - enemyPods[0].pos.y) * 0.80;
        target = Vector(targetx, targety);
        turn_angle = physics.turnAngle(ourPods[1],target);
        if(abs(turn_angle) > MAX_ANGLE) {
            turn_angle = turn_angle < 0 ? max(-MAX_ANGLE, turn_angle) : min(MAX_ANGLE, turn_angle);
            force = Vector::fromMagAngle(MAX_THRUST, turn_angle);
        } else {
            force = target.normalize() * MAX_THRUST;
        }
        ourPods[1].vel.x += force.x;
        ourPods[1].vel.y += force.y;
        ourPods[1].angle += turn_angle;
    };
};

class AnnealingBot : public DuelBot {
    Race race;
    Physics physics;
    static const int turns = 6;
    static constexpr float maxScore = numeric_limits<float>::infinity();
    static constexpr float minScore = -numeric_limits<float>::infinity();
    MinimalBot enemyBot;
    PairOutput previousSolution[turns];
    bool hasPrevious = false;
    PodState enemySimHistory[turns + 1][POD_COUNT];
    PodState ourSimHistory[turns + 1][POD_COUNT];

    void train(const PodState podsToTrain[], const PodState opponentPods[], PairOutput solution[]);

    float score(const PodState* pods[], const PodState* enemyPods[]);

    float score(const PairOutput solution[], int startFromTurn);

    void simulate(SimBot *pods1Sim, SimBot *pods2Sim, int turns, int startFromTurn);

    void randomSolution(PairOutput sol[]);

    PairOutput random();

    void randomEdit(PairOutput &po);

public:
    AnnealingBot() {}

    AnnealingBot(Race& r) {
        init(r);
    }

    void init(Race& r) {
        race = r;
        physics = Physics(race);
        enemyBot = MinimalBot(race);
    }

    PairOutput move(GameState& gameState) {
        PodState* ourPods = gameState.ourState().pods;
        PodState* enemyPods = gameState.enemyState().pods;
        PairOutput solution[turns];
        train(ourPods, enemyPods, solution);
        PairOutput sol = solution[0];
        if(gameState.ourState().leadPodID != 0) {
            PodOutputSim temp = sol.o1;
            sol.o1 = sol.o2;
            sol.o2 = temp;
        }
        return sol;
    };
};


#endif //CODERSSTRIKEBACKC_ANNEALINGBOT_H
