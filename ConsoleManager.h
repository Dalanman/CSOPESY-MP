#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <Windows.h>
#include <string>

const string MAIN_CONSOLE = "MAIN_CONSOLE";
const string MARQUEE_CONSOLE = "MARQUEE_CONSOLE";
const string SCHEDULING_CONSOLE = "SCHEDULING_CONSOLE";
const string MEMORY_CONSOLE = "MEMORY_CONSOLE";

class ConsoleManager
{
public:
    typedef std::unordered_map<string, std::shared_ptr<AConsole>> ConsoleTable;

    static ConsoleManager* getInstance();
    static void initialize();
    static void destroy();

    void drawConsole() const;
    void process() const;
    void switchConsole(string consoleName);
    void returnToPreviousConsole();
    void exitApplication();
    bool isRunning() const;

    HANDLE getConsoleHandle() const;

    void setCursorPosition(int posX, int posY) const;

private:
    ConsoleManager();
    ~ConsoleManager() = default;
    ConsoleManager(ConsoleManager const&) {}; // copy constructor is private
    ConsoleManager& operator=(ConsoleManager const&) {}; // assignment operator is private
    static ConsoleManager* sharedInstance;

    ConsoleTable consoleTable;
    std::shared_ptr<AConsole> currentConsole;
    std::shared_ptr<AConsole> previousConsole;

    HANDLE consoleHandle;
    bool running = true;
};
