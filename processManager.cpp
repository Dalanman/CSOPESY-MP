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

// ProcessManager::ProcessManager(int numCores) : cores(numCores) {}; 

void ProcessManager::makeDummies(int num, int minIns, int maxIns)
{
    int processNum = num;
    int numLines = rand() % (maxIns - minIns + 1) + minIns;
    std::string name;
    int assignedCore;

    std::cout << "Test 1" << std::endl;
    for (int i = 0; i < processNum; i++)
    {
        if (i < 10)
            name = "process0" + std::to_string(i);
        else
            name = "process" + std::to_string(i);
        auto proc = std::make_shared<Process>(name, i, assignedCore, numLines);
        addProcess(proc);

        std::cout << "Test 2" << std::endl;
        for (int j = 0; j < numLines; j++)
        {
            std::cout << "Test 3" << std::endl;
            std::string cmdStr;
            int type = rand() % 3; // 0: IO, 1: PRINT, 2: FOR

            switch (type)
            {
            case 0:
                cmdStr = IOCommand::randomCommand();
                break;
            case 1:
                cmdStr = "HELLO WORLD FROM " + name; // TODO: MAKE IT DEFAULT TO PRINTING "HELLO WORLD FROM <processName>!"
                break;
            case 2:
                cmdStr = ForCommand::randomCommand();
                break;
            }

            proc->addCommand(cmdStr);
        }
        std::cout << "Test 4" << std::endl;
        addToReadyQueue(proc.get());
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

void ProcessManager::addToReadyQueue(Process* p) {
    std::lock_guard<std::mutex> lock(readyQueueMutex);
    readyQueue.push(p);
}

void ProcessManager::executeRR(int numCpu, int cpuTick, int quantumCycle, int delayPerExec) {
    // Resize based on cpu count
    coreThreads.resize(numCpu);

    for (int i = 0; i < numCpu; i++) {
        std::cout << "Process creation " << i << std::endl; // For debugging ; creates i amount of threads based on numCpu
        coreThreads[i] = std::thread([&, i]() {
            while (!allProcessesDone()) {
                Process* currentProcess = nullptr;

                
                // Get process from queue
                {
                    std::lock_guard<std::mutex> lock(readyQueueMutex);
                    if (!readyQueue.empty()) {
                        currentProcess = readyQueue.front();
                        readyQueue.pop();
                    }
                }

                if (currentProcess) {
                    int executed = 0; // Counter 
                    std::cout << "Running process in if statement" << std::endl; // For debugging
                    // Execute up to quantum instructions or until finished
                    while (executed < quantumCycle && currentProcess->getStatus() != FINISHED) {
                        currentProcess->execute(); // Calls execute here || one instruction only
                        std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                        executed++;
                    }

                    // Requeue
                    if (currentProcess->getStatus() != FINISHED) {
                        std::cout << "Running process in requeue" << std::endl; // For debugging
                        std::lock_guard<std::mutex> lock(readyQueueMutex);
                        readyQueue.push(currentProcess);
                    }
                }
                else {
                    // Idle time if no processes in queue
                    std::cout << "Running process in idle" << std::endl; // For debugging
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                }
            }
            });
    }

    // UI updater 
    while (!allProcessesDone()) {
        UpdateProcessScreen();
        std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
    }

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