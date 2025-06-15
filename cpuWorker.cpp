#include "CPUWorker.hpp"
#include "process.hpp"


int CPUWorker::turn = 0;

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
void CPUWorker::runWorker() {
    while (true) {
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
