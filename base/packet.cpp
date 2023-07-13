#include "packet.h"

namespace syj
{
    //Helper function not core functionality for printBytes
    unsigned char flipBits(unsigned char b)
    {
       b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
       b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
       b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
       return b;
    }

    //Helper function not core functionality for printBytes
    std::string uint8tostr(unsigned char in)
    {
        std::string ret = "";
        ret += '0' + in/100 % 10;
        ret += '0' + in/10  % 10;
        ret += '0' + in     % 10;
        return ret;
    }

    //Copy a float to 4 uChars and write
    bool packet::writeFloat(float in)
    {
        unsigned char buf[4];
        memcpy(buf,&in,4);
        for(unsigned int a = 0; a<4; a++)
            writeUChar(buf[a]);
        //TODO: Write check
        return true;
    }

    //Read a float from 4 uChars and return
    float packet::readFloat()
    {
        float ret = 0;
        unsigned char buf[4];
        for(unsigned int a = 0; a<4; a++)
            buf[a] = readUChar();
        memcpy(&ret,buf,4);
        return ret;
    }

    //Move the stream pointer along a certain number of bits
    bool packet::move(int bitPos)
    {
        //Move forward
        if(bitPos > 0)
        {
            streamPosByte += bitPos / 8;
            streamPosBit += bitPos % 8;

            //Advance to next byte
            while(streamPosBit >= 8)
            {
                streamPosBit -= 8;
                streamPosByte++;
            }
        }
        //Move backward
        else
        {
            bitPos = 0 - bitPos;
            streamPosByte -= bitPos / 8;
            streamPosBit -= bitPos % 8;

            //Go back to previous byte
            while(streamPosBit < 0)
            {
                streamPosBit += 8;
                streamPosByte--;
            }

            if(streamPosByte < 0)
            {
                //If we go before the first bit throw an error and start a pos 0
                streamPosByte = 0;
                streamPosBit = 0;
                lastError = syjError::endOfStream;
                return false;
            }
        }
        return streamPosByte < allocatedChunks * syjNET_PacketChunkSize;
    }

    //Write a single bit/bool to the stream returns true on success
    bool packet::writeBit(const bool &value,bool allocCheck)
    {
        //Make sure we have enough space
        if(allocCheck)
        {
            //Don't preform check if this is being called from a different write function
            if(!allocationCheck(1))
                return false;
        }

        //The bit we want to set will be set to 1 and all else 0 (even if value == false)
        unsigned char bit = (1<<streamPosBit);

        //Actually set whatever bit we're at in the byte
        if(value)
            data[streamPosByte] |= bit;
        else
            data[streamPosByte] &= ~bit;

        //Move the stream pointer
        move(1);

        return true;
    }

    //Read a single bit from the stream
    bool packet::readBit()
    {
        //If we reached the end of the stream
        //Note: useful data may have ended long ago, this is merely the end of the final allocated chunk
        if(streamPosByte >= allocatedChunks * syjNET_PacketChunkSize)
        {
            lastError = syjError::endOfStream;
            return false;
        }

        //Get our actual value
        bool ret = data[streamPosByte] & (1<<streamPosBit);

        //Move pointer along a bit
        move(1);

        return ret;
    }

    unsigned char packet::readUChar()
    {
        //If we reached the end of the stream
        //Note: useful data may have ended long ago, this is merely the end of the final allocated chunk
        if(streamPosByte >= allocatedChunks * syjNET_PacketChunkSize)
        {
            lastError = syjError::endOfStream;
            return 0;
        }

        unsigned char ret = 0;

        //The easy way if we're at the beginning of a byte
        if(streamPosBit == 0)
        {
            ret = data[streamPosByte];
            move(8);
        }
        else
        {
            ret = readUInt(8);
            //Another check for the next byte we have to access
            /*if(streamPosByte+1 >= allocatedChunks * syjNET_PacketChunkSize)
            {
                lastError = syjError::endOfStream;
                return 0;
            }

            //Read upper bytes from current byte
            ret += data[streamPosByte]   &  (255 << streamPosBit);

            //Read lower bytes from next byte
            ret += data[streamPosByte+1] &  ((1<<streamPosBit)-1);*/
        }

        //Move pointer forward one byte... never mind
        //move(8);

        return ret;
    }

