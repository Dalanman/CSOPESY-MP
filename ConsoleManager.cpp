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

ConsoleManager::ConsoleManager() : pm(4) {
    std::thread Scheduler;  
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
    cout << R"(                               
        ___ ___  ___  _ __   ___  ___ _   _ 
       / __/ __|/ _ \| '_ \ / _ \/ __| | | |
      | (__\__ \ (_) | |_) |  __/\__ \ |_| |
       \___|___/\___/| .__/ \___||___/\__, |
                     |_|              |___/ 
    )" << endl;
    cout << GREEN << "\nHello, Welcome to CSOPESY commandline!" << RESET << endl;
    cout << YELLOW << "Type 'exit' to quit, 'clear' to clear the screen" << RESET << endl;
    cout << "Enter a command: ";
}

void ConsoleManager::initialize() {
    configReader = new ConfigReader();
    initialized = true;
}

void readConfig(){

}

bool ConsoleManager::handleCommand(const string& input){

    if (inSession) {
        if (input == "process-smi") {
            if (activeProcess) {
                cout << "\n=== Process SMI Report ===" << endl;
                cout << "Process Name: " << activeProcess->getProcessName() << endl;
                cout << "Process ID: " << activeProcess->getProcessId() << endl;
                cout << "Arrival Time: " << activeProcess->getArrivalTimestamp() << endl;
                cout << "Run Time: " << activeProcess->getRunTimestamp() << endl;

                string logFile = activeProcess->getProcessName() + ".txt";
                std::ifstream inFile(logFile);
                if (inFile.is_open()) {
                    cout << "\n--- Logs ---\n";
                    std::string line;
                    while (std::getline(inFile, line)) {
                        cout << line << endl;
                    }
                    inFile.close();
                } else {
                    cout << "No logs found.\n";
                }

                if (activeProcess->getStatus() == FINISHED) {
                    cout << "\nStatus: Finished!" << endl;
                }
            } else {
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
        cout << "\nEnter a command: ";
        return true;
    }

 // If NOT in a session
    else if(!inSession) {
        if (!initialized){
            if (input == "initialize")
            {
                // cout << "'Initialize' command recognized. Doing something.";
                this->initialize();
                cout << "Emulator initialized successfully." << endl;
                cout << "\nEnter a command: ";
            }
            else if (input == "exit"){
                cout << "'exit' command recognized. Exiting program.\n";
                if (Scheduler.joinable()) Scheduler.join();
                stopInput = true;
                return false;
            }
            else{
                cout << RED << "> Error: Emulator not initialized. Please run 'initialize' command first." << RESET << endl;
                cout << "\nEnter a command: ";
            }
        }
        else{
            if (input.substr(0, 9) == "screen -r" || input.substr(0, 9) == "screen -s") {
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
                } else if (activeProcess->getStatus() == FINISHED) {
                    cout << RED << "Process " << processName << " has already finished." << RESET << endl;
                } else {
                    clearScreen();
                    cout << YELLOW << "Attached to process: " << processName << RESET << endl;
                    inSession = true;
                }

                cout << "\nEnter a command: ";
            } 


            else if (input == "scheduler-stop")
            {
                pm.cancelAll();
                cout << "\nEnter a command: ";
            }
            else if (input == "scheduler-start")
            { 
                // cout << pm.getCores() << endl;
                pm.makeDummies(10, 100, "Hello world from");                // Initialize dummy processes
                if (Scheduler.joinable()) Scheduler.join(); // Wait if already running
                    Scheduler = std::thread(&ProcessManager::executeFCFS, &pm);
                cout << "\nEnter a command: ";
            }       
            else if (input == "report-util")
            {
                std::ofstream reportFile("csopesy-log.txt");

                if (!reportFile.is_open()) {
                    cout << RED << "Failed to open csopesy-log.txt for writing." << RESET << endl;
                } else {
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
                return false;
            }
            else if (input == "screen -ls") 
            {
                clearScreen();
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
