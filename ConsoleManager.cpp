#include "ConsoleManager.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include "Colors.h"
#include <fstream>
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

ConsoleManager::ConsoleManager(){
    
}

ConsoleManager::~ConsoleManager() {
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
    
}

void readConfig(){

}

void ConsoleManager::listAllProcess() {
    // std::cout << "test" << std::endl;
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
        else {

        }
    }
}

bool ConsoleManager::handleCommand(const string& input){
    
    // handles 'exit' if inside "process view" session
    if (inSession) {
        if (input == "exit") {
            cout << "> Exiting session..." << endl;
            inSession = false;  
            system("cls");  
            printHeader();  
            return true;
        }
        else {
            cout << "> You are currently viewing a process. Use 'exit' to return." << endl;
            cout << "\nEnter a command: ";
        }
    }
    else if(!inSession) {
        if (input == "initialize")
        {
            cout << "'Initialize' command recognized. Doing something.";
            this->initialize();
            cout << "\nEnter a command: ";
        }
        else if (input.substr(0, 9) == "screen -r" || input.substr(0, 9) == "screen -s") {

            if (input.length() <= 10 || input.substr(10).find_first_not_of(' ') == string::npos) {
                // if no "process" name
                clearScreen();
                cout << RED << "> Error: Missing process name for 'screen -r' command." << RESET << endl;
            }
            else {
                string processName = input.substr(10);
                clearScreen();
                cout << YELLOW << "Process name: " + processName << RESET << endl;
                cout << "Current line: 100/100" << endl;
                cout << getCurrentTimeFormatted() << endl;
                inSession = true;
                cout << "\nEnter a command: ";
            }
        }
        else if (input == "scheduler-stop")
        {
            cout << "'scheduler-stop' command recognized. Doing something.";
            cout << "\nEnter a command: ";
        }
        else if (input == "scheduler-start")
        { 
            ProcessManager pm(4);
            pm.makeDummies(10, 100, "Hello world from");                // Initialize dummy processes
            pm.executeFCFS();
            
            // I think dito papasok 'yung FCFS assignment ng processes
        }
        else if (input == "report-util")
        {
            cout << "'report-util' command recognized. Doing something.";
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
            return false;
        }
        else if (input == "screen -ls") 
        {
            // pm.UpdateProcessScreen();
            listAllProcess();
            cout << "\nEnter a command: ";
        }
        else
        {
            cout << RED << "Unknown command: " << input << RESET << endl;
            cout << "\nEnter a command: ";
        }
    }

    else {
        cout << "> You are currently viewing a process. Use 'exit' to return." << endl;
    }

}