    bool packet::writeUChar(const unsigned char &value,bool allocCheck)
    {
        //Make sure we have enough space
        if(allocCheck)
        {
            //Don't preform check if this is being called from a different write function
            if(!allocationCheck(2))
                return false;
        }

        //If we're at the start of a byte, we can just set the char directly, it's quicker that way
        if(streamPosBit == 0)
        {
            data[streamPosByte] = value;
            move(8);
        }
        else
        {
            //Why not
            writeUInt(value,8);

            //The slower way

            //Highest bits go into the first byte
            /*unsigned char ech = value & (255 << streamPosBit);
            data[streamPosByte] |=  ech;
            ech |= (255 >> (8-streamPosBit));
            data[streamPosByte] &=  ech;

            //Lowest bits go into the next byte
            //TODO: This clears the rest of the byte but shouldn't
            data[streamPosByte+1] = value & ((1<<streamPosBit)-1);*/
        }

        //Move stream pointer... never mind
        //move(8)

        return true;
    }

    packet &packet::operator<<(const lengthPrefixedString &string)
    {
        writeString(string);
        return *this;
    }

    bool packet::writeString(const lengthPrefixedString &string)
    {
        unsigned int length = string.getLength();
        if(length > 63)
            length += 2;
        else
            length++;

        for(unsigned int a = 0; a<length; a++)
            if(!writeUChar(string.data[a]))
                return false;
        return true;
    }

    lengthPrefixedString packet::readString()
    {
        //String goes here
        unsigned int offset = 1;
        unsigned int length = 0;
        unsigned char *buffer = 0;

        //Get the first byte that tells us how long the length field is
        unsigned char first = readUChar();
        //The lower 6 bits of the length
        length += first & 0b00111111;

        //Is string 16 bit length
        if(first & syjLPS_lengthFlag)
        {
            unsigned char second = readUChar();
            //Add 8 more bits of length
            length += second << 6;

            offset = 2;
            buffer = new unsigned char[length+2];
            buffer[1] = second;
        }
        else
            buffer = new unsigned char[length+1];

        //First byte set the same no matter what
        buffer[0] = first;

        //Read and copy over the rest of the string
        for(unsigned int a = 0; a<length; a++)
            buffer[offset+a] = readUChar();

        //Create a basic LPS
        lengthPrefixedString empty;
        //Delete the header (starts with 1 byte that says it has 0 characters)
        delete empty.data;
        //Replace with our own string
        empty.data = buffer;
        return empty;
    }

    packet &packet::operator<<(packetPrecision _precision)
    {
        precision = _precision.value;
        return *this;
    }

    packet &packet::operator<<(bool bit)
    {
        writeBit(bit);
        return *this;
    }

    packet &packet::operator<<(unsigned long value)
    {
        if(precision == 8)
            writeUChar(value);
        else
            writeUInt(value,precision);
        return *this;
    }

    unsigned long long packet::readUInt(unsigned char bits,bool debug)
    {
        //If we reached the end of the stream
        //Note: useful data may have ended long ago, this is merely the end of the final allocated chunk
        if(streamPosByte+(bits/8)+1 >= allocatedChunks * syjNET_PacketChunkSize)
        {
            lastError = syjError::endOfStream;
            return 0;
        }

        unsigned long long ret = 0;

        //Keep going through unsigned char *data byte by byte extracting the relevant data
        for(int bitsToRead = bits; bitsToRead>0; )
        {
            //How many bits are left in the byte
            int bitsLeft = 8-streamPosBit;
            if(bitsLeft > bitsToRead)
                bitsLeft = bitsToRead;
            //How many bits are we actually reading from this byte

            //Fun bitmasking stuff
            //Takes a copy of the byte in *data we're looking at and zeros out the irrelevant parts
            unsigned long long val = data[streamPosByte] & (255 >> ((8-bitsLeft)-streamPosBit));
            unsigned char mask = (255 << streamPosBit);
            val &= mask;

            //Here we see how much we then need to shift that relevant part to get it so we can just or it into our return value container
            char bitsToShift = streamPosBit - (bits-bitsToRead);
            if(bitsToShift > 0)
                val >>= bitsToShift;
            else if(bitsToShift < 0)
                val <<= -bitsToShift;

            //Increment the stream counter
            move(bitsLeft);
            bitsToRead -= bitsLeft;

            //Add whatever we extracted from the current byte to the return value
            ret += val;
        }

        return ret;
    }

