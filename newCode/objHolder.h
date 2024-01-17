#ifndef OBJHOLDER_H_INCLUDED
#define OBJHOLDER_H_INCLUDED

#include "code/base/logger.h"
#include "code/base/server.h"
#include <memory>
#include <vector>
#include <algorithm>

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

//Error when accessing object out of range
//#define safeVectorAccess

//simObject::netID is this type:
#define netIDbits 32
typedef uint32_t netIDtype;

/*
    objHolder exists to make sure whenever an object is removed from the vector, it's deallocated,
    and whenever it's deallocated, it's removed from the vector.
    We maintain a vector with *every* object of its type and we know they are all valid.
*/

//The number values here are non-trivial, and are written to packets sent to clients
//These are also used for specifying what a light/emitter is attached to
#define simObjectTypeBits 4
enum simObjectType
{
    simObjectNone = 0,
    simObjectDynamic = 1,
    simObjectItem = 2,
    simObjectLight = 3,
    simObjectEmitter = 4,
    simObjectBrick = 5,
    simObjectBrickCar = 6,
    simObjectRope = 7
};

//By default, each creation packet can't possibly have more than 255 objects
//Pretty unlikely to hit this limit anyway, because packets are limited in size
const unsigned int maxObjPacketAmountBits = 8;
const unsigned int maxObjPacketAmount = (1<<maxObjPacketAmount) - 1;
#define reservedHeaderBits 32

/*
    In reality it could have just been for simObjects, but it's templated so that we can include it before the simObject declaration,
    that way simObject can friend this, and ensure the constructor stays protected, and can only ever be called by objHolder
*/

template <typename T>
class objHolder
{
    //Corresponds to template class
    simObjectType type;

    //Honestly this could *almost* be global too, but I guess our CDN could use a different server object
    syj::server *server = 0;

    //Must remain as a reference to a null pointer
    std::shared_ptr<T> noObj;

    //Only changes to increment on creation
    netIDtype lastNetID = 0;

    std::vector<std::shared_ptr<T>> allObjs;

    //Objects deleted since the last frame
    //Useful mechanic for if a lua script clears 100k bricks in a single frame
    //We wouldn't want a separate deletion packet for each brick!
    std::vector<netIDtype> recentDeletions;

    //We also bunch up creation packets, but this time we need more than just the ID so we hold ptrs to the objects
    std::vector<std::shared_ptr<T>> recentCreations;

    public:

    //Get a shared_ptr from an argument someone passed to a function in lua
    //Safe function uses netID exclusively and will return a 0 if no object with that ID is meant to exist anymore
    //Bricks in particular will suffer a performance hit each time you try and find one by netID given there's often 100,000s of them
    std::shared_ptr<T> popLuaSafe(lua_State * const L) const
    {
        syj::scope("popLuaSafe");

        if(!lua_istable(L,-1))
        {
            lua_pop(L,1);
            syj::error("No table for item argument!");
            return noObj;
        }

        lua_getfield(L, -1, "id");
        if(!lua_isinteger(L,-1))
        {
            syj::error("No ID for item argument!");
            lua_pop(L,2);
            return noObj;
        }

        int netID = lua_tointeger(L,-1);
        lua_pop(L,2);

        if(netID < 0 || netID >= lastNetID)
        {
            syj::error("Index " + std::to_string(netID) + " out of range!");
            return noObj;
        }

        return find(netID);
    }

    //Allows us to keep object constructors non-public
    template <typename... Args>
    void create(Args... args)
    {
        std::shared_ptr<T> tmp(new T(args...));
        allObjs.push_back(std::move(tmp));
        allObjs.back()->netID = lastNetID++;
        allObjs.back()->creationTime = SDL_GetTicks();
        recentCreations.push_back(allObjs.back());
    };

    //Exchange netID (i.e. from incoming client packet, lua callback) for object itself
    const inline std::shared_ptr<T> find(const netIDtype &netID) const
    {
        auto hasNetID = [&netID](const std::shared_ptr<T> &query) { return query->netID == netID; };
        auto it = std::find_if(allObjs.begin(),allObjs.end(),hasNetID);
        return it != allObjs.end() ? *it : noObj;
    }

    //How many objects of this type (in the entire program, hopefully) exist
    const inline size_t size() const{return allObjs.size();}

    //Used to delete an object you found as the result of ex. a collision callback where you just have the raw pointer
    inline void destroyByPointer(T *target)
    {
        auto hasPointer = [&target](const std::shared_ptr<T> &query) { return query.get() == target; };
        auto it = std::find_if(allObjs.begin(),allObjs.end(),hasPointer);
        if(it != allObjs.end())
        {
            recentDeletions.push_back((*it)->netID);
            allObjs.erase(it);
        }
        else
        {
            syj::scope("objHolder::destroy");
            syj::error("Pointer not found in objHolder");
        }
    }

