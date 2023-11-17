#include "lengthPrefixedString.h"

void replaceAll(std::string& source, const std::string& from, const std::string& to)
{
    std::string newString;
    newString.reserve(source.length());  // avoids a few memory allocations

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;

    while(std::string::npos != (findPos = source.find(from, lastPos)))
    {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }

    // Care for the rest after last occurrence
    newString += source.substr(lastPos);

    source.swap(newString);
}

void split(const std::string &s,std::vector<std::string> &elems)
{
    std::stringstream wordSplitter(s);
    elems.clear();
    std::string buffer;
    while (wordSplitter >> buffer)
    {
        if(buffer.length() > 0 && buffer != " ")
            elems.push_back(buffer);
    }
}

std::string lowercase(std::string in)
{
    std::string ret = "";
    for(unsigned int a = 0; a<in.length(); a++)
    {
        if(in[a] >= 'A' && in[a] <= 'Z')
            ret += tolower(in[a]);
        else if(in[a] >= 'a' && in[a] <= 'z')
            ret += in[a];
        else if(in[a] >= '0' && in[a] <= '9')
            ret += in[a];
        else if(in[a] == '-')
            ret += in[a];
        else if(in[a] == ' ')
            ret += in[a];
    }
    return ret;
}

namespace syj
{
    //Convert a regular string to an LPS
    lengthPrefixedString& lengthPrefixedString::operator=(const std::string &toConvert)
    {
        //Allocate a string of the proper length
        clear(toConvert.length());

        //Copy over the string right after the header
        if(toConvert.length() > 63)
            memcpy(data+2,toConvert.c_str(),toConvert.length());
        else
            memcpy(data+1,toConvert.c_str(),toConvert.length());

        //Is this standard in assignment operators?
        return *this;
    }

    //Clear and reallocate a certain amount of space
    void lengthPrefixedString::clear(unsigned short bytes)
    {
        //If there was a string before (there always should be) delete it
        if(data)
            delete data;

        //How long is our data going to be
        //One or two bytes plus the actual amount of characters
        unsigned int bufLength = (bytes > 63) ? bytes+2 : bytes+1;

        //Allocate our data and initialize it to all zeros
        data = new unsigned char[bufLength];
        for(unsigned int a = 0; a<bufLength; a++)
            data[a] = 0;

        //Setting the lower six bits is pretty simple in this case
        if(bytes < 64)
            data[0] += bytes;
        else
        {
            //Set the 14-bit length flag to true
            data[0] += syjLPS_lengthFlag;

            //Set the lower six bits like above but making sure we don't add more than 63 and mess up our flags
            data[0] += bytes & 0b00111111;

            //Set the upper 8 bits
            data[1] += (bytes & 0b11111111000000) >> 6;
        }
    }

    //How many character are in our string
    unsigned short lengthPrefixedString::getLength() const
    {
        //If the string is somehow invalid just return -1 for length and allocate a zero-character string
        if(!data)
            return 0;

        //The string will always have the same lower 6 bits of the length field
        unsigned ret = data[0] & 0b00111111;

        if(data[0] & syjLPS_lengthFlag)
            ret += data[1] << 6;
            //The next 8 bits (byte 1) if used are the higher 8 bits of the encoded length

        return ret;
    }

    lengthPrefixedString::lengthPrefixedString(const lengthPrefixedString &toCopy)
    {
        //No self assignment needed
        if(&toCopy == this)
            return;

        //If the string to copy is somehow invalid, just create an empty string
        if(!toCopy.data)
        {
            data = 0;
            clear();
            return;
        }

        //Take the payload/characters length and turn it into a total buffer length by adding header room
        unsigned short len = toCopy.getLength();
        len += (len > 63) ? 2 : 1;

        //Copy over the actual data wholesale
        data = new unsigned char[len];
        memcpy(data,toCopy.data,len);
    }

    lengthPrefixedString::lengthPrefixedString(const std::string &toConvert)
    {
        data = 0;
        //Allocate a string of the proper length
        clear(toConvert.length());

        //Copy over the string right after the header
        if(toConvert.length() > 63)
            memcpy(data+2,toConvert.c_str(),toConvert.length());
        else
            memcpy(data+1,toConvert.c_str(),toConvert.length());
    }

