#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

//We return the string as an LPS
#include "lengthPrefixedString.h"
//We need to include this to return SDL errors
#include "SDL2/SDL_net.h"

namespace syj
{
    //Handy error class
    enum class syjError
    {
        //All clear
        noError,
        //Trying to read data from a packet when there's no more data
        endOfStream,
        //A write operation on a packet would require the packet to allocate more than syjNET_MaxSingleAllocation chunks at once
        exceedMaxAlloc,
        //A preference passed to create an object was out of range
        badPreference,
        //SDLNet threw an error
        SDLError,
        //SDLNet said there was a socket ready to receive but it did not receive anything
        readyButNoData,
        //An argument passed was invalid (invalid client, invalid packet, etc.)
        badArgument,
        //Someone sent us a packet that didn't follow proper syjNet protocol
        improperPacket,
        //An SDL send function did not send a UDPpacket successfully
        didNotSend,
        //Couldn't connect to the server
        noConnection,
        //Server was full
        serverFull
    };

    //Helper function to give info about an error that was thrown
    lengthPrefixedString getErrorString(syjError error);
}


#endif // ERRORS_H_INCLUDED
