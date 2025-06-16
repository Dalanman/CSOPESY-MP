#include "CPUWorker.hpp"
#include "process.hpp"

//values on initialization
std::mutex CPUWorker::executionMutex;
std::condition_variable CPUWorker::turnCV;

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
    while (!CPUWorker::stopFlag) {

        if (stopFlag) {
            break;
        }

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
			cout << "CPU Worker " << id << " is executing process " << endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));

            process->setStatus(FINISHED);
            process = nullptr;
            turn = (turn + 1) % CPU;
            turnCV.notify_all();

        }

    }
}
