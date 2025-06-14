#include "CPUWorker.hpp"
#include "process.hpp"

CPUWorker::CPUWorker(int id, std::shared_ptr<Process> proc)
    : id(id), process(proc) {}

void CPUWorker::runWorker()
{
    while (process->getStatus() != 3)
    {
        std::unique_lock<std::mutex> lock(executionMutex);
        turnCV.wait(lock, [this]
                    { return turn == id; });

        std::cout << "Core: " << id << " executing process " << process->getProcessId() << "...\n";
        process->execute();
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));

        turn = (turn + 1) % CPU;
        turnCV.notify_all();
    }
}
