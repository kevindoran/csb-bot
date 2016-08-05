#ifndef CODERSSTRIKEBACKC_VECTOR_H
#define CODERSSTRIKEBACKC_VECTOR_H

#include <cmath>
#include <iostream>

using namespace std;

class Vector {
public:
    static const int UN_SET = -1;
    mutable double length = UN_SET;
    mutable double lengthSq = UN_SET;
    double x;
    double y;

    Vector() {}

    Vector(double x, double y) : x(x), y(y) {}

    Vector(double x, double y, double length, double lengthSq) : x(x), y(y), length(length), lengthSq(lengthSq) {}

    // Use default.
    //Vector(const Vector& vector);

    ~Vector() {};

    static Vector fromMagAngle(double magnitude, double angle) {
        return Vector(magnitude * cos(angle), magnitude * sin(angle));
    }

    Vector project(Vector p) const {
        return (*this * (dotProduct(p) / getLengthSq()));
    }

    Vector normalize() const {
        return Vector(x / getLength(), y / getLength());
    }

    Vector &operator+=(const Vector &other) {
        x += other.x;
        y += other.y;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator+(const Vector &other) const {
        return Vector(*this) += other;
    }

    Vector &operator-=(const Vector &other) {
        x -= other.x;
        y -= other.y;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator-(const Vector &other) const {
        return Vector(*this) -= other;
    }

    Vector operator-() const {
        return Vector(-x, -y);
    }

    Vector operator*=(double scalar) {
        x *= scalar;
        y *= scalar;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    Vector operator*(double scalar) const {
        return Vector(*this) *= scalar;
    }

    Vector &operator/=(double scalar) {
        x /= scalar;
        y /= scalar;
        length = UN_SET;
        lengthSq = UN_SET;
        return *this;
    }

    friend ostream &operator<<(ostream &os, const Vector &vector) {
        os << "(" << vector.x << "," << vector.y << ")";
        return
        os;
    }

    bool operator==(const Vector& other) const {
        return (x == other.x && y == other.y);
    }

    bool operator !=(const Vector& other) const {
        return !(*this == other);
    }

    double getX() const {
        return x;
    }

    double getY() const {
        return y;
    }

    double getLength() const {
        if(length == UN_SET) {
            length = sqrt(getLengthSq());
        }
        return length;
    }

    double getLengthSq() const {
        if(lengthSq == UN_SET) {
            lengthSq = x*x + y*y;
        }
        return lengthSq;
    }

    double dotProduct(const Vector& other) const {
        return x * other.x + y * other.y;
    };
};

#endif //CODERSSTRIKEBACKC_VECTOR_H
//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_GAMESTATE_H
#define CODERSSTRIKEBACKC_GAMESTATE_H

#include <vector>
#include <string>
#include <sstream>


using namespace std;

struct Checkpoint {
    const Vector pos;
    const int order;

    Checkpoint(int x, int y, int order) :
            pos(x, y), order(order) {}
};

struct Race {
    static const int POD_COUNT = 2;
    static const int PLAYER_COUNT = 2;
    static const int CHECKPOINT_WIDTH = 600;
    const int laps;
    const vector<Checkpoint> checkpoints;
//    const int CHECKPOINT_COUNT;

    Race(int laps, vector<Checkpoint> checkpoints) :
            laps(laps), checkpoints(checkpoints) {};
};

struct PodState {
    const Vector pos;
    const Vector vel;
    // In radians
    const int angle;
    const int nextCheckpoint;

    PodState(int x, int y, int vx, int vy, int angle, int nextCheckpoint) :
            pos(x, y), vel(vx, vy), angle(angle), nextCheckpoint(nextCheckpoint) { }

    PodState(Vector pos, Vector vel, int angle, int nextCheckpoint) :
            pos(pos), vel(vel), angle(angle), nextCheckpoint(nextCheckpoint) {}

    bool operator ==(const PodState& other) const {
        return (pos == other.pos && vel == other.vel && angle == other.angle && nextCheckpoint == other.nextCheckpoint);
    }

    bool operator !=(const PodState& other) const {
        return !(*this == other);
    }
};

struct PlayerState {
//    static const int POD_COUNT = 2;
    vector<PodState> pods;
    const int id; // Why is this needed?
    PlayerState(int id, vector<PodState> pods) : id(id), pods(pods) {}
};

struct GameState {
    const Race race;
    const vector<PlayerState> playerStates;
    const int ourPlayerId;
    const int turn;

    GameState(Race race, vector<PlayerState>& playerStates, int ourPlayerId, int turn) :
            race(race), playerStates(playerStates), ourPlayerId(ourPlayerId), turn(turn) {}

    PlayerState ourState() {
        return playerStates[ourPlayerId];
    }
};

struct PodOutput {
    const int thrust;
    const Vector direction;
    static const int BOOST = -1;
    static const int SHIELD = -2;

    PodOutput(int thrust, Vector direction) :
            thrust(thrust), direction(direction) {}

    string toString() {
        stringstream out;
        string thrustStr;
        if(thrust == BOOST) {
            thrustStr = "BOOST";
        } else if(thrust == SHIELD){
            thrustStr = "SHIELD";
        } else {
            thrustStr = to_string((int)thrust);
        }
        out << (int) direction.x << " " << (int) direction.y << " " << thrustStr;
        return out.str();
    }
};



#endif //CODERSSTRIKEBACKC_GAMESTATE_H
//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_INPUTPARSER_H
#define CODERSSTRIKEBACKC_INPUTPARSER_H



class InputParser {
    Race *race;
    int turn = 0;
    istream& stream;

public:
    InputParser(istream& stream) : stream(stream) {};
    void init();
    Race getRace() {return *race;}
    GameState parseGameState();
};

#endif //CODERSSTRIKEBACKC_INPUTPARSER_H
//
// Created by Kevin on 4/08/2016.
//

#ifndef CODERSSTRIKEBACKC_NAVIGATION_H
#define CODERSSTRIKEBACKC_NAVIGATION_H



class Navigation {
public:
    PodOutput seek(PodState pod, Vector target, int max_acc);
};


#endif //CODERSSTRIKEBACKC_NAVIGATION_H
//
// Created by Kevin on 1/08/2016.
//

#ifndef CODERSSTRIKEBACKC_PODRACERBOT_H_H
#define CODERSSTRIKEBACKC_PODRACERBOT_H_H


#include <iostream>
#include <string>
#include <algorithm>
#include <sstream>



using namespace std;


class PodraceBot {
public:
    virtual ~PodraceBot() {};
    virtual PodOutput move(GameState& gameState) {};
};

class MinimalBot : PodraceBot {
    int _racer;
    Navigation nav;
public:
    MinimalBot(int racer) : _racer(racer), nav() {};
    PodOutput move(GameState& gameState) {
        Checkpoint ck = gameState.race.checkpoints[gameState.ourState().pods[_racer].nextCheckpoint];
        PodOutput move = nav.seek(gameState.ourState().pods[_racer], ck.pos, 100);
        return move;//PodOutput(100, ck.pos);
    }
};




#endif //CODERSSTRIKEBACKC_PODRACERBOT_H_H
//
// Created by Kevin on 1/08/2016.
//





int main()
{
    // game loop
    InputParser inputParser(cin);
    inputParser.init();
    MinimalBot bot1(0);
    MinimalBot bot2(1);

    while (1) {
        GameState gs = inputParser.parseGameState();
        // To debug: cerr << "Debug messages..." << endl;
        PodOutput po1 = bot1.move(gs);
        PodOutput po2 = bot2.move(gs);
        cout << po1.toString() << endl << po2.toString() << endl;
    }
}//
// Created by Kevin on 1/08/2016.
//


//
// Created by Kevin on 1/08/2016.
//




#include <string>

using namespace std;

void InputParser::init() {
    int laps, checkpointCount;
    stream >> laps >> checkpointCount;
    vector<Checkpoint> checkpoints;
    for(int i = 0; i < checkpointCount; i++) {
        int x, y;
        stream >> x >> y;
        checkpoints.push_back(Checkpoint(x, y, i));
    }
    race = new Race(laps, checkpoints);
}

GameState InputParser::parseGameState() {
    vector<PlayerState> playerStates;
    for(int i = 0; i < Race::PLAYER_COUNT; i++) {
        vector<PodState> podStates;
        for(int p = 0; p < Race::POD_COUNT; p++) {
            int x, y, vx, vy, angle, nextCheckpoint;
            stream >> x >> y >> vx >> vy >> angle >> nextCheckpoint;
            PodState* pod = new PodState(x, y, vx, vy, angle, nextCheckpoint);
            podStates.push_back(*pod);
        }
        PlayerState* ps = new PlayerState(i, podStates);
        playerStates.push_back(*ps);
    }
    const int OUR_ID = 0; // Possible move this to race if it is indeed true.
    GameState gs = GameState(*race, playerStates, OUR_ID, turn++);
    return gs;
}//
// Created by Kevin on 4/08/2016.
//




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


