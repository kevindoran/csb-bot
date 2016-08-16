#ifndef CODERSSTRIKEBACK_SIMULATION_H
#define CODERSSTRIKEBACK_SIMULATION_H

#include "State.h"
#include "AnnealingBot.h"
#include "Physics.h"

class GameHistory {
public:
    vector<PodState> player1Pod1;
    vector<PodState> player1Pod2;
    vector<PodState> player2Pod1;
    vector<PodState> player2Pod2;

    GameHistory() {};

    void recordTurn(PodState p11, PodState p12, PodState p21, PodState p22) {
        player1Pod1.push_back(p11);
        player1Pod2.push_back(p12);
        player1Pod1.push_back(p21);
        player2Pod2.push_back(p22);
    }
};


class Simulation {
    Race race;
    Physics physics;

    // Place pods along a line at checkpoint 0 facing checkpoint 1.
    void initializePods(vector<PodState>& aPods, vector<PodState>& bPods) {
        Vector facingDirection = race.checkpoints[1].pos - race.checkpoints[0].pos;
        Vector startLine = facingDirection.tanget().normalize();
        float gap = POD_RADIUS + 100;
        Vector posA1 = race.checkpoints[0].pos + startLine * gap;
        Vector posA2 = race.checkpoints[0].pos - startLine * gap;
        Vector posB1 = race.checkpoints[0].pos +  startLine * (2*gap + 100);
        Vector posB2 = race.checkpoints[0].pos - startLine * (2*gap + 100);
        float angle = Physics::angleTo(posA1, race.checkpoints[1].pos);
        aPods[0] = PodState(posA1, Vector(0,0), angle, 1);
        angle = Physics::angleTo(posA2, race.checkpoints[1].pos);
        aPods[1] = PodState(posA2, Vector(0,0), angle, 1);
        angle = Physics::angleTo(posB1, race.checkpoints[1].pos);
        bPods[0] = PodState(posB1, Vector(0,0), angle, 1);
        angle = Physics::angleTo(posB2, race.checkpoints[1].pos);
        bPods[1] = PodState(posB2, Vector(0,0), angle, 1);
    }

    bool victory(const vector<PodState>& ourPods, const vector<PodState>& enemyPods) {
        bool finished = ourPods[0].passedCheckpoints == race.totalCPCount() || ourPods[1].passedCheckpoints == race.totalCPCount();
        bool otherPlayerTimeout = enemyPods[0].turnsSinceCP > 100 && enemyPods[1].turnsSinceCP > 100;
        return finished || otherPlayerTimeout;
    }


    vector<PlayerState> stripAndCombine(const vector<PodState>& ourStates, const vector<PodState>& enemyStates) {
        vector<PlayerState> out;
        PlayerState us({strip(ourStates[0]), strip(ourStates[1])});
        PlayerState enemy({strip(enemyStates[0]), strip(enemyStates[1])});
        out.push_back(us);
        out.push_back(enemy);
        return out;
    }

    // Create a pod state as if it was parsed form the game input.
    PodState strip(const PodState& podState) {
        PodState basic;
        basic.pos = podState.pos;
        basic.vel = podState.vel;
        basic.nextCheckpoint = podState.nextCheckpoint;
        basic.angle = podState.angle;
        return basic;
    }

public:
    static const int TURN_LIMIT = 2000;
    Simulation(Race r) : race(r), physics(r) {}

    GameHistory simulate(DuelBot* a, DuelBot* b) {
        vector<PodState> aPods = {PodState(), PodState()};
        vector<PodState> bPods = {PodState(), PodState()};
        initializePods(aPods, bPods);
        GameHistory history;
        a->init(race);
        b->init(race);
        vector < PodState * > pods;
        pods.push_back(&aPods[0]);
        pods.push_back(&aPods[1]);
        pods.push_back(&bPods[0]);
        pods.push_back(&bPods[1]);
        State stateA(race);
        State stateB(race);
        for(int i = 0; i < TURN_LIMIT; i++) {
            history.recordTurn(*pods[0], *pods[1], *pods[2], *pods[3]);

            if (victory(aPods, bPods) || victory(bPods, aPods)) {
                cout << "Finished. Winner is: bot #" << (victory(aPods, bPods) ? "1" : "2") << endl;
                return history;
            }
            stateA.preTurnUpdate(stripAndCombine(aPods, bPods));
            stateB.preTurnUpdate(stripAndCombine(bPods, aPods));
            PairOutput aOut = a->move(stateA.game());
            PairOutput bOut = b->move(stateB.game());
            stateA.postTurnUpdate( aOut.o1.absolute(stateA.game().ourState().pods[0]),
                    aOut.o2.absolute(stateA.game().ourState().pods[1]));
            stateB.postTurnUpdate( bOut.o1.absolute(stateB.game().ourState().pods[0]),
                    bOut.o2.absolute(stateB.game().ourState().pods[1]));
            physics.apply(aPods, aOut);
            physics.apply(bPods, bOut);
            physics.simulate(pods);
        }
        cout << "Game reached turn limit.";
        return history;
    }
};

#endif //CODERSSTRIKEBACK_SIMULATION_H
