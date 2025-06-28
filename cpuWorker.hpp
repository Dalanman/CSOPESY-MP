#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>
#include <queue>
#include "process.hpp"



class CPUWorker
{
    
public:
    enum class WorkerState { IDLE, RUNNING, SLEEPING, DELAYED};
    CPUWorker(int id, int cores);

    void assignProcess(std::shared_ptr<Process> p);
    bool hasProcess() const;
    int getId() const;
    void runWorker(int cpuTick, int delayPerExec, std::queue<Process *> &readyQueue, std::mutex &readyQueueMutex);
    void runRRWorker(int cpuTick, int quantumCycle, int delayPerExec, std::queue<Process *> &readyQueue, std::mutex &readyQueueMutex);
    void stop();
    static void stopAllWorkers();
    bool busyStatus();
    void assignedProcess();
    WorkerState getState() const { return state.load(); }
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
    std::atomic<WorkerState> state = WorkerState::IDLE;
};