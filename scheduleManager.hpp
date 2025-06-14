#pragma once
#include <vector>
#include <thread>
#include "CPUWorker.hpp"

namespace scheduleManager {
    enum class ScheduleType { FCFS, RR };

    // Scheduler Configuration
    inline ScheduleType currentSchedule = ScheduleType::FCFS;
    inline int numCores = 4;
    inline int turnsPerCore = 1000;

    // CPUWorkers and Threads
    inline std::vector<std::unique_ptr<CPUWorker>> workers;
    inline std::vector<std::thread> threads;

    // Function declarations
    void initScheduler(ScheduleType type, int nTurns);
    void startScheduler();
    void joinAll();
    void reset();
}
