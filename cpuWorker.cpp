#include "CPUWorker.hpp"
#include "process.hpp"

// values on initialization
std::mutex CPUWorker::executionMutex;
std::condition_variable CPUWorker::turnCV;

int CPUWorker::turn = 0;
std::atomic<bool> CPUWorker::stopFlag{false};

CPUWorker::CPUWorker(int id, int cores)
    : id(id), CPU(cores) {}

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

void CPUWorker::assignProcess(std::shared_ptr<Process> p)
{
    process = p;
    isBusy = true; // Make sure to mark it busy here
}

void CPUWorker::assignedProcess()
{
    isBusy = true;
}

bool CPUWorker::busyStatus()
{
    return isBusy;
}

void CPUWorker::runWorker(int cpuTick, int delayPerExec,
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
            currentProcess->setArrivalTime();
            currentProcess->setStatus(RUNNING);

            while (currentProcess->getStatus() != FINISHED)
            {
                if (currentProcess->isSleeping())
                {
                    state = WorkerState::SLEEPING;
                    currentProcess->tickSleep();
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                    continue; // Wait out the sleep
                }

                state = WorkerState::RUNNING;
                currentProcess->execute();
                std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
            }

            // When process is done
            state = WorkerState::IDLE;
        }
        else
        {
            state = WorkerState::IDLE;
            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick)); // idle wait
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
                    state = WorkerState::SLEEPING;
                    currentProcess->tickSleep();
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));

                    // Push back to queue
                    std::lock_guard<std::mutex> lock(readyQueueMutex);
                    readyQueue.push(currentProcess);
                    break;
                }

                state = WorkerState::RUNNING;
                currentProcess->execute(); // one instruction
                std::this_thread::sleep_for(std::chrono::milliseconds(delayPerExec));
                executed++;
            }

            if (currentProcess->getStatus() != FINISHED && !currentProcess->isSleeping())
            {
                std::lock_guard<std::mutex> lock(readyQueueMutex);
                readyQueue.push(currentProcess);
            }

            state = WorkerState::IDLE;
        }
        else
        {
            state = WorkerState::IDLE;
            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
        }
    }
}
