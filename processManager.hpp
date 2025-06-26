#pragma once
#include "process.hpp"
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include "CPUWorker.hpp"

class ProcessManager {
public:
    ProcessManager(int numCores) : cores(numCores) {};

    void makeDummies(int num, int minIns, int maxIns);
    void addProcess(std::shared_ptr<Process> p);
    void UpdateProcessScreen();
    bool allProcessesDone();
    std::string toString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
    void executeFCFS();
    void cancelAll();
    int getCores() const { return cores; };
    std::vector<std::shared_ptr<Process>> getAllProcesses() const { return process; } //added

private:
    int cores;
    std::vector<std::shared_ptr<Process>> process;
    std::vector<std::unique_ptr<CPUWorker>> workers;
    std::vector<std::thread> threads;
};
