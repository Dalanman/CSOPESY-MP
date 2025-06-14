#pragma once

#include <thread>
#include <mutex>
#include <semaphore>
#include <condition_variable>
#include <process.hpp>
#include <processManager.hpp>
#include <string>
#include <memory>
#include <atomic>
#include <iostream>

class CPUWorker
{
public:
    inline static const int CPU = 4;

    CPUWorker(int type, int i, int numTurns, ProcessManager &pm);
    void runWorker();
    void runFCFS();
    void runRR(); // Not implemented yet

private:
    int id;
    int nTurns;
    int runType; // 0 = FCFS, 1 = RR
    ProcessManager &manager;

    // Static synchronization members
    inline static std::mutex turnMutex;
    inline static const int maxAccess = 1; // Or 4 for all CPUs
    inline static std::condition_variable turnCV;
    inline static std::counting_semaphore<CPU> semaphore{maxAccess};
    inline static std::atomic<int> sharedCounter = 1;
    inline static std::atomic<bool> running = false;
    inline static int turn = 0;
    inline static const int CPU = 4;
    inline static const int DELAY = 100;
};
