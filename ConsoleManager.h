#pragma once
#ifndef CONSOLEMANAGER_H
#define CONSOLEMANAGER_H

#include <map>
#include <string>
#include <vector>
#include <queue>
#include "processManager.hpp"
#include "scheduleManager.hpp"
class ConsoleManager {
private:
    std::vector<std::shared_ptr<Process>> process;
    bool inSession = false;
    bool initialized = false;
    ProcessManager pm;
    ScheduleManager sm;
public:
    ConsoleManager();
    ~ConsoleManager();

    void printHeader();

    void initialize();
    void readConfig();
    bool isInSession();
    void listAllProcess();
    bool handleCommand(const std::string& command);
    

};

#endif // ConsoleManager