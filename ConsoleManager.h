#pragma once
#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <queue>
#include "processManager.hpp"
#include "ConfigReader.hpp"
class ConsoleManager {
private:
    std::vector<std::shared_ptr<Process>> process;
    bool inSession = false;
    bool initialized = false;
    bool stopInput = false;
    ProcessManager pm;
    std::thread Scheduler;
    std::thread dummyMaker;
    std::thread InputHandler;
    ConfigReader* configReader;
    // SymbolTable symbolTable;
    std::shared_ptr<Process> activeProcess = nullptr;

    int numCpu;
    int quantumCycle;
    int BPF;
    int DelayPerExec;
    int MinIns;
    int MaxIns;
    int cpuTick = 0;
    int maxOverallMem;
    int memPerFrame;
    int minMemPerProcess;
    int maxMemPerProcess;

public:
    ConsoleManager();
    ~ConsoleManager();

    void printHeader();
    void run();
    void inputLoop();
    void initialize();
    void readConfig();
    bool isInSession();
    ConfigReader* getConfig() const { return configReader; }
    bool handleCommand(const std::string& command);
    

};

#endif // ConsoleManager