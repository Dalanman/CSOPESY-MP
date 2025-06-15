#include "CPUWorker.hpp"
#include "process.hpp"

std::mutex CPUWorker::executionMutex;
std::condition_variable CPUWorker::turnCV;
int CPUWorker::turn;

CPUWorker::CPUWorker(int id, std::shared_ptr<Process> proc, int cores)
    : id(id), process(proc), CPU(cores) {}

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