    //Exchange netID (i.e. from incoming client packet, lua callback) for vector index
    //Honestly no clue when this would be needed
    const inline size_t findIdx(const netIDtype &netID) const
    {
        auto hasNetID = [&netID](const std::shared_ptr<T> &query) { return query->netID == netID; };
        auto it = std::find_if(allObjs.begin(),allObjs.end(),hasNetID);
        return it - allObjs.begin();
    }

    //idk why this would be used either
    inline void destroyByIdx(const size_t &idx)
    {
        #ifdef safeVectorAccess
        syj::scope("objHolder::destroyByIdx");
        if(idx >= allObjs.size())
            syj::error("SimObject destroy index out of range.");
        else
        {
        #endif
            recentDeletions.push_back(allObjs[idx]->netID);
            allObjs.erase(allObjs.begin() + idx);
        #ifdef safeVectorAccess
        }
        #endif
    }

    //Without unique_ptr it's possible some objects may exist as still being bound to some other object
    inline void destroy(std::shared_ptr<T> &idx)
    {
        //You know, it's still possible if e.g. a brick has a ptr to a light or emitter it will live on
        //Hopefully in those sorts of cases the "brick" is the one calling this somehow
        auto it = std::find(allObjs.begin(),allObjs.end(),idx);
        if(it != allObjs.end())
        {
            recentDeletions.push_back((*it)->netID);
            allObjs.erase(it);
            idx.reset();
        }
        else
        {
            syj::scope("objHolder::destroy");
            syj::error("Pointer not found in objHolder");
        }
    }

    //Principle way of getting physics objects, outside of lua scripts and client packets that use netIDs
    inline const std::shared_ptr<T>& operator[](const std::size_t &idx) const
    {
        #ifdef safeVectorAccess
        syj::scope("objHolder[]");
        if(idx >= allObjs.size())
        {
            syj::error("SimObject index out of range.");
            return noObj;
        }
        else
        #endif
            return allObjs[idx];
    }

    //A few functions will cause a crash if there's no valid server object passed
    objHolder(simObjectType _type) : type(_type) //,syj::server * const _server) : type(_type) , server(_server)
    {
        syj::scope("objHolder::objHolder");

        if(_type == simObjectNone)
            syj::error("simObjectType invalid!");
        /*if(!_server)
            syj::error("Invalid server object!");*/
    };

    void setServer(syj::server * _server)
    {
        syj::scope("objHolder::setServer");

        if(!_server)
            syj::error("Invalid server object!");

        server = _server;
    }

    ~objHolder()
    {

    }

    //Performed when a new client connects to the game
    void sendAllToClient(syj::serverClientHandle * const target) const
    {
        unsigned int toSend = size();

        if(toSend == 0)
            return;

        unsigned int sentSoFar = 0;

        //syj::server doesn't break up packets for us, and in fact will sigsev if a packet is too big
        //How big? No clue, but packetMTUbits is the limit for now. Use multiple packets if required
        while(toSend > 0)
        {
            unsigned int sentInCurrentPacket = 0;
            //30 is arbitrary but should leave enough room for the packet type and header
            int bitsLeft = packetMTUbits - reservedHeaderBits;

            //Figure out how many objects we can fit in one packet
            do
            {
                bitsLeft -= allObjs[sentSoFar + sentInCurrentPacket]->getCreationPacketBits();
                sentInCurrentPacket++;

            }   while(
                      bitsLeft > 0 &&
                      (sentInCurrentPacket + sentSoFar < toSend) &&
                      sentInCurrentPacket < maxObjPacketAmount);


            //Actually construct the packet
            syj::packet currentPacket;
            currentPacket.writeUInt(packetType_addSimObject,packetTypeBits);
            currentPacket.writeUInt(type,simObjectTypeBits);
            currentPacket.writeUInt(sentInCurrentPacket,maxObjPacketAmountBits);
            for(int a = sentSoFar; a<sentSoFar+sentInCurrentPacket; a++)
                allObjs[a]->addToCreationPacket(&currentPacket);
            target->send(&currentPacket,true);

            toSend -= sentInCurrentPacket;
            sentSoFar += sentInCurrentPacket;
        }
    }

