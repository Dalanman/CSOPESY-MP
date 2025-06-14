#include "CPUWorker.hpp"

CPUWorker::CPUWorker(int type, int i, int numTurns, ProcessManager& pm)
    : runType(type), id(i), nTurns(numTurns), manager(pm) {}

void CPUWorker::runWorker() {
    running = true;

    if (runType == 0) {
        runFCFS();
    } else if (runType == 1) {
        runRR(); // Not implemented yet
    }
}

void CPUWorker::runFCFS() {
    int localCounter = 1;

    while (localCounter <= nTurns) {
        std::unique_lock<std::mutex> lock(turnMutex);
        turnCV.wait(lock, [this] { return turn == id; });
        
        std::cout << "CPUWorker " << id << ": " << localCounter << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));

        localCounter++;
        turn = (turn % CPU) + 1;
        turnCV.notify_all();
    }
}


void CPUWorker::runRR() {
    // Round Robin not implemented yet
}
