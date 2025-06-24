#pragma once
#ifndef COMMAND_H
#define COMMAND_H
#include <string>
#include <iostream>
using namespace std;
enum commandType
{
    IO,
    PRINT
};

class Command
{
public:
Command(int type, string value){
    if (type == 0) {
        type = IO;
    } else if (type == 1) {
        type = PRINT;
    } else {
        cout << "Invalid Type" << endl;
    }
}
void execute();

private:
commandType type;
string value;
string printValue;

};

#endif