#include "process.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
Process::Process(const std::string &name, int id, int assignedCore, int totalInstructions)
{
    this->processName = name;
    this->processId = id;
    this->coreIndex = assignedCore;
    this->numCommands = totalInstructions;
}

void Process::addCommand(string text){
    commands.emplace_back(text);
}

void Process::setArrivalTime()
{
    // Getting the arrival time
    std::chrono::time_point<std::chrono::system_clock> TimeStamp = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(TimeStamp);
    std::tm localTime;

#ifdef _WIN32
    localtime_s(&localTime, &timeNow); // Windows-safe
#else
    localtime_r(&timeNow, &localTime); // Unix-safe
#endif

    // Format: MM/DD/YYYY HH:MM:SSAM
    std::ostringstream oss;
    int hour = localTime.tm_hour;
    std::string ampm = (hour >= 12) ? "PM" : "AM";
    if (hour == 0)
        hour = 12;
    else if (hour > 12)
        hour -= 12;

    oss << std::setfill('0') << std::setw(2) << (localTime.tm_mon + 1) << "/"
        << std::setfill('0') << std::setw(2) << localTime.tm_mday << "/"
        << (1900 + localTime.tm_year) << " "
        << std::setfill('0') << std::setw(2) << hour << ":"
        << std::setfill('0') << std::setw(2) << localTime.tm_min << ":"
        << std::setfill('0') << std::setw(2) << localTime.tm_sec
        << ampm;

    arrivalTimeStamp = oss.str();
}

void Process::setRunTimeStamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;

#ifdef _WIN32
    localtime_s(&localTime, &timeNow);
#else
    localtime_r(&timeNow, &localTime);
#endif

    std::ostringstream oss;
    int hour = localTime.tm_hour;
    std::string ampm = (hour >= 12) ? "PM" : "AM";
    if (hour == 0)
        hour = 12;
    else if (hour > 12)
        hour -= 12;

    oss << std::setfill('0') << std::setw(2) << (localTime.tm_mon + 1) << "/"
        << std::setfill('0') << std::setw(2) << localTime.tm_mday << "/"
        << (1900 + localTime.tm_year) << " "
        << std::setfill('0') << std::setw(2) << hour << ":"
        << std::setfill('0') << std::setw(2) << localTime.tm_min << ":"
        << std::setfill('0') << std::setw(2) << localTime.tm_sec
        << ampm;

    runTimeStamp = oss.str();
}
void Process::execute()
{
	commandIndex = 0;
    string filename = processName + ".txt";
    // cout << "Writing to file: " << filename << endl;
    ofstream outFile(filename, std::ios::app);
        
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing logs: " << filename << std::endl;
        return;
    }

    outFile << "Process name: " << processName << "\n" << "Logs:\n\n";

    while (commandIndex < 100) {
        //cout << "EXEC: " << processName << endl;
        if (status == READY) {
            setArrivalTime();
            Status state = RUNNING;
            setStatus(state);
        }

        //cout << commandIndex << " / " << commands.size() << endl;
        if (commandIndex >= commands.size()) {
            // cout << "Process " << processName << " has no more commands to execute." << endl;

            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        outFile << "(" << arrivalTimeStamp << ") "
            << "Core: " << coreIndex << " " << commands[commandIndex] << " " << processName << "\n";

        if (commandIndex == 100) {
            Status state = FINISHED;
            setStatus(state);
            setRunTimeStamp();
        }

        commandIndex++;
	}

    outFile.close();

}

void Process::executeStep() {
    if (status == FINISHED) return;

    if (status == READY) {
        setArrivalTime();
        setStatus(RUNNING);
    }

    if (commandIndex >= commands.size()) {
        setStatus(FINISHED);
        setRunTimeStamp();
        return;
    }

    std::string filename = processName + ".txt";
    std::ofstream outFile(filename, std::ios::app);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // run time for each process

    outFile << "(" << arrivalTimeStamp << ") "
            << "Core: " << coreIndex << " "
            << commands[commandIndex] << " "
            << processName << "\n";

    commandIndex++;
    outFile.close();

    if (commandIndex >= commands.size()) {
        setStatus(FINISHED);
        setRunTimeStamp();
    }
}

