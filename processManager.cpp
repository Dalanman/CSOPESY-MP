#include "process.hpp"
#include "processManager.hpp"
#include <vector>
#include <string.h>
#include <memory>
#include <iostream>

ProcessManager::ProcessManager(int numCores) : cores(numCores) {}; 

void ProcessManager::makeDummies(int num, int instructions, string text) {
    int processNum = num;
    int numLines = instructions;
    string output = text;
    std::string name;
    int assignedCore;
    for (int i = 0; i < processNum; i++){
        if (i < 10){
            name = "screen_0" + std::to_string(i);
        } else {
            name = "screen_" + std::to_string(i);
        }
        assignedCore = i % this->cores;
        process.emplace_back(std::make_shared<Process>(name, i, assignedCore, numLines));
        for (int j = 0; j < numLines; j++){
            process[i]->addCommand(output);
        }
    }
}

void ProcessManager::UpdateProcessScreen() {
    std::cout << "----------------------------------" << std::endl;
    std::cout << "Running processes: " << std::endl;

    for (const auto& p : process) {
        if (p->getStatus() == 2) {
            std::cout << p->getProcessName() << "\t"
                      << p->getCreationTimestamp() << "\t"
                      << "Core: " << p->getCoreIndex() << "\t"
                      << p->getCommandIndex() << "/"
                      << p->getTotalCommands() << std::endl;
        }
    }

    std::cout << "----------------------------------" << std::endl;
    std::cout << "Finished processes: " << std::endl;

    for (const auto& p : process) {
        if (p->getStatus() == 3) {
            std::cout << p->getProcessName() << "\t"
                      << p->getCreationTimestamp() << "\t"
                      << "Finished\t"
                      << p->getTotalCommands() << "/"
                      << p->getTotalCommands() << std::endl;
        }
    }
}

void addProcess(){
    
}