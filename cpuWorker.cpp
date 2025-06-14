#include "CPUWorker.hpp"
#include "process.hpp"

CPUWorker::CPUWorker(int id, std::shared_ptr<Process> proc)
    : id(id), process(proc) {}

void CPUWorker::runWorker()
{
    while (!process->isDone())
    {
        std::unique_lock<std::mutex> lock(executionMutex);
        turnCV.wait(lock, [this]
                    { return turn == id; });

        std::cout << "Core: " << id << " executing process " << process->getID() << "...\n";
        process->print();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));

        turn = (turn + 1) % CPU;
        turnCV.notify_all();
    }
}
