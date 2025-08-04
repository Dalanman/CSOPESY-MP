#include "CPUWorker.hpp"
#include "process.hpp"
#include "memory.hpp"

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
                          std::mutex &readyQueueMutex, std::shared_ptr<FlatMemoryAllocator> memoryAllocator)
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
                currentProcess->execute(memoryAllocator);

                if (delayPerExec > 0)
                {
                    state = WorkerState::DELAYED;
                    for (int i = 0; i < delayPerExec; ++i)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                    }
                    state = WorkerState::RUNNING;
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                }
                activeTick++;
                totalTick++;
            }

            // When process is done
            state = WorkerState::IDLE;
        }
        else
        {
            state = WorkerState::IDLE;
            idleTick++;
            totalTick++;
            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick)); // idle wait
        }
    }
}


// Pass memory manager here. Process only proceeds if memory can handle.
void CPUWorker::runRRWorker(int cpuTick, int quantumCycle, int delayPerExec,
    std::queue<Process*>& readyQueue,
    std::mutex& readyQueueMutex,
    std::shared_ptr<FlatMemoryAllocator> memoryAllocator)
{
    int quantumCounter = 0;

    while (!CPUWorker::stopFlag.load())
    {
        Process* currentProcess = nullptr;
        totalTick++;

        // Only lock long enough to pop from queue
        {
            std::lock_guard<std::mutex> lock(readyQueueMutex);
            if (!readyQueue.empty())
            {
                currentProcess = readyQueue.front();
                readyQueue.pop();
            }
        }

        if (!currentProcess)
        {
            state = WorkerState::IDLE;
            idleTick++;
            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
            continue;
        }

        int pid = currentProcess->getProcessId();
        size_t memSize = currentProcess->getMemoryRequirement();

        // Use demand paging if not yet allocated
        if (!memoryAllocator->hasAllocation(pid))
        {
            memoryAllocator->allocateDemandPaged(memSize, pid);
        }

        currentProcess->setCoreIndex(this->id);
        currentProcess->setStatus(RUNNING);
        state = WorkerState::RUNNING;

        int executed = 0;
        size_t totalPages = (memSize + memoryAllocator->getPageSize() - 1) / memoryAllocator->getPageSize();
        size_t pageIndex = 0;

        while (executed < quantumCycle && currentProcess->getStatus() != FINISHED)
        {
            if (currentProcess->isSleeping())
            {
                state = WorkerState::SLEEPING;
                currentProcess->tickSleep();
                idleTick++;
                std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));

                // Requeue the sleeping process (lock briefly)
                {
                    std::lock_guard<std::mutex> lock(readyQueueMutex);
                    readyQueue.push(currentProcess);
                }
                break;
            }

            // Simulate a page access before executing
            char* pagePtr = memoryAllocator->accessPage(pid, pageIndex);
            if (!pagePtr)
            {
                // Page could not be loaded (frame shortage), requeue
                {
                    std::lock_guard<std::mutex> lock(readyQueueMutex);
                    readyQueue.push(currentProcess);
                }
                break;
            }

            // Execute process command — may internally call IO or memory again
            currentProcess->execute(memoryAllocator);
            activeTick++;

            if (delayPerExec > 0)
            {
                state = WorkerState::DELAYED;
                for (int i = 0; i < delayPerExec; ++i)
                {
                    idleTick++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                }
                state = WorkerState::RUNNING;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
            }

            pageIndex = (pageIndex + 1) % totalPages;
            executed++;
        }

        quantumCounter++;

        if (currentProcess->getStatus() == FINISHED)
        {
            memoryAllocator->deallocate(pid);
        }
        else if (!currentProcess->isSleeping())
        {
            std::lock_guard<std::mutex> lock(readyQueueMutex);
            readyQueue.push(currentProcess);
        }

        state = WorkerState::IDLE;
    }
}

