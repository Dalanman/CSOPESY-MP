#include <iostream>
#include <string>
#include <windows.h>

using namespace std;

void clearScreen()
{
    cout << "\033[2J\033[1;1H";
}

void printHeader()
{
    cout << R"(                               
        ___ ___  ___  _ __   ___  ___ _   _ 
       / __/ __|/ _ \| '_ \ / _ \/ __| | | |
      | (__\__ \ (_) | |_) |  __/\__ \ |_| |
       \___|___/\___/| .__/ \___||___/\__, |
                     |_|              |___/ 
    )" << endl;
}
int main()
{

    printHeader();
    string input;

    while (true)
    {

        cout << "\nHello, Welcome to CSOPESY commandline!" << endl
             << "Type 'exit' to quit, 'clear' to clear the screen" << endl;
        cout << "Enter a command: ";
        cin >> input;

        if (input == "initialize")
        {
            cout << "'Initialize' command recognized. Doing something.";
        }
        else if (input == "screen")
        {
            cout << "'Screen' command recognized. Doing something.";
        }
        else if (input == "scheduler-test")
        {
            cout << "'scheduler-test' command recognized. Doing something.";
        }
        else if (input == "scheduler-stop")
        {
            cout << "'scheduler-stop' command recognized. Doing something.";
        }
        else if (input == "report-util")
        {
            cout << "'report-util' command recognized. Doing something.";
        }
        else if (input == "clear")
        {
            clearScreen();
            printHeader();
        }
        else if (input == "exit")
        {
            cout << "'exit' command recognized. Exiting program.\n";
            break;
        }
        else
        {
            cout << "Unknown command: " << input << endl;
        }
    }

    return 0;
}
