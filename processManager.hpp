#pragma once
#include "process.hpp"
#include <vector>
#include <string>
#include <memory>

class ProcessManager {
public:
    ProcessManager(int numCores) : cores(numCores) {};

    void makeDummies(int num, int instructions, std::string text);
    void addProcess(std::shared_ptr<Process> p);
    void UpdateProcessScreen();
    bool allProcessesDone();

    void executeFCFS();
    void cancelAll();

private:
    int cores;
    std::vector<std::shared_ptr<Process>> process;
    std::vector<std::unique_ptr<CPUWorker>> workers;
    std::vector<std::thread> threads;
};
