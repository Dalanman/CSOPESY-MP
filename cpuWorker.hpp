#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <iostream>
#include "process.hpp"

class CPUWorker
{
public:
    CPUWorker(int id, std::shared_ptr<Process> proc, int cores);
    void runWorker();

private:
    int id;
    std::shared_ptr<Process> process;
    int CPU;
    inline static std::mutex executionMutex;
    inline static std::condition_variable turnCV;
    inline static int turn = 0;

    inline static const int DELAY = 500;
};
