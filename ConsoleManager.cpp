#include "ConsoleManager.h"
#include "ConfigReader.hpp"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#define byte win_byte_override //added
#include <windows.h>
#include "Colors.h"
#include <fstream>
#include <thread>
#include <algorithm> //added
#undef byte //added
using namespace std;

void clearScreen()
{
    system("cls");
}

std::string getCurrentTimeFormatted() {
    std::time_t now = std::time(0);
    std::tm localTime;

#ifdef _WIN32
    // Windows: use localtime_s
    if (localtime_s(&localTime, &now) != 0) {
        return "Error getting local time";
    }
#else
    // Unix/Linux: use localtime_r
    if (localtime_r(&now, &localTime) == nullptr) {
        return "Error getting local time";
    }
#endif

    // Extract date and time components
    int month = localTime.tm_mon + 1;  // tm_mon is 0-based
    int day = localTime.tm_mday;
    int year = localTime.tm_year + 1900;  // tm_year is years since 1900

    int hour = localTime.tm_hour;
    int minute = localTime.tm_min;
    int second = localTime.tm_sec;

    // Convert to 12-hour format
    std::string ampm = (hour >= 12) ? "PM" : "AM";
    if (hour == 0) {
        hour = 12;
    }
    else if (hour > 12) {
        hour -= 12;
    }

    // Create formatted string
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << month << "/"
        << std::setfill('0') << std::setw(2) << day << "/"
        << year << ", "
        << std::setfill('0') << std::setw(2) << hour << ":"
        << std::setfill('0') << std::setw(2) << minute << ":"
        << std::setfill('0') << std::setw(2) << second << " "
        << ampm;

    return oss.str();
}

ConsoleManager::ConsoleManager() : pm(numCpu) {
    std::thread Scheduler;
    std::thread dummyMaker;
}

void ConsoleManager::run() {
    InputHandler = std::thread(&ConsoleManager::inputLoop, this);
}

void ConsoleManager::inputLoop() {
    printHeader();
    std::string input;
    while (!stopInput) {
        std::getline(std::cin, input);
        if (!handleCommand(input)) break;
    }
}

ConsoleManager::~ConsoleManager() {
    if (InputHandler.joinable()) InputHandler.join();
    if (Scheduler.joinable()) Scheduler.join();
}

bool ConsoleManager::isInSession() {
    return inSession;
}

