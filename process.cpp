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

    creationTimeStamp = oss.str();
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
    int lineCounter = 0;
    setArrivalTime();

    std::string filename = processName + ".txt";
    std::ofstream outFile(filename, std::ios::app);

    if (!outFile.is_open())
    {
        std::cerr << "Failed to open file for writing logs: " << filename << std::endl;
        return;
    }

    outFile << "\nLogs:\n";

    for (const std::string &cmd : commands)
    {
        outFile << "(" << creationTimeStamp << ")\t"
                << "Core: " << coreIndex << " " << cmd << "\n";
        lineCounter++;
        commandIndex = lineCounter;
    }
    if (lineCounter == commands.size() - 1)
    {
        Status state = FINISHED;
        setStatus(state);
        setRunTimeStamp();
    }

    outFile.close();
}