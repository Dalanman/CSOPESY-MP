#include "CPUWorker.hpp"
#include "process.hpp"

// values on initialization
std::mutex CPUWorker::executionMutex;
std::condition_variable CPUWorker::turnCV;

int CPUWorker::turn = 0;
std::atomic<bool> CPUWorker::stopFlag{false};

CPUWorker::CPUWorker(int id, std::shared_ptr<Process> proc, int cores)
    : id(id), process(proc), CPU(cores) {}

void CPUWorker::assignProcess(std::shared_ptr<Process> p)
{
    process = p;
}

bool CPUWorker::hasProcess() const
{
    return process && process->getStatus() != FINISHED && process->getStatus() != CANCELLED;
}

int CPUWorker::getId() const
{
    return id;
}

void CPUWorker::stopAllWorkers()
{
    stopFlag.store(true);
    turnCV.notify_all(); // Wake all threads
}

void CPUWorker::stop()
{
    stopFlag.store(true);
    turnCV.notify_all();
}

void CPUWorker::runWorker(int cpuTick) {
    while (!CPUWorker::stopFlag) {
        if (stopFlag.load()) {
            if (process && process->getStatus() != FINISHED) {
                process->setStatus(CANCELLED);
            }
            break;
        }

        if (process) {
            if (process->isSleeping()) {
                process->tickSleep();

                if (!process->isSleeping()) {
                    process->setStatus(READY);
                    process = nullptr;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                continue;
            }

            if (process->getStatus() == RUNNING) {
                std::unique_lock<std::mutex> lock(executionMutex);
                turnCV.wait(lock, [this] { return turn == id; });

                process->execute();

                if (process->isSleeping()) {
                    process->tickSleep(); // Set up sleep and start ticking
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));

                if (process->getStatus() == FINISHED) {
                    process = nullptr;
                }

                turn = (turn + 1) % CPU;
                turnCV.notify_all();
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
        }
    }
}


void CPUWorker::runRRWorker(int cpuTick, int quantumCycle, int delayPerExec,
                            std::queue<Process *> &readyQueue,
                            std::mutex &readyQueueMutex)
{
    while (!CPUWorker::stopFlag.load())
    {
        Process *currentProcess = nullptr;

        {
            std::lock_guard<std::mutex> lock(readyQueueMutex);
            if (!readyQueue.empty())
            {
                currentProcess = readyQueue.front();
                readyQueue.pop();
            }
        }

        if (currentProcess)
        {
            currentProcess->setCoreIndex(this->id);
            int executed = 0;

            while (executed < quantumCycle && currentProcess->getStatus() != FINISHED)
            {
                if (currentProcess->isSleeping())
                {
                    currentProcess->tickSleep();
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));

                    // RR pushes back if sleeping
                    std::lock_guard<std::mutex> lock(readyQueueMutex);
                    readyQueue.push(currentProcess);
                    break; // leave quantum loop
                }

                currentProcess->execute(); // one instruction
                std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                executed++;
            }

            if (currentProcess->getStatus() != FINISHED && !currentProcess->isSleeping())
            {
                std::lock_guard<std::mutex> lock(readyQueueMutex);
                readyQueue.push(currentProcess);
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick)); // idle wait
        }
    }
}