void ConsoleManager::printHeader() {
    std::cout << R"(
     ______  __ __    ___       ____   ___    ____  ______   _____      ___    _____
    |      ||  |  |  /  _]     /    | /   \  /    ||      | / ___/     /   \  / ___/
    |      ||  |  | /  [_     |   __||     ||  o  ||      |(   \_     |     |(   \_ 
    |_|  |_||  _  ||    _]    |  |  ||  O  ||     ||_|  |_| \__  |    |  O  | \__  |
      |  |  |  |  ||   [_     |  |_ ||     ||  _  |  |  |   /  \ |    |     | /  \ |
      |  |  |  |  ||     |    |     ||     ||  |  |  |  |   \    |    |     | \    |
      |__|  |__|__||_____|    |___,_| \___/ |__|__|  |__|    \___|     \___/   \___|
                                                                            
    )" << std::endl;
    cout << GREEN << "\nHello, Welcome to CSOPESY commandline!" << RESET << endl;
    cout << YELLOW << "Type 'exit' to quit, 'clear' to clear the screen" << RESET << endl;
    cout << "Enter a command: ";
}

void ConsoleManager::initialize() {
    configReader = new ConfigReader();
    initialized = true;

    cpuTick = 500;

    numCpu = configReader->getNumCpu();
    quantumCycle = configReader->getQuantum();
    BPF = configReader->getBPF();
    DelayPerExec = configReader->getDelayPerExec();
    MinIns = configReader->getMinIns();
    MaxIns = configReader->getMaxIns();
    pm.setCore(numCpu);
}

void readConfig() {

}

bool ConsoleManager::handleCommand(const string& input) {

    if (inSession) {
        if (input == "process-smi") {
            if (activeProcess) {
                cout << "\n" << endl;
                cout << "Process Name: " << activeProcess->getProcessName() << endl;
                cout << "ID: " << activeProcess->getProcessId() << endl;
                cout << "Logs: " << endl;
                for (const auto& log : activeProcess->getLogs()) {
                    cout << log << endl;
                }
                cout << "\n" << endl;
                cout << "Current Instruction Line: " << activeProcess->getCommandIndex() << endl;
                cout << "Total Instructions: " << activeProcess->getTotalCommands() << endl;

                if (activeProcess->getStatus() == FINISHED) {
                    cout << "\nStatus: Finished!" << endl;
                }
            }
            else {
                cout << RED << "No process is currently active in session." << RESET << endl;
            }
        }
        else if (input == "exit") {
            cout << "> Exiting session..." << endl;
            inSession = false;
            activeProcess = nullptr;
            system("cls");
            printHeader();
        }
        else {
            cout << "> Unknown command in session. Try 'process-smi' or 'exit'." << endl;
        }
        if(inSession)
            cout << "\nEnter a command: ";
        return true;
    }

    // If NOT in a session
    else if (!inSession) {
        if (!initialized) {
            if (input == "initialize")
            {
                // cout << "'Initialize' command recognized. Doing something.";
                this->initialize();
                cout << "Emulator initialized successfully." << endl;
                cout << "\nEnter a command: ";
            }
            else if (input == "exit") {
                cout << "'exit' command recognized. Exiting program.\n";
                if (Scheduler.joinable()) Scheduler.join();
                stopInput = true;
                return false;
            }
            else {
                cout << RED << "> Error: Emulator not initialized. Please run 'initialize' command first." << RESET << endl;
                cout << "\nEnter a command: ";
            }
        }
        else {
            if (input.substr(0, 9) == "screen -r") {

                if (input.length() <= 10 || input.substr(10).find_first_not_of(' ') == string::npos) {
                    // if no "process" name
                    // clearScreen();
                    cout << RED << "> Error: Missing process name for 'screen -r' command." << RESET << endl;
                }
                else {
                    string processName = input.substr(10);
                    auto all = pm.getAllProcesses();
                    bool found = false;

                    for (const auto& proc : all) {
                        if (proc->getProcessName() == processName) {
                            activeProcess = proc;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        cout << RED << "Process " << processName << " not found or already finished." << RESET << endl;
                    }
                    else if (activeProcess->getStatus() == FINISHED) {
                        cout << RED << "Process " << processName << " has already finished." << RESET << endl;
                    }
                    else {
                        clearScreen();
                        cout << YELLOW << "Attached to process: " << processName << RESET << endl;
                        cout << "\n" << endl;
                        cout << "Process Name: " << activeProcess->getProcessName() << endl;
                        cout << "ID: " << activeProcess->getProcessId() << endl;
                        cout << "Logs: " << endl;
                        for (const auto& log : activeProcess->getLogs()) {
                            cout << log << endl;
                        }
                        cout << "\n" << endl;
                        cout << "Current Instruction Line: " << activeProcess->getCommandIndex() << endl;
                        cout << "Total Instructions: " << activeProcess->getTotalCommands() << endl;

                        if (activeProcess->getStatus() == FINISHED) {
                            cout << "\nStatus: Finished!" << endl;
                        }
                        inSession = true;
                    }
                }

                cout << "\nEnter a command: ";
            }
            else if (input.substr(0, 9) == "screen -s") {
                if (input.length() <= 10 || input.substr(10).find_first_not_of(' ') == string::npos) {
                    // if no "process" name
                    // clearScreen();
                    cout << RED << "> Error: Missing process name for 'screen -s' command." << RESET << endl;
                }
                else {
                    pm.makeDummy(input.substr(10), cpuTick, MinIns, MaxIns, BPF);
                    cout << GREEN << "Process " << input.substr(10) << " started successfully." << RESET << endl;
                }
                cout << "\nEnter a command: ";
            }

            else if (input == "scheduler-stop")
            {
                pm.stopDummy();
                if (dummyMaker.joinable()) dummyMaker.join();
                cout << "\nEnter a command: ";
            }
            else if (input == "scheduler-start")
            {
                // Create dummy processes
                if (dummyMaker.joinable()) dummyMaker.join();
                dummyMaker = std::thread(&ProcessManager::makeDummies, &pm, cpuTick, MinIns, MaxIns, BPF);
                if (Scheduler.joinable()) Scheduler.join();
                    
                if (configReader->getSchedulerType() == 0)
                {
                    // std::cout << "Executing FCFS" << std::endl;
                    Scheduler = std::thread(&ProcessManager::executeFCFS, &pm, numCpu, cpuTick, quantumCycle, DelayPerExec);
                }
                else 
                {
                    // std::cout << "Executing RR" << std::endl;
                    Scheduler = std::thread(&ProcessManager::executeRR, &pm, numCpu, cpuTick, quantumCycle, DelayPerExec);
                }
              

                cout << "\nEnter a command: ";
            }
            else if (input == "report-util")
            {
                std::ofstream reportFile("csopesy-log.txt");

                if (!reportFile.is_open()) {
                    cout << RED << "Failed to open csopesy-log.txt for writing." << RESET << endl;
                }
                else {
                    reportFile << "==== CSOPESY CPU UTILIZATION REPORT ====\n";
                    reportFile << "Timestamp: " << getCurrentTimeFormatted() << "\n\n";

                    int cores = pm.getCores();
                    reportFile << "CPU Cores Available: " << cores << "\n";

                    // Count cores in use
                    std::vector<bool> coreUsed(cores, false);
                    for (const auto& proc : pm.getAllProcesses()) {
                        if (proc->getStatus() == RUNNING) {
                            coreUsed[proc->getCoreIndex()] = true;
                        }
                    }
                    int used = std::count(coreUsed.begin(), coreUsed.end(), true);
                    reportFile << "Cores Currently in Use: " << used << "\n\n";

                    reportFile << "-- Running Processes --\n";
                    for (const auto& proc : pm.getAllProcesses()) {
                        if (proc->getStatus() == RUNNING) {
                            reportFile << "Name: " << proc->getProcessName() << "\t"
                                << "Arrival: " << proc->getArrivalTimestamp() << "\t"
                                << "Core: " << proc->getCoreIndex() << "\t"
                                << "Progress: " << proc->getCommandIndex() << "/"
                                << proc->getTotalCommands() << "\n";
                        }
                    }

                    reportFile << "\n-- Finished Processes --\n";
                    for (const auto& proc : pm.getAllProcesses()) {
                        if (proc->getStatus() == FINISHED) {
                            reportFile << "Name: " << proc->getProcessName() << "\t"
                                << "Arrival: " << proc->getArrivalTimestamp() << "\t"
                                << "Finished\t"
                                << "Total: " << proc->getTotalCommands() << "\n";
                        }
                    }

                    reportFile.close();
                    cout << GREEN << "Utilization report saved to csopesy-log.txt" << RESET << endl;
                }

                cout << "\nEnter a command: ";
            }

            else if (input == "clear")
            {
                clearScreen();
                printHeader();
            }
            else if (input == "exit")
            {
                cout << "'exit' command recognized. Exiting program.\n";
                if (Scheduler.joinable()) Scheduler.join();
                stopInput = true;
                if (InputHandler.joinable()) InputHandler.join();
                return false;
            }
            else if (input == "screen -ls")
            {
                // clearScreen();
                pm.UpdateProcessScreen();
                cout << "\nEnter a command: ";
            }
            else
            {
                cout << RED << "Unknown command: " << input << RESET << endl;
                cout << "\nEnter a command: ";
            }
        }

    }

    else {
        cout << "> You are currently viewing a process. Use 'exit' to return." << endl;
    }
    return true; //added
}