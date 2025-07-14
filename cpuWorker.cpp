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
    int quantumCounter = 0;
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
            currentProcess->setStatus(RUNNING);
            state = WorkerState::RUNNING;
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
                if (delayPerExec > 0)
                {
                    state = WorkerState::DELAYED;
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                    if (delayPerExec == 1) {
                        for (int i = 0; i < delayPerExec; ++i)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                        }
                        state = WorkerState::RUNNING;
                    }
                    else {
                        for (int i = 0; i < delayPerExec - 1; ++i)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                        }
                        state = WorkerState::RUNNING;
                    }
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cpuTick));
                }
                executed++;
            }

            quantumCounter++;
            std::string filename = "memory_stamp_" + std::to_string(quantumCounter) + ".txt";
            std::ofstream outFile(filename);
            if (outFile.is_open())
            {
                outFile << "Timestamp: (" << currentProcess->getRunTimestamp() << ")" << std::endl;
                outFile << "Number of processes in memory: X" << std::endl;
                outFile << "Total external fragmentation in KB: X" << std::endl;
                outFile << "" << std::endl;
                outFile << "----end---- = X" << std::endl;
                outFile << "" << std::endl;
                outFile << "{PROCESS1 UPPER LIMIT}" << std::endl;
                outFile << "PX" << std::endl;
                outFile << "{PROCESS1 LOWER LIMIT}" << std::endl;
                outFile << "" << std::endl;
                outFile << "{PROCESS2 UPPER LIMIT}" << std::endl;
                outFile << "PX" << std::endl;
                outFile << "{PROCESS2 LOWER LIMIT}" << std::endl;
                outFile << "" << std::endl;
                outFile << "----start---- = X" << std::endl;
                outFile.close();
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
