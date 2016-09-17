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

    bool timeout(PodState pods[]) {
        return pods[0].turnsSinceCP > 100 && pods[1].turnsSinceCP > 100;
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
    GameHistory history;
    static const int TURN_LIMIT = 250;
    Simulation(Race r) : race(r), physics(r), history(r.checkpoints){}

    int parameterSim(PodState aPods[], PodState bPods[], ScoreFactors sFactors, bool printOut) {
        PodState* pods[] = {&aPods[0], &aPods[1], &bPods[0], &bPods[1]};
        for(int i = 0; i < TURN_LIMIT; i++) {
            if(printOut) {
                history.recordTurn(*pods[0], *pods[1], *pods[2], *pods[3]);
            }
            // If game over (by aPods victory), return the number of turns.
            if (aPods[0].passedCheckpoints == race.totalCPCount()) {
                return i;
            }
            // Setup game data.
            PlayerState forA[PLAYER_COUNT] = {PlayerState(aPods), PlayerState(bPods)};
            PlayerState forB[PLAYER_COUNT] = {PlayerState(bPods), PlayerState(aPods)};
            int turn = i+1; // skip first turn to avoid initialization which will reset out fake setup.
            GameState aGS(race, forA, i+1);
            GameState bGS(race, forB, i+1);

            // Train pod 1
            PairOutput bouncerSolution[5];
            AnnealingBot<5> bouncerBotFake(race, 40);
            bouncerBotFake.sFactors.overallRacer = 0;
            PodState stateExpected[5][2];
            bouncerBotFake.train(aGS.enemyState().pods, aGS.ourState().pods, bouncerSolution, stateExpected[0], true);
            CustomAIWithBackup<5> bouncerAI(race, bouncerSolution, stateExpected, 0);
            bouncerAI.setDefaultAfter(5);
            AnnealingBot<6> racerBot(race, 80, &bouncerAI);
            // Disable bouncer.
            racerBot.sFactors.overallBouncer = 0;

            // Train pod 2
            PairOutput racerSolution[5];
            AnnealingBot<5> racerBotFake(race, 40);
            racerBotFake.sFactors.overallBouncer = 0;
            PodState stateExpected2[5][2];
            racerBotFake.train(bGS.enemyState().pods, bGS.ourState().pods, racerSolution, stateExpected[0], true);
            CustomAIWithBackup<5> racerAI(race, racerSolution, stateExpected2, 0);
            racerAI.setDefaultAfter(5);
            AnnealingBot<6> bouncerBot(race, 80, &racerAI);
            bouncerBot.sFactors = sFactors;
            // Disable racer.
            bouncerBot.sFactors.overallRacer = 0;

            // Play the turn.
            PairOutput aOut = racerBot.move(aGS);
            PairOutput bOut = bouncerBot.move(bGS);
            physics.apply(aPods, aOut);
            physics.apply(bPods, bOut);
            physics.simulate(pods);
        }
        return TURN_LIMIT;
    }



    float progressDiff(PodState aPods[], PodState bPods[]) {
        int aLead = physics.leadPodID(aPods);
        int bLead = physics.leadPodID(bPods);
        float aProgress = progress(aPods[aLead]);
        float bProgress = progress(bPods[bLead]);
        return aProgress - bProgress;
    }

    float checkpointsToGo(PodState winner[]) {
        int lead = physics.leadPodID(winner);
        return race.totalCPCount() - winner[lead].passedCheckpoints;
    }


    static constexpr float CP_BONUS = 2000;
    double progress(PodState& racer) {
        double progress = 0;
        for(int i = 1; i <= racer.passedCheckpoints; i++) {
            int cpIdx = i % race.checkpoints.size();
            progress += CP_BONUS;
            progress += race.distFromPrevCP(cpIdx);
        }
        int curCPIdx = (racer.passedCheckpoints + 1) % race.checkpoints.size();
        progress += race.distFromPrevCP(curCPIdx) - Vector::dist(racer.pos, race.checkpoints[curCPIdx]);
        return progress;
    }

    static const int PROGRESS_LIMIT = 100000;
    static const int SCORE_LIMIT =   1000000;
    static const int EARLY_VICTORY_BONUS = 1000;
    double fullGameParamSim(ScoreFactors sFactors, bool printOut) {
        PodState aPods[POD_COUNT];
        PodState bPods[POD_COUNT];
        initializePods(aPods, bPods);
        PodState* pods[] = {&aPods[0], &aPods[1], &bPods[0], &bPods[1]};
        for(int i = 0; true;i++) {
            if(printOut) {
                history.recordTurn(*pods[0], *pods[1], *pods[2], *pods[3]);
            }
            double pdiff = progressDiff(bPods, aPods);
            if (victory(aPods, bPods) || victory(bPods, aPods) || abs(pdiff) > PROGRESS_LIMIT) {
                float score = pdiff;
                if(pdiff > PROGRESS_LIMIT || timeout(bPods)) {
                    score += EARLY_VICTORY_BONUS * checkpointsToGo(bPods);
                } else if(pdiff < -PROGRESS_LIMIT || timeout(aPods)) {
                    score -= EARLY_VICTORY_BONUS * checkpointsToGo(aPods);
                }
                if(score > SCORE_LIMIT) {
                    cerr << "Score limit reached at: " << score << endl;
                    score = 0;
                }
                return score;
            }

            // Setup game data.
            PlayerState forA[PLAYER_COUNT] = {PlayerState(aPods), PlayerState(bPods)};
            PlayerState forB[PLAYER_COUNT] = {PlayerState(bPods), PlayerState(aPods)};
            int turn = i+1; // skip first turn to avoid initialization which will reset out fake setup.
            GameState aGS(race, forA, i+1);
            GameState bGS(race, forB, i+1);

            // Train pod 1
            PairOutput bouncerSolution[5];
            AnnealingBot<5> bouncerBotFake(race, 25);
//            bouncerBotFake.isControl = true;
            PodState stateExpected[5][2];
            bouncerBotFake.train(aGS.enemyState().pods, aGS.ourState().pods, bouncerSolution, stateExpected[0], true);
            CustomAIWithBackup<5> bouncerAI(race, bouncerSolution, stateExpected, 0);
            bouncerAI.setDefaultAfter(5);
            AnnealingBot<6> racerBot(race, 70, &bouncerAI);
//            racerBot.isControl = true;

            // Train pod 2
            PairOutput racerSolution[5];
            AnnealingBot<5> racerBotFake(race, 25);
            racerBotFake.sFactors = sFactors;
            PodState stateExpected2[5][2];
            racerBotFake.train(bGS.enemyState().pods, bGS.ourState().pods, racerSolution, stateExpected2[0], true);
            CustomAIWithBackup<5> racerAI(race, racerSolution, stateExpected2, 0);
            racerAI.setDefaultAfter(5);
            AnnealingBot<6> bouncerBot(race, 70, &racerAI);
            bouncerBot.sFactors = sFactors;

            // Play the turn.
            PairOutput aOut = racerBot.move(aGS);
            PairOutput bOut = bouncerBot.move(bGS);
            physics.apply(aPods, aOut);
            physics.apply(bPods, bOut);
            physics.simulate(pods);
        }
    }



    GameHistory simulate(DuelBot* a, DuelBot* b) {
        PodState aPods[POD_COUNT];
        PodState bPods[POD_COUNT];
        initializePods(aPods, bPods);
        GameHistory history(race.checkpoints);
        PodState* pods[] = {&aPods[0], &aPods[1], &bPods[0], &bPods[1]};
        State stateA(race);
        State stateB(race);
        for(int i = 0; i < TURN_LIMIT; i++) {
            a = new AnnealingBot<6>(race, 140);
            history.recordTurn(*pods[0], *pods[1], *pods[2], *pods[3]);

            if (victory(aPods, bPods) || victory(bPods, aPods)) {
//                cerr << "Finished. Winner is: bot #" << (victory(aPods, bPods) ? "1" : "2") << endl;
//                cerr << "Victory on turn: " << i << endl;
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
            physics.simulate(pods);
        }
        cout << "Game reached turn limit.";
        delete(a);
        return history;
    }
};

#endif //CODERSSTRIKEBACK_SIMULATION_H
