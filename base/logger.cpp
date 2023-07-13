#include "logger.h"

//What the hell did I do here?
//Still it basically works...

/*
    logger::setErrorFile("logs/error.txt");
    logger::setInfoFile("logs/log.txt");

    scope("Main");
    log().setDebug(true);
    debug("Test");
    info("Hi");
    error("Whoops");

*/

//TODO: Have it make a folder if one is in the path of setInfoFile/setErrorFile yet doesn't exist

namespace syj
{
    std::function<logger&()> log = logger::get;
    std::function<void(std::string)> debug = logger::debug;
    std::function<void(std::string)> error = logger::error;
    std::function<void(std::string)> info =  logger::infon;
    std::function<void(std::string)> addScope =  logger::addScope;
    std::function<void()> leave =  logger::leave;

    void logger::addScope(std::string text)
    {
        //Add scope on top of the stack
        get().scopes.push_back(text);
    }

    void logger::leave()
    {
        if(get().scopes.size() < 1)
            return;
        //Remove last added scope from the stack
        get().scopes.erase(get().scopes.begin() + get().scopes.size() - 1);
    }

    void logger::setDebug(bool useDebug)
    {
        get().debugMode = useDebug;
    }

    logger &logger::get()
    {
        static logger instance;
        return instance;
    }

    logger &logger::operator<<(std::string text)
    {
        if(!get().newLineNeeded)
        {
            if( logger::get().infoFile.is_open())
                logger::get().infoFile<<format("",true);
            std::cout<<format("",true);
        }

        if( logger::get().infoFile.is_open())
            logger::get().infoFile<<text;
        std::cout<<text;
        if(logCallback)
            logCallback(text);
        get().newLineNeeded = true;
        return get();
    }

    logger &logger::operator<<(int text)
    {
        get()<<std::to_string(text);
        return get();
    }

    logger &logger::operator<<(float text)
    {
        get()<<std::to_string(text);
        return get();
    }

    std::string logger::format(std::string text,bool noLine,bool noHeader)
    {
        time_t     now = time(0);
        struct tm  tstruct;
        char       buf[80];
        tstruct = *localtime(&now);
        strftime(buf, sizeof(buf), "%Y/%m/%d - %X", &tstruct);

        std::string ret = "";
        if(!noHeader)
        {
            ret = std::string(buf) + " - ";
            for(unsigned int a = 0; a<get().scopes.size(); a++)
            {
                std::string scope = get().scopes[a];
                if(scope.length() > 0)
                    ret += scope + " - ";
            }
        }
        ret += text;
        if(!noLine)
            ret += "\n";
        return ret;
    }

    //This probably isn't how you're supposed to implement a singleton...
    bool logger::setInfoFile(std::string path)
    {
        if( logger::get().infoFile.is_open())
             logger::get().infoFile.close();
        logger::get().infoFileOpened = false;
        logger::get().infoFile.open(path.c_str(),std::ios_base::app);
        if( logger::get().infoFile.is_open())
        {
            logger::get().infoFileOpened = true;
            return true;
        }
        else
            return false;
    }

    bool logger::setErrorFile(std::string path)
    {
        if( logger::get().errorFile.is_open())
             logger::get().errorFile.close();
        logger::get().errorFileOpened = false;
        logger::get().errorFile.open(path.c_str(),std::ios_base::app);
        if( logger::get().errorFile.is_open())
        {
            logger::get().errorFileOpened = true;
            return true;
        }
        else
            return false;
    }

    void logger::setCallback(std::function<void(std::string text)> toSet)
    {
        logger::get().logCallback = toSet;
    }

    void logger::error(std::string text)
    {
        if( logger::get().errorFile.is_open())
            logger::get().errorFile<<logger::format(text);

        info("ERROR - " + text);
    }

    void logger::info(std::string text,bool noLine)
    {
        if(get().newLineNeeded)
        {
            get().newLineNeeded = false;
            if( logger::get().infoFile.is_open())
                logger::get().infoFile<<"\n";
            std::cout<<"\n";
        }

        if( logger::get().infoFile.is_open())
            logger::get().infoFile<<logger::format(text,noLine);

        std::cout<<logger::format(text,noLine);
        if(get().logCallback)
            get().logCallback(logger::format(text,noLine));
    }

    void logger::infon(std::string text)
    {
        info(text);
    }

    void logger::debug(std::string text)
    {
        if(logger::get().debugMode)
            info(text);
    }
}
