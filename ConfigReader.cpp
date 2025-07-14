#include "ConfigReader.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

ConfigReader::ConfigReader()
{
    numCpu = 0;
    schedulerType = SchedulerType::FCFS;
    quantum = 0;
    batchProcessFreq = 0;
    minIns = 0;
    maxIns = 0;
    delayPerExec = 0;
    maxOverallMem = 0;
    memPerFrame = 0;
    minMemPerProcess = 0;
    maxMemPerProcess = 0;
    readConfig();
}

ConfigReader::~ConfigReader()
{
}

void ConfigReader::readConfig() {
    std::ifstream file("config.txt");
    std::string line;

    if (!file.is_open()) {
        cout << "ERROR: Could not open 'config.txt'." << endl;
        return;
    }
    while (std::getline(file, line)) {
        if (line.find("num-cpu") == 0) {
            numCpu = stoi(line.substr(8));
        }
        else if (line.find("scheduler") == 0) {
            std::string typeString = line.substr(10);
            if (typeString == "'fcfs'") {
                schedulerType = SchedulerType::FCFS;
            }
            else if (typeString == "\"fcfs\"") {
                schedulerType = SchedulerType::FCFS;
            }
            else if (typeString == "\"rr\"") {
                schedulerType = SchedulerType::RR;
            }
            else if (typeString == "'rr'") {
                schedulerType = SchedulerType::RR;
            }
        }
        else if (line.find("quantum-cycles") == 0) {
            quantum = stoi(line.substr(15));
        }
        else if (line.find("batch-process-freq") == 0) {
            batchProcessFreq = stoi(line.substr(19));
        }
        else if (line.find("min-ins") == 0) {
            minIns = stoi(line.substr(8));
        }
        else if (line.find("max-ins") == 0) {
            maxIns = stoi(line.substr(8));
        }
        else if (line.find("delay-per-exec") == 0) {
            delayPerExec = stoi(line.substr(15));
        }
        else if (line.find("max-overall-mem") == 0) {
            maxOverallMem = stoi(line.substr(16));
        }
        else if (line.find("mem-per-frame") == 0) {
            memPerFrame = stoi(line.substr(14));
        }
        else if (line.find("min-mem-per-proc") == 0) {
            minMemPerProcess = stoi(line.substr(17));
        }
        else if (line.find("max-mem-per-proc") == 0) {
            maxMemPerProcess = stoi(line.substr(17));
        }
    }

    file.close();
    debug();
}

void ConfigReader::debug() {
    std::cout << "Number of CPUs: " << numCpu << std::endl;
    std::cout << "Scheduler type: ";
    switch (schedulerType) {
    case SchedulerType::FCFS:
        std::cout << "FCFS" << std::endl;
        break;
    case SchedulerType::RR:
        std::cout << "RR" << std::endl;
        break;
    default:
        std::cout << "Unknown" << std::endl;
        break;
    }
    std::cout << "Quantum cycles: " << quantum << std::endl;
    std::cout << "Batch process frequency: " << batchProcessFreq << std::endl;
    std::cout << "Minimum instructions: " << minIns << std::endl;
    std::cout << "Maximum instructions: " << maxIns << std::endl;
    std::cout << "Delay per execution: " << delayPerExec << std::endl;
    std::cout << "Max overall memory: " << maxOverallMem << std::endl;
    std::cout << "Memory per frame: " << memPerFrame << std::endl;
    std::cout << "Minimum memory per process: " << minMemPerProcess << std::endl;
    std::cout << "Maximum memory per process: " << maxMemPerProcess << std::endl;
}