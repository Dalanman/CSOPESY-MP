#include "process.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <thread>
#include <cstddef>
#include "commandList.hpp"


Process::Process(const std::string& name, int id, int assignedCore, int totalInstructions, size_t maxMemPerProcess)
    : processName(name),
    processId(id),
    coreIndex(assignedCore),
    numCommands(totalInstructions),
    commandList(totalInstructions),
    memoryRequirement(maxMemPerProcess) //Add this
{
}

void Process::addCommand(string text)
{
    commands.emplace_back(text);
}

void Process::parse()
{
    //std::cout << "SIZE: " << commands.size() << std::endl;
    bool test = commandList.parseCommands(commands);
    //std::cout << "ACTUAL SIZE: " << commandList.getSize() << endl;
    //std::cout << test << endl;
    
    /*
    for (const auto &cmd : commands)
    {
        std::cout << "(" << cmd << ")" << std::endl;
    }
    */
    
    
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
    // Initialize on first run
    if (status == READY)
    {
        setArrivalTime();
        setStatus(RUNNING);
    }

    if (isSleeping())
    {
        std::ostringstream oss;
        oss << "(" << arrivalTimeStamp << ") "
            << "Core: " << coreIndex << " "
            << "SLEEPING (" << sleepRemainingTicks << " ticks left) "
            << processName;
        smiLogs.push_back(oss.str());
        return;
    }

    // Finish if all commands are done
    if (commandIndex >= commandList.getTotalCommands())
    {
        setRunTimeStamp();
        setStatus(FINISHED);
        return;
    }

    std::shared_ptr<Command> currentCommand = commandList.getCommand(commandIndex);

    // Handle FOR unrolling safely
    if (currentCommand->type == FOR)
    {
        auto forCmd = std::dynamic_pointer_cast<ForCommand>(currentCommand);
        if (forCmd)
        {
            auto expanded = forCmd->unrollBody();
            commandList.removeCommandAt(commandIndex);
            commandList.insertCommandsAt(commandIndex, expanded);
            // Don't return early — let the loop fall through to execute next instruction
            if (commandIndex >= commandList.getTotalCommands())
            {
                setRunTimeStamp();
                setStatus(FINISHED);
                return;
            }
            currentCommand = commandList.getCommand(commandIndex);  // refresh pointer
        }
    }

    setRunTimeStamp();

    if (currentCommand->type == PRINT)
    {
        std::ostringstream oss;
        std::string cmdStr = currentCommand->toString();
        std::string printParam;
        size_t firstQuote = cmdStr.find('\"');
        size_t lastQuote = cmdStr.rfind('\"');
        if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote)
        {
            printParam = cmdStr.substr(firstQuote + 1, lastQuote - firstQuote - 1);
        }
        else
        {
            printParam = cmdStr;
        }
        oss << "(" << getRunTimestamp() << ") "
            << "Core: " << coreIndex << " "
            << printParam;
        smiLogs.push_back(oss.str());
    }

    switch (currentCommand->type)
    {
    case PRINT:
        currentCommand->printExecute(getRunTimestamp(), coreIndex, &logs);
        commandIndex++;
        break;

    case IO:
    {
        auto ioCmd = std::dynamic_pointer_cast<IOCommand>(currentCommand);
        if (ioCmd && ioCmd->getOperation() == "SLEEP")
        {
            sleepRemainingTicks = ioCmd->getSleepTicks();
            commandIndex++;
        }
        else
        {
            currentCommand->IOExecute();
            commandIndex++;
        }
        break;
    }

    default:
        commandIndex++;
        break;
    }

    if (commandIndex >= commandList.getTotalCommands())
    {
        setRunTimeStamp();
        setStatus(FINISHED);
    }
}
