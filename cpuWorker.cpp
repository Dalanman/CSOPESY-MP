#include "CPUWorker.hpp"
#include "process.hpp"

//values on initialization
int CPUWorker::turn = 0; 
std::atomic<bool> CPUWorker::stopFlag{false};

CPUWorker::CPUWorker(int id, std::shared_ptr<Process> proc, int cores)
    : id(id), process(proc), CPU(cores) {}

void CPUWorker::assignProcess(std::shared_ptr<Process> p) {
    process = p;
}

bool CPUWorker::hasProcess() const {
    return process && process->getStatus() != FINISHED && process->getStatus() != CANCELLED;
}

int CPUWorker::getId() const {
    return id;
}

void CPUWorker::stopAllWorkers() {
    stopFlag.store(true);
    turnCV.notify_all();  // Wake all threads
}

void CPUWorker::stop() {
    stopFlag.store(true);
    turnCV.notify_all();
}

void CPUWorker::runWorker() {
    while (true) {

        if (stopFlag.load()) {
            if (process && process->getStatus() != FINISHED) {
                process->setStatus(CANCELLED);
            }
            break;
        }

        if (process && process->getStatus() == RUNNING) {
            std::unique_lock<std::mutex> lock(executionMutex);
            turnCV.wait(lock, [this] { return turn == id; });

            process->execute();
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));

            process = nullptr;
            process->setStatus(FINISHED);
            turn = (turn + 1) % CPU;
            turnCV.notify_all();
        }

    }
}
