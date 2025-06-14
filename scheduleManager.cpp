#include "scheduleManager.hpp"

ScheduleManager::ScheduleManager(ScheduleType scheduleType, ProcessManager& pm, int cores)
    : type(scheduleType), manager(pm), cpuCores(cores) {}

void ScheduleManager::startSchedule() {
    workers.clear();
    threads.clear();
    Process pr;
    for (int i = 0; i < 4; ++i) {
        pr = manager.process[i];
        workers.emplace_back(std::make_unique<CPUWorker>(i, pr));
    }

    for (auto& worker : workers) {
        threads.emplace_back(&CPUWorker::runWorker, worker.get());
    }
}

void ScheduleManager::joinAll() {
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}
