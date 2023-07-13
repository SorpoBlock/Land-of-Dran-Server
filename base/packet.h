#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED

#include "common.h"

namespace syj
{
    //Just a way to pass precision settings through the stream operator <<
    //E.g. myPacket<<packetPrecision(16)<<15000; would be the same as myPacket.writeUInt(15000,16);
    struct packetPrecision
    {
        unsigned int value = 8;
        packetPrecision(unsigned int _value) : value(_value) {};
    };

    struct packet
    {
        //private:

        //What was the last error that was thrown
        syjError lastError = syjError::noError;

        //The actual data of our packet
        unsigned char *data = 0;

        //How many bytes (allocateChunks * syjNET_PacketChunkSize) have been allocated in *data
        int allocatedChunks = 0;

        //Where are we writing to or reading from next in bits (streamPos = streamPosByte*8 + streamPosBit)
        char streamPosBit = 0;

        //Where are we writing to or reading from next in bytes (streamPos = streamPosByte*8 + streamPosBit)
        int streamPosByte = 0;

        //See where our streamPos iter is and allocate data until the streamPos is inside a chunk
        //Needed bytes is how many we need for the next write operation if any
        //Returns true if okay to write
        bool allocationCheck(unsigned int neededBytes = 0);

        //How many bytes will the next stream operator use to write an integer
        unsigned int precision = 8;

        public:

        //These are just stream operator packages for the regular write functions
        packet &operator<<(bool bit);
        packet &operator<<(unsigned long value);
        packet &operator<<(const lengthPrefixedString &string);
        packet &operator<<(packetPrecision _precision);

        //Go to a position in bits, returns true if within allocated space
        bool seek(unsigned int bitPos);

        //Move the stream pointer along a certain number of bits
        bool move(int bitPos);

        //Tells us where we are in the stream in bits
        unsigned int getStreamPos() const;

        //Gets the last error the clears it
        syjError getLastError();

        //Calls getLastError and getErrorString
        std::string getLastErrorString();

        //Copies an LPS into the packet
        bool writeString(const lengthPrefixedString &string);

        //Write a single bit to the stream, returns true on success
        bool writeBit(const bool &value,bool allocCheck = true);

        //Read a single bit from the stream
        bool readBit();

        //Write another packet to this packet starting at this packets current stream pos
        bool writePacket(packet toWrite);

        //Copy the next bit of data into an LPS
        lengthPrefixedString readString();

        //Write a single byte
        bool writeUChar(const unsigned char &value,bool allocCheck = true);

        //Read a single byte
        unsigned char readUChar();

        //Write a single integer, true on success
        bool writeUInt(const unsigned long long &value,unsigned char bits,bool debug=false);

        //Copy a float to 4 uChars and write
        bool writeFloat(float in);

        //Read a float from 4 uChars and return
        float readFloat();

        //Read a single integer
        unsigned long long readUInt(unsigned char bits,bool debug=false);

        //The creation time for packets to send, the received time for packets received
        unsigned int creationTime = 0;

        //Set this to the source of a received packet, should be empty for a packet to send out
        IPaddress source;

        //Was this packet originally received as a critical packet
        bool critical = false;

        //Only used for critical packets and only on the senders side
        unsigned int packetID;

        //Allocate first chunk of data and set creation time
        packet();

        //Allocate a packet based on some incoming data
        //Length is in bytes
        packet(unsigned char *dataIn,unsigned int length);

        //Copy constructor
        packet(const packet &toCopy);

        //Free data
        ~packet();

        std::string printBits(unsigned int numBytes = 0) const;

        std::string printBytes(bool flip = false) const;
    };
}

#endif // PACKET_H_INCLUDED
