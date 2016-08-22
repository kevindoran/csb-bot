#ifndef CODERSSTRIKEBACK_SIMULATION_H
#define CODERSSTRIKEBACK_SIMULATION_H

#include "State.h"
#include "AnnealingBot.h"
#include "Physics.h"
#include "json.hpp"

using json = nlohmann::json;

class GameHistory {
public:
    vector<Vector> checkpoints;
    vector<PodState> player1Pod1;
    vector<PodState> player1Pod2;
    vector<PodState> player2Pod1;
    vector<PodState> player2Pod2;

    GameHistory(vector<Vector> checkpoints) : checkpoints(checkpoints){};

    void recordTurn(PodState p11, PodState p12, PodState p21, PodState p22) {
        player1Pod1.push_back(p11);
        player1Pod2.push_back(p12);
        player2Pod1.push_back(p21);
        player2Pod2.push_back(p22);
    }

    json toJson(PodState& podState) {
        json j;
        j["pos"]["x"] = podState.pos.x;
        j["pos"]["y"] = podState.pos.y;
        j["vel"]["x"] = podState.vel.x;
        j["vel"]["y"] = podState.vel.y;
        j["angle"] = podState.angle;
        j["shieldEnabled"] = podState.shieldEnabled;
        return j;
    }

    json toJson(Vector& vec) {
        json v;
        v["pos"]["x"] = vec.x;
        v["pos"]["y"] = vec.y;
        return v;
    }

    void writeToStream(ostream& out) {
        json game;
        json cps;
        for(int i = 0; i < checkpoints.size(); i++) {
            cps.push_back(toJson(checkpoints[i]));
        }
        game["checkpoints"] = cps;
        json turns;
        for(int i = 0; i < player1Pod1.size(); i++) {
            json bothStates;
            bothStates["p1pod1"] = toJson(player1Pod1[i]);
            bothStates["p1pod2"] = toJson(player1Pod2[i]);
            bothStates["p2pod1"] = toJson(player2Pod1[i]);
            bothStates["p2pod2"] = toJson(player2Pod2[i]);
            turns.push_back(bothStates);
        }
        game["turns"] = turns;
        out << game;
    }
};


class Simulation {
    Race race;
    Physics physics;

    // Place pods along a line at checkpoint 0 facing checkpoint 1.
    void initializePods(PodState aPods[], PodState bPods[]) {
        Vector facingDirection = race.checkpoints[1] - race.checkpoints[0];
        Vector startLine = facingDirection.tanget().normalize();
        float gap = POD_RADIUS + 100;
        Vector posA1 = race.checkpoints[0] + startLine * gap;
        Vector posA2 = race.checkpoints[0] - startLine * gap;
        Vector posB1 = race.checkpoints[0] +  startLine * 3 * gap;
        Vector posB2 = race.checkpoints[0] - startLine * 3 * gap;
        float angle = Physics::angleTo(posA1, race.checkpoints[1]);
        aPods[0] = PodState(posA1, Vector(0,0), angle, 1);
        angle = Physics::angleTo(posA2, race.checkpoints[1]);
        aPods[1] = PodState(posA2, Vector(0,0), angle, 1);
        angle = Physics::angleTo(posB1, race.checkpoints[1]);
        bPods[0] = PodState(posB1, Vector(0,0), angle, 1);
        angle = Physics::angleTo(posB2, race.checkpoints[1]);
        bPods[1] = PodState(posB2, Vector(0,0), angle, 1);
    }

    bool victory(PodState ourPods[], PodState enemyPods[]) {
        bool finished = ourPods[0].passedCheckpoints == race.totalCPCount() || ourPods[1].passedCheckpoints == race.totalCPCount();
        bool otherPlayerTimeout = enemyPods[0].turnsSinceCP > 100 && enemyPods[1].turnsSinceCP > 100;
        return finished || otherPlayerTimeout;
    }


    void stripAndCombine(PodState ourStates[], PodState enemyStates[], PlayerState out[]) {
        out[0].pods[0] = strip(ourStates[0]);
        out[0].pods[1] = strip(ourStates[1]);
        out[1].pods[0] = strip(enemyStates[0]);
        out[1].pods[1] = strip(enemyStates[1]);
    }

    // Create a pod state as if it was parsed form the game input.
    PodState strip(const PodState& podState) {
        PodState basic;
        basic.vel.x = (int)podState.vel.x;
        basic.vel.y = (int)podState.vel.y;
        basic.pos.x = (int)round(podState.pos.x);
        basic.pos.y = (int)round(podState.pos.y);
        basic.nextCheckpoint = podState.nextCheckpoint;
        basic.angle = physics.degreesToRad(round(physics.radToDegrees(podState.angle)));
        return basic;
    }

public:
    static const int TURN_LIMIT = 120;
    Simulation(Race r) : race(r), physics(r) {}

    GameHistory simulate(DuelBot* a, DuelBot* b) {
        PodState aPods[POD_COUNT];
        PodState bPods[POD_COUNT];
        initializePods(aPods, bPods);
        GameHistory history(race.checkpoints);
        a->init(race);
        b->init(race);
        PodState* pods[] = {&aPods[0], &aPods[1], &bPods[0], &bPods[1]};
        State stateA(race);
        State stateB(race);
        for(int i = 0; i < TURN_LIMIT; i++) {
            a = new AnnealingBot(race);
            history.recordTurn(*pods[0], *pods[1], *pods[2], *pods[3]);

            if (victory(aPods, bPods) || victory(bPods, aPods)) {
//                cout << "Finished. Winner is: bot #" << (victory(aPods, bPods) ? "1" : "2") << endl;
//                cout << "Victory on turn: " << i << endl;
                return history;
            }
            PlayerState forA[PLAYER_COUNT];
            PlayerState forB[PLAYER_COUNT];
            stripAndCombine(aPods, bPods, forA);
            stripAndCombine(bPods, aPods, forB);
            stateA.preTurnUpdate(forA);
            stateB.preTurnUpdate(forB);
            PairOutput aOut = a->move(stateA.game());
            PairOutput bOut = b->move(stateB.game());
            stateA.postTurnUpdate( aOut.o1.absolute(stateA.game().ourState().pods[0]),
                    aOut.o2.absolute(stateA.game().ourState().pods[1]));
            stateB.postTurnUpdate( bOut.o1.absolute(stateB.game().ourState().pods[0]),
                    bOut.o2.absolute(stateB.game().ourState().pods[1]));
            physics.apply(aPods, aOut);
            physics.apply(bPods, bOut);
            for(int z= 0; z < 3; z++) {
                for(int y = z + 1; y < 4; y++) {
                    if(Vector::dist(pods[z]->pos, pods[y]->pos) < 400) {
                        cerr << "There should have been a collision." << endl;
                    }
                }
            }
            physics.simulate(pods);
            for(int z= 0; z < 3; z++) {
                for(int y = z + 1; y < 4; y++) {
                    if(Vector::dist(pods[z]->pos, pods[y]->pos) < 400) {
                        cerr << "There should have been a collision." << endl;
                    }
                }
            }
        }
        cout << "Game reached turn limit.";
        delete(a);
        return history;
    }
};

#endif //CODERSSTRIKEBACK_SIMULATION_H
