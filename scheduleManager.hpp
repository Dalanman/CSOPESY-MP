#pragma once

#include <vector>
#include <thread>
#include <memory>
#include "CPUWorker.hpp"
#include "processManager.hpp"

enum class ScheduleType {
    FCFS,
    RR
};

class ScheduleManager {
public:
    ScheduleManager(ScheduleType scheduleType, ProcessManager& pm, int cores);
    void startSchedule();
    void joinAll();
    void setPM(ProcessManager& manager);
    void executeFCFS();
    void executeRR();
private:
    ScheduleType type;
    ProcessManager& manager;
    int cpuCores;
    std::vector<std::unique_ptr<CPUWorker>> workers;
    std::vector<std::thread> threads;
};
