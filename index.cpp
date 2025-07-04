#include "Colors.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include "ConsoleManager.h"
#define _CRT_SECURE_NO_WARNINGS

using namespace std;

static ConsoleManager* consoleManager;

int main() {

    string input;
    bool running = true;
    consoleManager = new ConsoleManager();
    consoleManager->printHeader();  

    while (running) {
        getline(cin, input);

        running = consoleManager->handleCommand(input);
    }

    return 0;
}