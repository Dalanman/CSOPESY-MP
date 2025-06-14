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
    bool handleCommand(const std::string& command);
    

};

#endif // ConsoleManager