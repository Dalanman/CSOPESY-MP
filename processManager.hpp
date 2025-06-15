#pragma once
#include "process.hpp"
#include <vector>
#include <string.h>
#include <memory>
class ProcessManager {

    public:
        ProcessManager(int numCores) : cores(numCores) {};
        std::vector<std::shared_ptr<Process>> process;
        void makeDummies(int num, int instructions, string text);
        void addProcess();
        void UpdateProcessScreen();

    private:
        int cores;
};

