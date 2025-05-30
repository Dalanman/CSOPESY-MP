#include "Colors.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "ConsoleManager.h"

using namespace std;

static ConsoleManager* consoleManager;

int main() {

    string input;
    bool running = true;
    consoleManager = new ConsoleManager();
    consoleManager->printHeader();  // init header

    while (running) {

        // Get the user input
        getline(cin, input);

        running = consoleManager->handleCommand(input);
    }

    return 0;
}