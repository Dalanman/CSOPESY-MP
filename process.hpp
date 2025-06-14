#pragma once
#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std;

class Process
{
public:
    // Constructor
    Process(const std::string &name, int id, int assignedCore, int totalInstructions);

    enum Status
    {
        WAITING,
        READY,
        RUNNING,
        FINISHED
    };

    // Public methods
    void displayDetails() const;
    void processSMI();
    void getNextCommand();
    void execute();
    void setRunTimeStamp();
    void setCoreIndex(int core) { coreIndex = core; }
    void setStatus(Status newStatus) { status = newStatus; }
    void setArrivalTime();
    void addCommand(string text);
    // Getters
    string getProcessName() const { return processName; }
    int getProcessId() const { return processId; }
    Status getStatus() const { return status; }
    bool getIsActive() const { return isActive; }
    size_t getTotalCommands() const { return commands.size(); }
    int getCommandIndex() const { return commandIndex; }
    int getCoreIndex() const { return coreIndex; }
    string getCreationTimestamp() const { return creationTimeStamp; }
    string getRunTimestamp() const { return runTimeStamp; }

private:
    Status status;
    string processName;
    int processId;
    vector<string> commands; // List of commands
    int numCommands; //number of instructions
    int commandIndex;        // Current executed command
    string creationTimeStamp;
    string runTimeStamp;
    bool isActive;
    int coreIndex; // index of core assigned to process
};

#endif // PROCESS_H