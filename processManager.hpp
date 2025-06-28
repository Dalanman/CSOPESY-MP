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

    void makeDummies(int cpuTick, int minIns, int maxIns, int BPF);
    void addProcess(std::shared_ptr<Process> p);
    void UpdateProcessScreen();
    bool allProcessesDone();
    std::string toString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
    void executeFCFS(int numCpu, int cpuTick, int quantumCycle, int delayPerExec);
    void cancelAll();
    int getCores() const { return cores; };
    std::vector<std::shared_ptr<Process>> getAllProcesses() const { return process; }
    void addToReadyQueue(Process* p);
    void executeRR(int numCpu, int cpuTick, int quantumCycle, int delayPerExec);
    void stopDummy(){
        dummyStop = true;
    }
    int getBusyCores();
    int getAvailableCores();
private:
    int cores;
    std::vector<std::shared_ptr<Process>> process;  // Created processes
    std::vector<std::unique_ptr<CPUWorker>> workers;
    std::vector<std::thread> threads;
    bool dummyStop = false;
    // RR
    std::vector<std::thread> coreThreads; 
    std::queue<Process*> readyQueue;
    std::mutex readyQueueMutex;
};