    bool packet::writeUInt(const unsigned long long &value,unsigned char bits,bool debug)
    {
        //Make sure we have enough space
        if(!allocationCheck((bits/8) + 1))
            return false;

        //For each uchar in *data we'll need to write this value
        for(int bitsToWrite = bits; bitsToWrite>0; )
        {
            //Bits left in the byte in *data that the stream pointer is on
            unsigned char bitsLeft = 8-streamPosBit;

            //Normally fill to the end of the byte unless we hit the end of what we're writing
            if(bitsLeft > bitsToWrite)
                bitsLeft = bitsToWrite;

            unsigned long long ones = -1;
            unsigned long long mask = 1;
            mask <<= ((bits-bitsToWrite)+bitsLeft);
            mask--;

            //This is the value after everything after what we want to copy is zeroed out
            unsigned long long v1 = value & mask;
            unsigned long long v2 = mask;

            //This is the value after everything before what we want to copy is zeroed out
            v1 &= (ones<<(bits-bitsToWrite));
            v2 &= (ones<<(bits-bitsToWrite));

            //Before we write we have to shift the value we want to the correct position it'll have within the destination bit
            char bitsToShiftRight = (bits-bitsToWrite) - streamPosBit;
            if(bitsToShiftRight > 0)
            {
                v1 >>= bitsToShiftRight;
                v2 >>= bitsToShiftRight;
            }
            else if(bitsToShiftRight < 0)
            {
                v1 <<= -bitsToShiftRight;
                v2 <<= -bitsToShiftRight;
            }

            data[streamPosByte] |= v1;

            v1 |= ~v2;

            data[streamPosByte] &= v1;

            bitsToWrite -= bitsLeft;
            move(bitsLeft);
        }

        return true;
    }

    //Check if we need new chunks, if so allocate them
    //Needed bytes is how many we need for the next write operation if any
    //Returns true if okay to write
    bool packet::allocationCheck(unsigned int neededBytes)
    {
        //How far inside the allocated data is our stream pointer
        int dist = (allocatedChunks*syjNET_PacketChunkSize) - (streamPosByte+neededBytes);

        //If we have enough data
        if(dist > 0)
            return true;

        //The stream pointer is so far beyond the end of the data stream we're not sure if it's a good idea to allocate the space
        else if(-dist >= syjNET_PacketChunkSize*syjNET_MaxSingleAllocation)
        {
            lastError = syjError::exceedMaxAlloc;
            return false;
        }
        //Allocate more space for the next piece of data we need to write
        else
        {
            //How many chunks do we need
            int chunksToAlloc = (-dist / syjNET_PacketChunkSize)+1;

            //New data container
            unsigned char *newData = new unsigned char[(chunksToAlloc + allocatedChunks) * syjNET_PacketChunkSize];

            //Move old data over
            memcpy(newData,data,allocatedChunks * syjNET_PacketChunkSize);

            //Get rid of old data
            delete data;

            //Replace buffer
            data = newData;
            allocatedChunks += chunksToAlloc;

            return true;
        }
    }

    //Allocate a packet based on some incoming data
    //Length is in bytes
    packet::packet(unsigned char *dataIn,unsigned int length)
    {
        //Packets will be made when there is data received or intended for sending
        creationTime = SDL_GetTicks();
        critical = false;

        //Minimum amount of chunks needed to fit the incoming data
        unsigned actualLength = (length/syjNET_PacketChunkSize)+1;

        //Allocate our first chunk
        data = new unsigned char[actualLength*syjNET_PacketChunkSize];
        allocatedChunks = actualLength;

        //Old: //Most functions in syjNet get the "written to" length of a packet by it's stream pos at the time of use
        //Start at beginning for now
        streamPosBit = 0;
        streamPosByte = 0;

        //Default setting
        precision = 8;

        //No errors so far
        lastError = syjError::noError;

        //Copy over data we passed in constructor
        memcpy(data,dataIn,length);
    }

