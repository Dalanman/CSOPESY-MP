#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>
#include <queue>
#include "process.hpp"

class CPUWorker {
public:
    CPUWorker(int id, std::shared_ptr<Process> proc, int cores);

    void assignProcess(std::shared_ptr<Process> p);
    bool hasProcess() const;
    int getId() const;
    void runWorker(int cpuTick);
    void runRRWorker(int cpuTick, int quantumCycle, int delayPerExec, std::queue<Process*>& readyQueue, std::mutex& readyQueueMutex);
    void stop();
    static void stopAllWorkers();
    void assignedProcess();
    bool busyStatus();


    static std::mutex executionMutex;
    static std::condition_variable turnCV;
    static int turn;

private:
    int id;
    int CPU;
    std::shared_ptr<Process> process;
    int DELAY = 500;
    static std::atomic<bool> stopFlag;
    std::atomic<bool> isBusy = false;
};