    //Basic empty string constructor
    lengthPrefixedString::lengthPrefixedString()
    {
        //I forgot to put this here and took me forever to realize what was going on
        data = 0;
        //Just make it a string of zero characters but with the proper header byte
        clear();
    }

    //Just deallocate our block of data
    lengthPrefixedString::~lengthPrefixedString()
    {
        if(data)
            delete data;
    }

    //Cast to normal C-String
    lengthPrefixedString::operator std::string() const
    {

        std::string ret = "";
        unsigned short length = getLength();
        //Literally just copy the data one byte at a time
        for(unsigned int a = 0; a<length; a++)
            ret += data[((length > 63) ? 2 : 1) + a];
        return ret;
    }

    //Conversion from char array
    lengthPrefixedString::lengthPrefixedString(const char *source)
    {
        //Get length of char*
        unsigned int len = 0;
        while(source[len])len++;

        //Allocate space
        data = 0;
        clear(len);

        memcpy(data+(len>63 ? 2 : 1),source,len);
    }

    //Assignment from char array
    lengthPrefixedString &lengthPrefixedString::operator=(const char *source)
    {
        //Get length of char*
        unsigned int len = 0;
        while(source[len])len++;

        //Allocate space
        data = 0;
        clear(len);

        memcpy(data+(len>63 ? 2 : 1),source,len);

        return *this;
    }

    //Add two LPSs together
    lengthPrefixedString& lengthPrefixedString::operator+=(const lengthPrefixedString &toAdd)
    {
        //How big will our new string be
        unsigned short length = getLength();
        unsigned short copyLength = toAdd.getLength();
        unsigned short newLength = length + copyLength;

        //Copy our data into a temp buffer
        char *oldData = new char[length];
        memcpy(oldData,(length > 63) ? data+2 : data+1,length);

        //Allocate space
        clear(newLength);

        //Copy data from both strings over
        unsigned char *start = (newLength > 63) ? data+2 : data+1;
        memcpy(start,oldData,length);
        memcpy(start+length,(copyLength > 63) ? toAdd.data+2 : toAdd.data+1,copyLength);

        //Delete buffer
        delete oldData;

        return *this;
    }

    //Add two LPSs together
    lengthPrefixedString& lengthPrefixedString::operator+=(const char *toAdd)
    {
        //Once again just easier to use preexisting functions
        //Probably less efficient though
        *this += lengthPrefixedString(toAdd);
        return *this;
    }

    //String comparison
    bool lengthPrefixedString::operator==(const lengthPrefixedString &toCompare)
    {
        unsigned short length = getLength();
        if(toCompare.getLength() != length)
            return false;
        //Don't bother comparing strings with unequal lengths

        //Where do the characters start in data
        unsigned char offsetA = (length > 63) ? 2 : 1;
        unsigned char offsetB = (toCompare.getLength() > 63) ? 2 : 1;

        //Go through character by character
        for(unsigned int a = 0; a<length; a++)
            if(toCompare.data[offsetB+a] != data[offsetA+a])
                return false;

        return true;
    }

    //Write to cout (or ofstream)
    std::ostream &operator<<(std::ostream &stream,const lengthPrefixedString &us)
    {
        //Just cast it and output normally
        stream<<std::string(us);
        return stream;
    }

    //String concatenation
    lengthPrefixedString operator+(const lengthPrefixedString &result,const lengthPrefixedString &toAdd)
    {
        //Just use the += operator rather than rewrite a function
        lengthPrefixedString ret(result);
        ret += toAdd;
        return ret;
    }

    //String concatenation
    lengthPrefixedString operator+(const lengthPrefixedString &result,const char *toAdd)
    {
        //Just use the += operator rather than rewrite a function
        lengthPrefixedString ret = result;
        ret += toAdd;
        return ret;
    }

    lengthPrefixedString& lengthPrefixedString::operator=(const lengthPrefixedString &toCopy)
    {
        //No self assignment needed
        if(&toCopy == this)
            return *this;

        //If the string to copy is somehow invalid, just create an empty string
        if(!toCopy.data)
        {
            data = 0;
            clear();
            return *this;
        }

        //Take the payload/characters length and turn it into a total buffer length by adding header room
        unsigned short len = toCopy.getLength();
        len += (len > 63) ? 2 : 1;

        //Copy over the actual data wholesale
        data = new unsigned char[len];
        memcpy(data,toCopy.data,len);

        return *this;
    }
}













