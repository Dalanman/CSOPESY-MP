#pragma once
#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <queue>
#include "processManager.hpp"
class ConsoleManager {
private:
    std::vector<std::shared_ptr<Process>> process;
    bool inSession = false;
    bool initialized = false;
    bool stopInput = false;
    bool stopTick = false;      // for CPU tick implementation, used for Scheduler thread
    ProcessManager pm;
    std::thread Scheduler;
    std::thread InputHandler;

public:
    ConsoleManager();
    ~ConsoleManager();

    void printHeader();
    void run();
    void inputLoop();
    void initialize();
    void readConfig();
    bool isInSession();
    //void listAllProcess();
    bool handleCommand(const std::string& command);
    void ConsoleManager::startRR(int quantumCycle);
    

};

#endif // ConsoleManager