#pragma once
#include "scheduleManager.hpp"
#include "CPUWorker.hpp"
#include "processManager.hpp"
#include <thread>
#include <memory>
#include <iostream>

ScheduleManager::ScheduleManager(ScheduleType scheduleType, ProcessManager& pm, int cores)
    : type(scheduleType), manager(pm), cpuCores(cores) {}

void ScheduleManager::startSchedule()
{
    workers.clear();
    threads.clear();

    if (type == ScheduleType::FCFS)
    {
        executeFCFS();
    }
    else if (type == ScheduleType::RR)
    {
        executeRR();
    }
}

void ScheduleManager::executeFCFS()
{
    int coreCount = std::min(cpuCores, static_cast<int>(manager.process.size()));

    for (int i = 0; i < coreCount; ++i)
    {
        auto proc = manager.process[i];
        workers.emplace_back(std::make_unique<CPUWorker>(i, proc, cpuCores));
    }

    for (auto& worker : workers)
    {
        threads.emplace_back(&CPUWorker::runWorker, worker.get());
    }
}

void ScheduleManager::executeRR()
{
    // Placeholder for round-robin scheduling logic.
    std::cout << "Round Robin scheduling not implemented yet.\n";
}

void ScheduleManager::setPM(ProcessManager& pm)
{
    manager = pm;
}

void ScheduleManager::joinAll()
{
    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }
}
