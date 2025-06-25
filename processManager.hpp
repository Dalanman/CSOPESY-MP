#pragma once
#include "process.hpp"
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <queue>
#include "CPUWorker.hpp"

class ProcessManager {
public:
    ProcessManager(int numCores) : cores(numCores) {};

    void makeDummies(int num, int instructions, std::string text);
    void addProcess(std::shared_ptr<Process> p);
    void UpdateProcessScreen();
    bool allProcessesDone();
    std::string toString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
    void executeFCFS();
    void cancelAll();
    int getCores() const { return cores; };
    void ProcessManager::executeRR(int numCpu, int cpuTick, int quantumCycle, int delayPerExec);

private:
    int cores;
    std::vector<std::shared_ptr<Process>> process;
    std::vector<std::unique_ptr<CPUWorker>> workers;
    std::vector<std::thread> threads;

    std::vector<std::thread> coreThreads; 
    std::queue<Process*> readyQueue;
    std::mutex readyQueueMutex;
};
