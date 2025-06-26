#include "process.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include "commandList.hpp"
Process::Process(const std::string &name, int id, int assignedCore, int totalInstructions)
    : processName(name),
      processId(id),
      coreIndex(assignedCore),
      numCommands(totalInstructions),
      commandList(totalInstructions)
{
    
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
    std::string filename = processName + ".txt";
    std::ofstream outFile(filename, std::ios::app);

    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing logs: " << filename << std::endl;
        return;
    }

    // Initialize on first run
    if (status == READY) {
        setArrivalTime();
        setStatus(RUNNING);
    }

    if (commandIndex >= commandList.getTotalCommands()) {
        setRunTimeStamp();
        setStatus(FINISHED);
        return;
    }

    std::shared_ptr<Command> currentCommand = commandList.getCommand(commandIndex);

    
    if (currentCommand->type == FOR) {
        auto forCmd = std::dynamic_pointer_cast<ForCommand>(currentCommand);
        if (forCmd) {
            auto expanded = forCmd->unrollBody();

            
            commandList.removeCommandAt(commandIndex);  // assume this exists
            commandList.insertCommandsAt(commandIndex, expanded);  // assume this exists

            
            outFile.close();
            return;
        }
    }

    
    outFile << "(" << arrivalTimeStamp << ") "
            << "Core: " << coreIndex << " "
            << currentCommand->toString() << " "
            << processName << "\n";

    
    switch (currentCommand->type) {
        case PRINT:
            currentCommand->printExecute(outFile, &logs);
            break;
        case IO:
            currentCommand->IOExecute();
            break;
        default:
            break;
    }

    commandIndex++;


    if (commandIndex >= commandList.getTotalCommands()) {
        setRunTimeStamp();
        setStatus(FINISHED);
    }

    outFile.close();
}



