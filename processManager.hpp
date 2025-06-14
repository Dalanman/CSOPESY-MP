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
    private:
    int cores;
};



void ProcessManager::makeDummies(int num, int instructions, string text) {
    int processNum = num;
    int numLines = instructions;
    string output = text;
    int assignedCore;
    for (int i = 0; i < processNum; i++){
        std::string name = "screen_0" + std::to_string(i);
        assignedCore = i % this->cores;
        process.emplace_back(std::make_shared<Process>(name, i, assignedCore, numLines));
    }
}

