#ifndef PREFERENCE_H_INCLUDED
#define PREFERENCE_H_INCLUDED

#include <iostream>
#include <vector>
//Used for trimming:
#include <algorithm>
#include <functional>
#include <cctype>
//For importing and exporting
#include <fstream>

class preferenceFile;

//Abstract
struct preference
{
    friend class preferenceFile;

    protected:

    //What is the title/key for this preference
    std::string name = "";
    std::string lowerName = "";

    public:

    //Has the value of this preference been updated recently, creation counts as an update
    bool updated = true;

    //The following three methods allow any preference to be converted to any other type of preference
    virtual std::string toString() = 0;
    virtual int toInteger() = 0;
    virtual bool toBool() = 0;
    //Reset to default value ("",0,or false)
    virtual void reset() = 0;
    //Set preference value, will preform conversions automatically
    virtual void set(std::string _value) = 0;
    virtual void set(int _value) = 0;
    virtual void set(bool _value) = 0;
    //Output Type TAB Name TAB Value NEWLINE
    virtual std::string textSerialize() = 0;

    //Set name and also update lowerName
    void setName(std::string _name);
    //Returns name
    std::string getName();

    preference(std::string _name);
    virtual ~preference();
};

struct stringPreference : preference
{
    private:

    std::string value = "default";

    public:

    std::string toString();
    int toInteger();
    bool toBool();
    void reset();
    void set(std::string _value);
    void set(int _value);
    void set(bool _value);
    std::string textSerialize();
    stringPreference(std::string _name);
    ~stringPreference();
};

struct integerPreference : preference
{
    private:

    int value = 0;

    public:

    std::string toString();
    int toInteger();
    bool toBool();
    void reset();
    void set(std::string _value);
    void set(int _value);
    void set(bool _value);
    std::string textSerialize();

    integerPreference(std::string _name);
    ~integerPreference();
};

struct boolPreference : preference
{
    private:

    bool value = false;

    public:

    std::string toString();
    int toInteger();
    bool toBool();
    void reset();
    void set(std::string _value);
    void set(int _value);
    void set(bool _value);
    std::string textSerialize();

    boolPreference(std::string _name);
    ~boolPreference();
};

class preferenceFile
{
    //Used with pollFirstUpdate and pollNextUpdate
    unsigned int pollUpdateIterator = 0;

    std::vector<preference*> preferences;

    //Has any preference been updated?
    bool anyUpdates = false;

    std::string filePath = "";

    public:

    std::string getFilePath() { return filePath; }

    //These call the preference's set method but also set anyUpdates to true
    void set(std::string name,std::string _value);
    void set(std::string name,int _value);
    void set(std::string name,bool _value);

    //Return anyUpdates
    bool hasBeenUpdated();

    //Cycle through all preferences that have updated = true, call firstUpdate once, then nextUpdate
    preference *pollFirstUpdate();
    //Will return 0 when at end of preferences
    preference *pollNextUpdate();

    //Add preference, returns false if there's already one by that name
    bool addStringPreference(std::string name,std::string value);
    bool addIntegerPreference(std::string name,int value);
    bool addBoolPreference(std::string name,bool value);

    //Find preference by case insensitive name, returns 0 if none found
    preference *getPreference(std::string name);

    //Set all preferences to updated (true) or not updated (false)
    void setUpdateFlags(bool state);

    //Writes preferences to a text file, returns false on error
    bool exportToFile(std::string path);

    //Imports preferences from a text file, returns false on error
    bool importFromFile(std::string path);

    //Find a string by case insensitive name and delete its entry, returns true if an entry deleted
    bool removePreference(std::string name);
};

#endif // PREFERENCE_H_INCLUDED