    //Set creation time and allocate first chunk
    packet::packet()
    {
        //Packets will be made when there is data received or intended for sending
        creationTime = SDL_GetTicks();
        critical = false;

        //Allocate our first chunk
        data = new unsigned char[syjNET_PacketChunkSize];
        allocatedChunks = 1;

        //Start at beginning
        streamPosBit = 0;
        streamPosByte = 0;

        //Default setting
        precision = 8;

        //No errors so far
        lastError = syjError::noError;
    }

    //Seek to a position (in bits) within the packet, returns true if within allocated space
    bool packet::seek(unsigned int bitPos)
    {
        streamPosByte = bitPos / 8;
        streamPosBit = bitPos % 8;
        return streamPosByte < allocatedChunks * syjNET_PacketChunkSize;
    }

    //Copy constructor
    packet::packet(const packet &toCopy)
    {
        //Allocate space and copy data
        data = new unsigned char[toCopy.allocatedChunks * syjNET_PacketChunkSize];
        memcpy(data,toCopy.data,toCopy.allocatedChunks * syjNET_PacketChunkSize);

        //Stuff for syjNet
        creationTime = toCopy.creationTime;
        packetID = toCopy.packetID;
        critical = toCopy.critical;
        source = toCopy.source;

        //A lot of other code uses the position of the stream pointer as the indicator of (used) size
        allocatedChunks = toCopy.allocatedChunks;
        streamPosBit = toCopy.streamPosBit;
        streamPosByte = toCopy.streamPosByte;

        //These last two probably don't need to be copied but whatever
        precision = toCopy.precision;
        lastError = toCopy.lastError;
    }

    //Tells us where we are in the stream in bits
    unsigned int packet::getStreamPos() const
    {
        return streamPosByte * 8 + streamPosBit;
    }

    //Insert a different packet inside this one at this one's stream pos
    bool packet::writePacket(packet toWrite)
    {
        //Only write until the current stream pos in the current packet (considered to be the end of useful data for most purposes)
        unsigned int bits = toWrite.getStreamPos();

        //Go back to the beginning to read, by the end we'll be at the end of the packet again if no errors thrown
        toWrite.seek(0);

        //Write all of the complete bytes of the source packet
        /*for(unsigned int a = 0; a<(bits/8); a++)
            if(!writeUChar(toWrite.readUChar()));
                return false;

        //Write the right amount of bits from the last incomplete bit (if any)
        for(unsigned int a = 0; a<(bits%8); a++)
            if(!writeBit(toWrite.readBit()))
                return false;*/

        //TODO: Fix this, get the other version working
        for(unsigned int a = 0; a<bits; a++)
            if(!writeBit(toWrite.readBit()))
                return false;

        return true;
    }

    //Returns the last error thrown the clears it
    syjError packet::getLastError()
    {
        syjError ret = lastError;
        lastError = syjError::noError;
        return ret;
    }

    //Calls getLastError and getErrorString
    std::string packet::getLastErrorString()
    {
        return getErrorString(getLastError());
    }

    //Free data
    packet::~packet()
    {
        if(data)
            delete data;
    }

    std::string packet::printBits(unsigned int bytes) const
    {
        std::string ret = "";
        unsigned int length = bytes;
        if(length == 0)
            length = (getStreamPos()+7)/8;

        for(unsigned int a = 0; a<length; a++)
        {
            for(unsigned int b = 0; b<8; b++)
                ret += data[a] & (1<<b) ? "1" : "0";

            ret += " ";
        }

        return ret;
    }

    //128 64 32 16    8 4 2 1
    //128 - 1
    //64 - 2
    //32 - 4
    //16 - 8

    std::string packet::printBytes(bool flip) const
    {
        std::string ret = "";
        /*unsigned int length = getStreamPos();*/
        unsigned int bytes = syjNET_PacketChunkSize;

        for(unsigned int a = 0; a<bytes; a++)
        {
            if(flip)
                ret += uint8tostr((unsigned int)flipBits(data[a])) + " ";
            else
                ret += uint8tostr((unsigned int)data[a]) + " ";
        }

        return ret;
    }
}



