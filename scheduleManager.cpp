#include "scheduleManager.hpp"

void scheduleManager::initScheduler(ScheduleType type, int nTurns) {
    workers.clear();
    threads.clear();
    currentSchedule = type;
    for (int i = 0; i < numCores; ++i) {
        workers.emplace_back(std::make_unique<CPUWorker>((int)type, i, nTurns));
    }
}

void scheduleManager::startScheduler() {
    for (auto& worker : workers) {
        threads.emplace_back(&CPUWorker::runWorker, worker.get());
    }
}

void scheduleManager::joinAll() {
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

void scheduleManager::reset() {
    workers.clear();
    threads.clear();
}
