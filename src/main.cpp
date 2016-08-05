//
// Created by Kevin on 1/08/2016.
//

#include "InputParser.h"
#include "podracerBot.h"
#include "State.h"

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
}