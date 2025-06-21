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
    std::thread InputHandler;
    ConfigReader* configReader;

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