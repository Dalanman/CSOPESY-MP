#define _CRT_SECURE_NO_WARNINGS

#include "process.hpp"
#include "processManager.hpp"
#include <vector>
#include <string.h>
#include <memory>
#include <iostream>
#include "cpuWorker.hpp"
#include <unordered_set>
#include <algorithm>  
#include <queue>


// ProcessManager::ProcessManager(int numCores) : cores(numCores) {}; 

void ProcessManager::makeDummies(int num, int instructions, string text) {
    int processNum = num;
    int numLines = instructions;
    string output = text;
    std::string name;
    int assignedCore;
    for (int i = 0; i < processNum; i++){
		//cout << "Creating dummy process... " << i << std::endl;
        if (i < 10){
            name = "screen_0" + std::to_string(i);
        } else {
            name = "screen_" + std::to_string(i);
        }
        assignedCore = i % this->cores;
        auto proc = std::make_shared<Process>(name, i, assignedCore, numLines);
        addProcess(proc);
        for (int j = 0; j < numLines; j++){
            //cout << "Adding line " << j << " to " << name << "..." << std::endl;
            process[i]->addCommand(output);
        }
    }
}

void ProcessManager::UpdateProcessScreen() {


    std::cout << "----------------------------------" << std::endl;
    std::cout << "Running processes: " << std::endl;

    for (const auto& p : process) {
        if (p->getStatus() == 2 || p->getStatus() == 1 || p->getStatus() == 0) {
            std::cout << p->getProcessName() << "\t"
                      << p->getArrivalTimestamp() << "\t"
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

std::string ProcessManager::toString(const std::chrono::time_point<std::chrono::system_clock>& timePoint) {
    // Convert time_point to std::time_t
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);

    // Convert to tm struct for formatting
    std::tm* tm_time = std::localtime(&time);

    // Format the time into a string
    std::ostringstream oss;
    oss << std::put_time(tm_time, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

void ProcessManager::addProcess(std::shared_ptr<Process> p) {
    p->setCreationTime(toString(std::chrono::system_clock::now()));
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

	//cout << "Executing processes using FCFS scheduling..." << std::endl;
    std::unordered_set<int> assignedProcessIds;

    for (int i = 0; i < cores; ++i) {
        workers.emplace_back(std::make_unique<CPUWorker>(i, nullptr, cores));
    }

    for (auto& worker : workers) {
        threads.emplace_back(&CPUWorker::runWorker, worker.get());
    }

    while (!allProcessesDone()) {
		//cout << "Checking for processes to assign..." << std::endl;
        std::vector<std::shared_ptr<Process>> readyQueue;

        for (auto& p : process) {
			//cout << "Checking process: " << p->getProcessName() << std::endl;
            if (p->getStatus() == READY && !assignedProcessIds.count(p->getProcessId())) {
                readyQueue.push_back(p);
            }
        }

        std::sort(readyQueue.begin(), readyQueue.end(), [](auto& a, auto& b) {
            return a->getCreationTimestamp() < b->getCreationTimestamp();
        });

        for (auto& p : readyQueue) {
			//cout << "Assigning process: " << p->getProcessName() << std::endl;
            for (auto& worker : workers) {
                if (!worker->hasProcess()) {
                    p->setStatus(RUNNING);
                    p->setCoreIndex(worker->getId());
                    p->setArrivalTime();
                    worker->assignProcess(p);
                    assignedProcessIds.insert(p->getProcessId());
                    break;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    workers[0]->stopAllWorkers();

    //std::cout << "All processes done " << allProcessesDone() << std::endl;

    threads[0].join();
    threads[1].join();
    threads[2].join();
    threads[3].join();
    //cout << "Thread joined." << std::endl;
    /*
    for (auto& t : threads) {
		cout << "Joining thread..." << std::endl;
		cout << t.joinable() << std::endl;
        if (t.joinable()) {
            t.join();
			cout << "Thread joined." << std::endl;
        }
    }
    */

	cout << "All processes have been executed using FCFS scheduling." << std::endl;
    cout << "Enter a command: ";
    return;
}

void ProcessManager::executeRoundRobin(int cpuTick, bool& stopFlag, int quantum) {
    std::queue<std::shared_ptr<Process>> readyQueue;
    std::mutex queueMutex;

    // Initialize ready queue with all READY processes
    for (auto& p : process) {
        if (p->getStatus() == READY) {
            readyQueue.push(p);
        }
    }

    // Launch one thread per core
    std::vector<std::thread> coreThreads;
    for (int i = 0; i < cores; ++i) {
        coreThreads.emplace_back([&, i]() {
            while (!stopFlag) {
                std::shared_ptr<Process> p = nullptr;

                // Get next process from ready queue
                {
                    std::lock_guard<std::mutex> lock(queueMutex);
                    if (!readyQueue.empty()) {
                        p = readyQueue.front();
                        readyQueue.pop();
                    }
                }

                // If a process was available, execute it
                if (p) {
                    p->setStatus(RUNNING);
                    p->setArrivalTime();

                    int executedInstructions = 0;

                    // Execute up to 'quantum' instructions
                    while (executedInstructions < quantum && p->getStatus() == RUNNING) {
                        p->executeStep(); 
                        executedInstructions++;

                        if (p->getStatus() == FINISHED) break;

                        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // simulate time delay per instruction
                    }

                    if (p->getStatus() != FINISHED) {
                        p->setStatus(READY);
                        std::lock_guard<std::mutex> lock(queueMutex);
                        readyQueue.push(p);
                    }
                }
                else {
                    // No available process; idle time
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            });
    }

    // Update UI 
    while (!allProcessesDone() && !stopFlag) {
        UpdateProcessScreen();
        std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick)); 
    }

    stopFlag = true;

    for (auto& t : coreThreads) {
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