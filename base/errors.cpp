#include "errors.h"

namespace syj
{
    //Helper function to give info about an error that was thrown
    lengthPrefixedString getErrorString(syjError error)
    {
        switch(error)
        {
            case syjError::noError:         return "No error occurred.";
            case syjError::endOfStream:     return "Attempt to read from beyond the end of a stream of data.";
            case syjError::exceedMaxAlloc:  return "A write operation on a packet would require it to allocate more than syjNET_MaxSingleAllocation chunks.";
            case syjError::badPreference:   return "A preference passed to create an object was out of range.";
            case syjError::SDLError:        return "SDL Threw the following error: " + std::string(SDLNet_GetError());
            case syjError::readyButNoData:  return "SDLNet said a socket was ready to receive data but it was not: " + std::string(SDLNet_GetError());
            case syjError::badArgument:     return "An argument passed to a function was invalid. (Missing packet, missing client, etc.)";
            case syjError::improperPacket:  return "We were sent a packet that did not follow syjNet protocol.";
            case syjError::didNotSend:      return "An SDL send call did not send a UDP packet successfully: " + std::string(SDLNet_GetError());
            case syjError::noConnection:    return "Could not connect to server!";
            case syjError::serverFull:      return "Server was full.";
            default: return "syjError not located in enum. (shouldn't happen codeblocks made me put this here";
        }
    }
}
