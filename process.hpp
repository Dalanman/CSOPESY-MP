#pragma once
#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "commandList.hpp"
#include "command.hpp"
using namespace std;

enum Status
{
    WAITING,
    READY,
    RUNNING,
    FINISHED,
    CANCELLED
};

class Process
{
public:
    // Constructor
    Process(const std::string &name, int id, int assignedCore, int totalInstructions);

    // Public methods
    // void displayDetails() const;
    // void processSMI();
    // void getNextCommand();
    void execute();
    void setRunTimeStamp();
    void setCoreIndex(int core) { coreIndex = core; }
    void setStatus(Status newStatus) { status = newStatus; }
    void setArrivalTime();
    void addCommand(string text);
    // void setCreationTime(std::chrono::time_point<std::chrono::system_clock> creationTime) { creationTimeStamp = creationTime; }
    void setCreationTime(string creationTime) { creationTimeStamp = creationTime; }
    void parse();
    // Getters
    string getProcessName() const { return processName; }
    int getProcessId() const { return processId; }
    Status getStatus() const { return status; }
    bool getIsActive() const { return isActive; }
    size_t getTotalCommands() const { return commands.size(); }
    int getCommandIndex() const { return commandIndex; }
    int getCoreIndex() const { return coreIndex; }
    // std::chrono::time_point<std::chrono::system_clock> getCreationTimestamp() const { return creationTimeStamp; }
    string getCreationTimestamp() const { return creationTimeStamp; }
    string getRunTimestamp() const { return runTimeStamp; }
    string getArrivalTimestamp() const { return arrivalTimeStamp; }
    int getActualCommands() { return commandList.getSize(); }
    vector<string> getsmiLogs() { return smiLogs; }
    vector<string> getLogs() { return logs; }
    int getNumCommands() { return numCommands; }

    std::shared_ptr<Command> getCommand(int j) {
        return commandList.getCommand(j);
    };

    bool isSleeping() const
    {
        return sleepRemainingTicks > 0;
    }

    void tickSleep()
    {
        if (sleepRemainingTicks > 0)
        {
            sleepRemainingTicks--;
        }
        // Wake up automatically after last tick
        if (sleepRemainingTicks == 0 && getStatus() != FINISHED)
        {
            // Continue normally in next execute()
            // Optional: setStatus(RUNNING); // Only if needed
        }
    }

    int getSleepRemaining() const { return sleepRemainingTicks; }

private:
    Status status;
    string processName;
    int processId;
    vector<string> commands; // List of commands
    CommandList commandList;
    vector<string> logs;
    int numCommands;      // number of instructions
    int commandIndex = 0; // Current executed command
    // std::chrono::time_point<std::chrono::system_clock> creationTimeStamp;
    string creationTimeStamp;
    string arrivalTimeStamp;
    string runTimeStamp;
    bool isActive;
    int coreIndex; // index of core assigned to process
    int sleepRemainingTicks = 0;
    vector<string> smiLogs;
};

#endif // PROCESS_H