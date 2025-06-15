#include "process.hpp"
#include "processManager.hpp"
#include <vector>
#include <string.h>
#include <memory>
#include <iostream>
#include "cpuWorker.hpp"
#include <unordered_set>

ProcessManager::ProcessManager(int numCores) : cores(numCores) {}; 

void ProcessManager::makeDummies(int num, int instructions, string text) {
    int processNum = num;
    int numLines = instructions;
    string output = text;
    std::string name;
    int assignedCore;
    for (int i = 0; i < processNum; i++){
        if (i < 10){
            name = "screen_0" + std::to_string(i);
        } else {
            name = "screen_" + std::to_string(i);
        }
        assignedCore = i % this->cores;
        auto proc = std::make_shared<Process>(name, i, assignedCore, numLines);
        for (int j = 0; j < numLines; j++){
            process[i]->addCommand(output);
        }
        addProcess(proc);
    }
}

void ProcessManager::UpdateProcessScreen() {
    std::cout << "----------------------------------" << std::endl;
    std::cout << "Running processes: " << std::endl;

    for (const auto& p : process) {
        if (p->getStatus() == 2) {
            std::cout << p->getProcessName() << "\t"
                      << p->getCreationTimestamp() << "\t"
                      << "Core: " << p->getCoreIndex() << "\t"
                      << p->getCommandIndex() << "/"
                      << p->getTotalCommands() << std::endl;
        }
    }

    std::cout << "----------------------------------" << std::endl;
    std::cout << "Finished processes: " << std::endl;

    for (const auto& p : process) {
        if (p->getStatus() == 3) {
            std::cout << p->getProcessName() << "\t"
                      << p->getArrivalTimestamp() << "\t"
                      << "Finished\t"
                      << p->getTotalCommands() << "/"
                      << p->getTotalCommands() << std::endl;
        }
    }
}

void ProcessManager::addProcess(std::shared_ptr<Process> p) {
    p->setCreationTime(std::chrono::system_clock::now());
    Status state = READY;
    p->setStatus(state);
    process.push_back(p);
}

bool ProcessManager::allProcessesDone() {
    for (const auto& p : process) {
        if (p->getStatus() != FINISHED && p->getStatus() != CANCELLED)
            return false;
    }
    return true;
}

void ProcessManager::executeFCFS() {
    workers.clear();
    threads.clear();

    std::unordered_set<int> assignedProcessIds;

    for (int i = 0; i < cores; ++i) {
        workers.emplace_back(std::make_unique<CPUWorker>(i, nullptr, cores));
    }

    for (auto& worker : workers) {
        threads.emplace_back(&CPUWorker::runWorker, worker.get());
    }

    while (!allProcessesDone()) {
        std::vector<std::shared_ptr<Process>> readyQueue;

        for (auto& p : process) {
            if (p->getStatus() == READY && !assignedProcessIds.count(p->getProcessId())) {
                readyQueue.push_back(p);
            }
        }

        std::sort(readyQueue.begin(), readyQueue.end(), [](auto& a, auto& b) {
            return a->getCreationTimestamp() < b->getCreationTimestamp();
        });

        for (auto& p : readyQueue) {
            for (auto& worker : workers) {
                if (!worker->hasProcess()) {
                    p->setStatus(RUNNING);
                    p->setCoreIndex(worker->getId());
                    worker->assignProcess(p);
                    assignedProcessIds.insert(p->getProcessId());
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
}

void ProcessManager::cancelAll() {
    
    CPUWorker::stopAllWorkers();

    
    for (auto& p : process) {
        if (p->getStatus() != FINISHED) {
            p->setStatus(CANCELLED);
        }
    }

    
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    std::cout << "All workers have been stopped and unfinished processes are CANCELLED.\n";
}