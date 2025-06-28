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

#include <cstdlib> // For srand and rand
#include <ctime>   // For time

void ProcessManager::makeDummies(int cpuTick, int minIns, int maxIns, int BPF)
{
    int numLines = 0;
    std::string name;
    int assignedCore = -1;
    int i = 0;
    int counterForBPF = 0;
    // Seed the random number generator
    srand(static_cast<unsigned int>(time(nullptr)));

    while(!dummyStop)
    {
        if (counterForBPF % BPF == 0) {
            
            // std::cout << BPF << "         " << counterForBPF << "      Created 1 process at " << i << std::endl;
            // Generate a random number of instructions within the range
            numLines = rand() % (maxIns - minIns + 1) + minIns;
            //std::cout << numLines << "   " << minIns << "       " << maxIns << std::endl;

            if (i < 10)
                name = "process0" + std::to_string(i);
            else
                name = "process" + std::to_string(i);

            auto proc = std::make_shared<Process>(name, i, assignedCore, numLines);
            addProcess(proc);

            for (int j = 0; j < numLines; j++)
            {
                std::string cmdStr;
                int type = rand() % 3; // 0: IO, 1: PRINT, 2: FOR

                switch (type)
                {
                case 0:
                    cmdStr = IOCommand::randomCommand();
                    break;
                case 1:
                    cmdStr = "PRINT(Hello World from " + name + ")";
                    break;
                case 2:
                    cmdStr = ForCommand::randomCommand(name);
                    break;
                }

                proc->addCommand(cmdStr);
            }

            proc->parse();
            addToReadyQueue(proc.get());
            i++;
        }

        counterForBPF++;
        std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
    }
}

void ProcessManager::UpdateProcessScreen()
{
    int busy = getBusyCores(); 
    int available = getAvailableCores();
    int utilization = (100 * busy) / cores;

    std::cout << "CPU utilization: " << utilization << std::endl;
    std::cout << "Cores Used: " << busy << std::endl;
    std::cout << "Cores Available: " << available << std::endl;
    std::cout << " " << std::endl;
    std::cout << " " << std::endl;
    std::cout << " " << std::endl;
    std::cout << " " << std::endl;

    std::cout << "----------------------------------" << std::endl;
    std::cout << "Running processes: " << std::endl;

    for (const auto &p : process)
    {
        if (p->getStatus() == 2 || p->getStatus() == 1 || p->getStatus() == 0)
        {
            std::cout << p->getProcessName() << "\t"
                      << p->getArrivalTimestamp() << "\t"
                      << "Core: " << p->getCoreIndex() << "\t"
                      << p->getCommandIndex() << "/"
                      << p->getNumCommands() << std::endl;
        }
    }

    std::cout << "----------------------------------" << std::endl;
    std::cout << "Finished processes: " << std::endl;

    for (const auto &p : process)
    {
        if (p->getStatus() == 3)
        {
            std::cout << p->getProcessName() << "\t"
                      << p->getArrivalTimestamp() << "\t"
                      << "Finished\t"
                      << p->getNumCommands() << "/"
                      << p->getNumCommands() << std::endl;
        }
    }
}

int ProcessManager::getBusyCores()
{
    int busy = 0;
    for (auto& worker : workers)
    {
        if (worker->busyStatus())
            busy++;
    }
    return busy;
}

int ProcessManager::getAvailableCores()
{
    return cores - getBusyCores();
}

std::string ProcessManager::toString(const std::chrono::time_point<std::chrono::system_clock> &timePoint)
{
    // Convert time_point to std::time_t
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);

    // Convert to tm struct for formatting
    std::tm *tm_time = std::localtime(&time);

    // Format the time into a string
    std::ostringstream oss;
    oss << std::put_time(tm_time, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

void ProcessManager::addProcess(std::shared_ptr<Process> p)
{
    p->setCreationTime(toString(std::chrono::system_clock::now()));
    Status state = READY;
    p->setStatus(state);
    process.push_back(p);
}

bool ProcessManager::allProcessesDone()
{
    for (const auto &p : process)
    {
        if (p->getStatus() != FINISHED && p->getStatus() != CANCELLED)
            return false;
    }
    return true;
}

void ProcessManager::executeFCFS(int numCpu, int cpuTick, int quantumCycle, int delayPerExec)
{
    workers.clear();
    threads.clear();
    std::unordered_set<int> assignedProcessIds;

    for (int i = 0; i < cores; ++i)
    {
        workers.emplace_back(std::make_unique<CPUWorker>(i, nullptr, cores));
    }

    for (auto &worker : workers)
    {
        threads.emplace_back(&CPUWorker::runWorker, worker.get(), cpuTick);
    }

    while (!allProcessesDone())
    {
        std::vector<std::shared_ptr<Process>> readyQueue;

        for (auto &p : process)
        {
            if (p->getStatus() == READY && !assignedProcessIds.count(p->getProcessId()))
            {
                readyQueue.push_back(p);
            }
        }

        std::sort(readyQueue.begin(), readyQueue.end(),
                  [](auto &a, auto &b)
                  {
                      return a->getCreationTimestamp() < b->getCreationTimestamp();
                  });

        for (auto &p : readyQueue)
        {
            for (auto &worker : workers)
            {
                if (!worker->hasProcess())
                {
                    worker->assignedProcess();
                    p->setStatus(RUNNING);
                    p->setCoreIndex(worker->getId());
                    p->setArrivalTime();
                    worker->assignProcess(p);
                    assignedProcessIds.insert(p->getProcessId());
                    break;
                }
            }
        }
    }

    CPUWorker::stopAllWorkers();
    for (auto &t : threads)
        if (t.joinable())
            t.join();

}

void ProcessManager::addToReadyQueue(Process *p)
{
    std::lock_guard<std::mutex> lock(readyQueueMutex);
    readyQueue.push(p);
}

void ProcessManager::executeRR(int numCpu, int cpuTick, int quantumCycle, int delayPerExec)
{
    workers.clear();
    threads.clear();

    // Create CPUWorkers (but don't assign specific processes)
    for (int i = 0; i < numCpu; ++i)
    {
        workers.emplace_back(std::make_unique<CPUWorker>(i, nullptr, numCpu));
    }

    // Start threads using runRRWorker with the shared readyQueue
    for (auto &worker : workers)
    {
        worker->assignedProcess();
        threads.emplace_back(
            &CPUWorker::runRRWorker,
            worker.get(),
            cpuTick,
            quantumCycle,
            delayPerExec,
            std::ref(readyQueue),
            std::ref(readyQueueMutex));
    }

    // Hindi na naca-call since at the start, wala pang processes
    while (!allProcessesDone())
    {
        UpdateProcessScreen();
        std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
    }

    // Stop all workers (if needed for now, they exit when processes finish)

    // Join all threads
    for (auto &t : threads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }

    std::cout << "All processes have been executed using Round Robin scheduling." << std::endl;
    std::cout << "Enter a command: ";
}

void ProcessManager::cancelAll()
{

    CPUWorker::stopAllWorkers();

    for (auto &p : process)
    {
        if (p->getStatus() != FINISHED)
        {
            p->setStatus(CANCELLED);
        }
    }

    for (auto &t : threads)
    {
        if (t.joinable())
            t.join();
    }

    std::cout << "All workers have been stopped and unfinished processes are CANCELLED.\n";
}