#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include <iostream>
#include <fstream>
#include <time.h>
#include <functional>
#include <vector>

namespace syj
{
    //Gets the file name and extension from a full file path
    //e.g. "assets/bob/bob.png" as filepath returns "bob.png"
    std::string getFileFromPath(std::string in);

    //This class is a singleton
    class logger
    {
        public:
            //Pushes and pops scopes from the stack for logging:
            static void addScope(std::string text);
            static void leave();

            //Three levels of logging:
            static void info(std::string text,bool noLine = false);
            static void error(std::string text);
            static void debug(std::string text); //writes to infoFile only if debugMode = true
            static void infon(std::string text); //This shouldn't be needed, what am I doing

            static void setDebug(bool useDebug);

            logger &operator<<(std::string);
            logger &operator<<(int);
            logger &operator<<(float);

            //File control:
            static bool setInfoFile(std::string path);
            static bool setErrorFile(std::string path);

            //Not really needed now that all the methods are static I guess?
            static logger& get();

            //Make sure there can't ever be more than one logger at a time
            logger(const &logger) = delete;
            logger &operator=(const &logger) = delete;

            //Call back for when text is logged:
            std::function<void(std::string text)> logCallback = 0;
            static void setCallback(std::function<void(std::string text)> toSet);

        private:
            static std::string format(std::string text,bool noLine = false,bool noHeader = false);

            std::vector<std::string> scopes;
            bool newLineNeeded = false;
            bool debugMode = false;
            bool infoFileOpened = false;
            bool errorFileOpened = false;
            std::ofstream infoFile;
            std::ofstream errorFile;

            logger() {}
    };

    extern std::function<logger&()> log;
    extern std::function<void(std::string)> info;
    extern std::function<void(std::string)> debug;
    extern std::function<void(std::string)> error;
    extern std::function<void(std::string)> addScope;
    extern std::function<void()> leave;

    //The whole point of this, is just to call global leave() when at the end of a given scope:
    struct scopeLogger
    {
        scopeLogger(){};
        ~scopeLogger(){leave();};
    };
}


//Adds a scope name to the stack, and removes it when the current scope is left
//If this throws an error saying undefined, be sure you're using namespace syj
#define scope(n) addScope(n); syj::scopeLogger tempScopeName;

#endif // LOGGER_H_INCLUDED
