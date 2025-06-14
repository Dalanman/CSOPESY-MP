#include "scheduleManager.hpp"
#include "process.hpp"
#include "memory"
ScheduleManager::ScheduleManager(ScheduleType scheduleType, ProcessManager &pm, int cores)
    : type(scheduleType), manager(pm), cpuCores(cores) {}

void ScheduleManager::startSchedule()
{
    workers.clear();
    threads.clear();
    string name;
    int id, core, turns;
    for (int i = 0; i < 4; ++i)
    {
        workers.emplace_back(std::make_unique<CPUWorker>(i, manager.process[i]));
    }

    for (auto &worker : workers)
    {
        threads.emplace_back(&CPUWorker::runWorker, worker.get());
    }
}

void ScheduleManager::joinAll()
{
    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }
}