    //Send a list of IDs of things that have been deleted since the last time this function was called
    void sendRecentDeletions()
    {
        unsigned int toSend = recentDeletions.size();

        if(toSend == 0)
            return;

        unsigned int sentSoFar = 0;

        while(toSend > 0)
        {
            //30 is still kinda arbitrary, see above
            int bitsLeft = packetMTUbits - reservedHeaderBits;

            //To delete we just send the ID, which is always the same size (4 bytes)
            unsigned int sentInCurrentPacket = bitsLeft / netIDbits;
            sentInCurrentPacket = std::min(sentInCurrentPacket,maxObjPacketAmountBits);
            sentInCurrentPacket = std::min(sentInCurrentPacket,toSend);

            //This packet is just a list of IDs of things to delete of a given type
            syj::packet currentPacket;
            currentPacket.writeUInt(packetType_removeSimObject,packetTypeBits);
            currentPacket.writeUInt(type,simObjectTypeBits);
            currentPacket.writeUInt(sentInCurrentPacket,maxObjPacketAmountBits);
            for(int a = sentSoFar; a<sentSoFar+sentInCurrentPacket; a++)
                currentPacket.writeUInt(recentDeletions[a],netIDbits);
            server->send(&currentPacket,true);

            toSend -= sentInCurrentPacket;
            sentSoFar += sentInCurrentPacket;
        }

        recentDeletions.clear();
    }

    //Send creations since the last frame
    //Code is largely a copy of sendAllToClient with different packet target and source vector
    void sendRecentCreations()
    {
        if(recentCreations.size() == 0)
            return;

        unsigned int toSend = recentCreations.size();

        unsigned int sentSoFar = 0;

        //syj::server doesn't break up packets for us, and in fact will sigsev if a packet is too big
        //How big? No clue, but packetMTUbits is the limit for now. Use multiple packets if required
        while(toSend > 0)
        {
            unsigned int sentInCurrentPacket = 0;
            //30 is arbitrary but should leave enough room for the packet type and header
            int bitsLeft = packetMTUbits - reservedHeaderBits;

            //Figure out how many objects we can fit in one packet
            do
            {
                bitsLeft -= recentCreations[sentSoFar + sentInCurrentPacket]->getCreationPacketBits();
                sentInCurrentPacket++;

            }   while(
                      bitsLeft > 0 &&
                      (sentInCurrentPacket + sentSoFar < toSend) &&
                      sentInCurrentPacket < maxObjPacketAmount);


            //Actually construct the packet
            syj::packet currentPacket;
            currentPacket.writeUInt(packetType_addSimObject,packetTypeBits);
            currentPacket.writeUInt(type,simObjectTypeBits);
            currentPacket.writeUInt(sentInCurrentPacket,maxObjPacketAmountBits);
            for(int a = sentSoFar; a<sentSoFar+sentInCurrentPacket; a++)
                recentCreations[a]->addToCreationPacket(&currentPacket);
            server->send(&currentPacket,true);

            toSend -= sentInCurrentPacket;
            sentSoFar += sentInCurrentPacket;
        }

        recentCreations.clear();
    }

    //Performed each frame or possibly each few frames, send updates since last call to everyone
    void sendQueuedUpdates() const
    {
        unsigned int toCheck = allObjs.size();

        if(toCheck == 0)
            return;

        unsigned int checkedSoFar = 0;

        while(toCheck > 0)
        {
            //More space is reserved (62 bits) because we also send the server time with each update packet
            int bitsLeft = packetMTUbits - (reservedHeaderBits + sizeof(unsigned int));

            int sentThisPacket = 0;
            int skippedThisPacket = 0;

            //Objects are supposed to set requiresUpdate = true whenever they want something sent to clients
            do
            {
                int idx = sentThisPacket + skippedThisPacket + checkedSoFar;
                if(allObjs[idx]->requiresNetUpdate())
                {
                    bitsLeft -= allObjs[idx]->getUpdatePacketBits();
                    sentThisPacket++;
                }
                else
                    skippedThisPacket++;

            } while(
                    bitsLeft > 0 &&
                    (sentThisPacket + skippedThisPacket + checkedSoFar < toCheck) &&
                    sentThisPacket < maxObjPacketAmount);

            syj::packet currentPacket;
            currentPacket.writeUInt(packetType_updateSimObject,packetTypeBits);
            currentPacket.writeUInt(type,simObjectTypeBits);

            //The time is used for snapshot interpolation of physics objects
            //It's not really needed for anything other than dynamics and items at the moment
            currentPacket.writeUInt(syj::getServerTime(),sizeof(unsigned int));
            for(int a = checkedSoFar; a < checkedSoFar + sentThisPacket + skippedThisPacket; a++)
            {
                if(allObjs[a]->requiresNetUpdate())
                    allObjs[a]->addToUpdatePacket(&currentPacket);
            }

            //Dynamic/item update packets (their transforms) can be dropped and its not a big deal
            //Other update packets (like a brick color change) are rarer and require verification of receipt
            server->send(&currentPacket,!(type == simObjectDynamic || type == simObjectItem || type == simObjectRope));

            checkedSoFar += skippedThisPacket + sentThisPacket;
            toCheck -= skippedThisPacket + sentThisPacket;
        }
    }
};

#endif // OBJHOLDER_H_INCLUDED










