#include "preference.h"

bool preferenceFile::hasBeenUpdated()
{
    return anyUpdates;
}

// trim whitespace from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim whitespace from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim whitespace from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

//From user Timmmm on stackoverflow, case insensitive string comparison
bool iequals(const std::string& a, const std::string& b)
{
    unsigned int sz = a.size();
    if (b.size() != sz)
        return false;
    for (unsigned int i = 0; i < sz; ++i)
        if (tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}

preference::~preference(){}

void preference::setName(std::string _name)
{
    if(_name.length() < 1)
        return;
    if(_name == name)
        return;

    lowerName = "";
    name = _name;
    for(unsigned int a = 0; a<name.length(); a++)
        lowerName += tolower(name[a]);
}

std::string preference::getName()
{
    return name;
}

//Force all preferences to have a name
preference::preference(std::string _name) : name(_name)
{
    //Store a lower case copy of the name for quick case insensitive searching
    for(unsigned int a = 0; a<name.length(); a++)
        lowerName += tolower(name[a]);
}

stringPreference::~stringPreference(){}

stringPreference::stringPreference(std::string _name) : preference(_name) {}

std::string stringPreference::textSerialize()
{
    return "String\t" + name + "\t" + value + "\n";
}

bool stringPreference::toBool()
{
    //A string of 'true', '1', or 'yes' represents a true value, case insensitive
    return iequals(value,"true") || value == "1" || iequals(value,"yes");
}

int stringPreference::toInteger()
{
    return atoi(value.c_str());
}

void stringPreference::set(std::string _value)
{
    if(value != _value)
    {
        updated = true;
        value = _value;
    }
}

void stringPreference::set(int _value)
{
    std::string newValue = std::to_string(_value);
    if(newValue != value)
    {
        value = newValue;
        updated = true;
    }
}

void stringPreference::set(bool _value)
{
    std::string newValue = _value ? "True" : "False";
    if(newValue != value)
    {
        updated = true;
        value = newValue;
    }
}

void stringPreference::reset()
{
    value = "default";
    updated = true;
}

std::string stringPreference::toString()
{
    return value;
}

integerPreference::~integerPreference(){}

integerPreference::integerPreference(std::string _name) : preference(_name) {}

std::string integerPreference::textSerialize()
{
    return "Integer\t" + name + "\t" + toString() + "\n";
}

bool integerPreference::toBool()
{
    return value != 0;
}

int integerPreference::toInteger()
{
    return value;
}

void integerPreference::set(std::string _value)
{
    int newValue = atoi(_value.c_str());
    if(newValue != value)
    {
        value = newValue;
        updated = true;
    }
}

void integerPreference::set(int _value)
{
    if(_value != value)
    {
        updated = true;
        value = _value;
    }
}

void integerPreference::set(bool _value)
{
    int newValue = _value ? 1 : 0;
    if(newValue != value)
    {
        value = newValue;
        updated = true;
    }
}

void integerPreference::reset()
{
    value = 0;
    updated = true;
}

std::string integerPreference::toString()
{
    return std::to_string(value);
}

boolPreference::~boolPreference(){}

boolPreference::boolPreference(std::string _name) : preference(_name) {}

std::string boolPreference::textSerialize()
{
    return "Bool\t" + name + "\t" + toString() + "\n";
}

bool boolPreference::toBool()
{
    return value;
}

int boolPreference::toInteger()
{
    return value ? 1 : 0;
}

void boolPreference::reset()
{
    updated = true;
    value = false;
}

std::string boolPreference::toString()
{
    return value ? "True" : "False";
}

void boolPreference::set(std::string _value)
{
    //Get rid of white space
    trim(_value);

    //A string of 'true', '1', or 'yes' represents a true value, case insensitive
    if(iequals(_value,"true") || _value == "1" || iequals(_value,"yes"))
    {
        if(!value)
            updated = true;
        value = true;
    }
    else
    {
        if(value)
            updated = true;
        value = false;
    }
}

void boolPreference::set(int _value)
{
    //0 is false, everything else is true
    if(_value == 0)
    {
        if(value)
            updated = true;
        value = false;
    }
    else
    {
        if(!value)
            updated = true;
        value = true;
    }
}

void boolPreference::set(bool _value)
{
    if(value != _value)
    {
        updated = true;
        value = _value;
    }
}

bool preferenceFile::removePreference(std::string name)
{
    std::string lowerSearch = "";
    for(unsigned int a = 0; a<name.length(); a++)
        lowerSearch += tolower(name[a]);

    for(unsigned int a = 0; a<preferences.size(); a++)
    {
        if(preferences[a]->lowerName == lowerSearch)
        {
            delete preferences[a];
            preferences[a] = 0;
            preferences.erase(preferences.begin() + a);
            return true;
        }
    }

    return false;
}

bool preferenceFile::addStringPreference(std::string name,std::string value = "")
{
    if(getPreference(name))
        return false;

    stringPreference *tmp = new stringPreference(name);
    tmp->set(value);
    preferences.push_back(tmp);

    return true;
}

bool preferenceFile::addIntegerPreference(std::string name,int value = 0)
{
    if(getPreference(name))
        return false;

    integerPreference *tmp = new integerPreference(name);
    tmp->set(value);
    preferences.push_back(tmp);

    return true;
}

bool preferenceFile::addBoolPreference(std::string name,bool value = false)
{
    if(getPreference(name))
        return false;

    boolPreference *tmp = new boolPreference(name);
    tmp->set(value);
    preferences.push_back(tmp);

    return true;
}

preference *preferenceFile::getPreference(std::string name)
{
    std::string lowerSearch = "";
    for(unsigned int a = 0; a<name.length(); a++)
        lowerSearch += tolower(name[a]);
    for(unsigned int a = 0; a<preferences.size(); a++)
        if(preferences[a]->lowerName == lowerSearch)
            return preferences[a];
    return 0;
}

bool preferenceFile::importFromFile(std::string path)
{
    filePath = path;

    std::ifstream importFile(path.c_str());
    if(!importFile.is_open())
        return false;

    std::string line;
    while(!importFile.eof())
    {
        getline(importFile,line);
        //Break up the line into 3 fields using tabs
        unsigned int firstSpace = line.find("\t");
        unsigned int secondSpace = line.find("\t",firstSpace+1);
        if(firstSpace == secondSpace)
            continue;
        if(firstSpace == std::string::npos)
            continue;
        if(secondSpace == std::string::npos)
            continue;
        std::string type = line.substr(0,firstSpace);
        std::string name = line.substr(firstSpace+1,secondSpace-firstSpace-1);
        std::string value = line.substr(secondSpace+1,line.length()-secondSpace+1);

        //Does the preference already exist, just set it's value, otherwise add a new preference entry
        if(preference *tmp = getPreference(name))
            tmp->set(value);
        else
        {
            if(type == "String")
                addStringPreference(name);
            else if(type == "Integer")
                addIntegerPreference(name);
            else if(type == "Bool")
                addBoolPreference(name);
            if(preference *tmp = getPreference(name))
                tmp->set(value);
        }
    }

    return true;
}

bool preferenceFile::exportToFile(std::string path)
{
    std::ofstream exportFile(path.c_str());
    if(!exportFile.is_open())
        return false;

    for(unsigned int a = 0; a<preferences.size(); a++)
        exportFile<<preferences[a]->textSerialize();

    exportFile.close();

    return true;
}

preference *preferenceFile::pollFirstUpdate()
{
    for(pollUpdateIterator = 0; pollUpdateIterator<preferences.size(); pollUpdateIterator++)
    {
        if(preferences[pollUpdateIterator]->updated)
        {
            pollUpdateIterator++;
            return preferences[pollUpdateIterator-1];
        }
    }

    return 0;
}

preference *preferenceFile::pollNextUpdate()
{
    for( ; pollUpdateIterator<preferences.size(); pollUpdateIterator++)
    {
        if(preferences[pollUpdateIterator]->updated)
        {
            pollUpdateIterator++;
            return preferences[pollUpdateIterator-1];
        }
    }

    return 0;
}

void preferenceFile::setUpdateFlags(bool state)
{
    if(!(state || anyUpdates))
        return;

    anyUpdates = state;

    for(unsigned int a = 0; a<preferences.size(); a++)
        preferences[a]->updated = state;
}

//TODO: Do I really need three basically duplicate functions?
void preferenceFile::set(std::string name,std::string _value)
{
    preference *tmp = getPreference(name);
    if(!tmp)
        return;
    tmp->set(_value);
    if(tmp->updated)
        anyUpdates = true;
}
void preferenceFile::set(std::string name,int _value)
{
    preference *tmp = getPreference(name);
    if(!tmp)
        return;
    tmp->set(_value);
    if(tmp->updated)
        anyUpdates = true;
}
void preferenceFile::set(std::string name,bool _value)
{
    preference *tmp = getPreference(name);
    if(!tmp)
        return;
    tmp->set(_value);
    if(tmp->updated)
        anyUpdates = true;
}
