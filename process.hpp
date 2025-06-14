#pragma once
#ifndef PROCESS_H
#define PROCESS_H

#include <string>
#include <vector>

using namespace std;

class Process
{
public:
    // Constructor
    Process(const std::string& name, int id, int assignedCore, int totalInstructions);

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
    void setTimestamp();
    void setCoreIndex(int core) { coreIndex = core; }
    void setStatus(Status newStatus) { status = newStatus; }

    // Getters
    string getProcessName() const { return processName; }
    int getProcessId() const { return processId; }
    Status getStatus() const { return status; }
    bool getIsActive() const { return isActive; }
    size_t getTotalCommands() const { return commands.size(); }
    int getCommandIndex() const { return commandIndex; }
    int getCoreIndex() const { return coreIndex; }
    string getCreationTimestamp() const { return creationTimestamp; }
    string getRunTimestamp() const { return runTimestamp; }

private:
    Status status;
    string processName;
    int processId;
    vector<string> commands; // List of commands
    int commandIndex;        // Current executed command
    string creationTimestamp;
    string runTimestamp;
    bool isActive;
    int coreIndex; // index of core assigned to process
};

#endif // PROCESS_H