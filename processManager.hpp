#pragma once
#include "process.hpp"
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <queue>
#include <cstddef>
#include "CPUWorker.hpp"
#include "memory.hpp"

class ProcessManager {
public:
    ProcessManager(int numCores) : cores(numCores) {};

    void makeDummies(int cpuTick, int minIns, int maxIns, int BPF, size_t maxMemPerProcess);
    void makeDummy(std::string name, int cpuTick, int minIns, int maxIns, int BPF);
    void addProcess(std::shared_ptr<Process> p);
    void UpdateProcessScreen();
    bool allProcessesDone();
    std::string toString(const std::chrono::time_point<std::chrono::system_clock>& timePoint);
    void executeFCFS(int numCpu, int cpuTick, int quantumCycle, int delayPerExec, std::shared_ptr<FlatMemoryAllocator> memoryAllocator);
    void cancelAll();
    void setCore(int numCpu) {
        cores = numCpu;
    }
    int getCores() const { return cores; };
    std::vector<std::shared_ptr<Process>> getAllProcesses() const { return process; }
    void addToReadyQueue(Process* p);
    void executeRR(int numCpu, int cpuTick, int quantumCycle, int delayPerExec, std::shared_ptr<FlatMemoryAllocator> memoryAllocator);
    void stopDummy(){
        dummyStop = true;
    }

    void ReportUtil();
    int getBusyCores();
    int getAvailableCores();
    void makeAlternatingDummy(std::string name, int cpuTick, int minIns, int maxIns, int BPF);
    void alternatingCase(int cpuTick, int minIns, int maxIns, int BPF);
	// void makeCustomDummy(std::string name, int cpuTick, int minIns, int maxIns, int BPF, size_t memSize, std::vector<std::string> instructions, size_t maxMemPerProcess);
    // Add this method declaration to the ProcessManager class in processManager.hpp

    void makeCustomDummy(std::string name, int cpuTick, int minIns, int maxIns, int BPF, size_t memSize, std::vector<std::string> commands, size_t maxMemPerProcess);
    
    void vmstat(std::shared_ptr<FlatMemoryAllocator> memoryAllocator, int maxMemory);
    void processSMI(std::shared_ptr<FlatMemoryAllocator> memoryAllocator, int maxMemory);

private:
    int cores;
	int pid_counter = 0;  
    std::vector<std::shared_ptr<Process>> process;  // Created processes
    std::vector<std::unique_ptr<CPUWorker>> workers;
    std::vector<std::thread> threads;
    bool dummyStop = false;
    // RR
    std::vector<std::thread> coreThreads; 
    std::queue<Process*> readyQueue;
    std::mutex readyQueueMutex;
};
