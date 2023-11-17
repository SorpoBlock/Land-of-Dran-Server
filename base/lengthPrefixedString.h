#ifndef LENGTHPREFIXEDSTRING_H_INCLUDED
#define LENGTHPREFIXEDSTRING_H_INCLUDED

#include <iostream>  //ostream
#include <string.h>  //memcpy
#include <vector>
#include <sstream>

//This little library is my implementation of a lightweight 8-bit length prefixed string type
//It can have 0 values in it since it is not null-terminated and it's cross platform
//There's going to eventually be support for a 16-bit character encoding as well

#define syjLPS_encodingFlag 0b10000000    //If enabled means string has 16-bit characters instead of 8-bit ones
#define syjLPS_lengthFlag   0b01000000    //If enabled means string length is 14 bits as opposed to 6 bits

//Did not know where else to put this
void replaceAll(std::string& source, const std::string& from, const std::string& to);
void split(const std::string &s,std::vector<std::string> &elems);
std::string lowercase(std::string in);

namespace syj
{
    typedef class lengthPrefixedString
    {
        //Here's where our actual data goes!
        //First bit refers to the encoding, 0 for 8 bit strings 1 for 16 bit strings (not used)
        //The next bit refers to the amount of bits given to the length field, either 6 or 14
        //The next 6 or 14 bits are the length of the string data itself in characters

        public: //debug

        unsigned char *data = 0;

        public:

        //Cast to normal C-String
        operator std::string() const;

        //Comparison
        bool operator==(const lengthPrefixedString &toCompare);

        //String concatenation
        lengthPrefixedString& operator+=(const lengthPrefixedString &toAdd);

        //String concatenation
        lengthPrefixedString& operator+=(const char *toAdd);

        //Conversion from regular strings
        lengthPrefixedString& operator=(const std::string &toConvert);

        lengthPrefixedString& operator=(const lengthPrefixedString &toCopy);

        //Conversion from regular strings
        lengthPrefixedString& operator=(const char* source);

        //Write to cout
        friend std::ostream &operator<<(std::ostream &stream,const lengthPrefixedString &us);

        //String concatenation
        friend lengthPrefixedString operator+(const lengthPrefixedString &result,const lengthPrefixedString &toAdd);

        //String concatenation
        friend lengthPrefixedString operator+(const lengthPrefixedString &result,const char *toAdd);

        //How many characters long is this string
        unsigned short getLength() const;

        //Clear the entire string, giving it <bytes> number of characters all initialized to zero
        void clear(unsigned short bytes = 0);

        //Create a string with a length of zero characters.
        lengthPrefixedString();

        //Create an LPS from a standard string
        lengthPrefixedString(const std::string &toConvert);

        //Conversion from char array
        lengthPrefixedString(const char *source);

        //Copy constructor, allocate a new identical copy
        lengthPrefixedString(const lengthPrefixedString &toCopy);

        //Delete data block
        ~lengthPrefixedString();
    } LPS;
}

#endif // LENGTHPREFIXEDSTRING_H_INCLUDED
