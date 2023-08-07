#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

//Used in server and client
#include <vector>
//Used to convert IP to string in server
#include <sstream>
//Function handles for users to listen to connections, disconnections, data received etc.
#include <functional>
//These strings are good for putting into our packets
#include "lengthPrefixedString.h"
//A nice lightweight and cross-platform networking library
#include <SDL2/SDL_net.h>
//Helpful error struct
#include "errors.h"
//For std::remove
#include <algorithm>
//A nice container for our packets with easy methods to extract and write binary data
#include "packet.h"
//Code for the server
//#include "server.h"


#include <chrono>
using namespace std::chrono;


//How much data do we allocate at a time for our packets
#define syjNET_PacketChunkSize      100
//How many chunks can a packet allocate in one write operation
#define syjNET_MaxSingleAllocation  10
//32 bit magic number sent upon being kicked because the server was full
#define syjNET_serverFullMessage    14881234
//32 bit magic number sent by the client to request a connection
#define syjNET_clientConnectMessage 89478485
//32 bit magic number sent by the server when it accepts a connection request
#define syjNET_acceptClientMessage  15001500
//What we send to a client to let them know they were intentionally kicked by host
#define syjNET_clientKickedMessage  87654321
//What we send to a client to let them know they were kicked for inactivity
#define syjNET_timedOutMessage      22334411
//What we send to a client to let them know they were kicked because another IP sent a packet with their user ID
#define syjNET_compromisedMessage   15015015
//What a client sends when they want to disconnect
#define syjNET_disconnectMessage    196242202

#define packetMTUbits 11200

#define packetTypeBits 5
#define packetTypeBitsFromClient 4

#define packetType_connectionRepsonse 0
//#define packetType_addDynamic 1
#define packetType_addRemoveDynamic 1
#define packetType_addDynamicType 2
#define packetType_addSpecialBrickType 3
#define packetType_cameraDetails 4
#define packetType_addBricks 5
#define packetType_skipBricksCompile 6
#define packetType_updateDynamicTransforms 7
//#define packetType_removeDynamic 8
#define packetType_addRemoveLight 8
#define packetType_removeBrick 9
#define packetType_addBricksToBuiltCar 10
#define packetType_luaPasswordResponse 11
#define packetType_setColorPalette 12
#define packetType_clientPhysicsData 13
//#define packetType_clientLuaConsoleText 13
//#define packetType_addGUIText 14
#define packetType_ping 14
#define packetType_removeBrickVehicle 15
#define packetType_setShapeName 16
#define packetType_addItemType 17
#define packetType_addRemoveItem 18
//#define packetType_addItem 18
#define packetType_addSoundType 19
#define packetType_playSound 20
#define packetType_forceWrenchDialog 21
#define packetType_setInventory 22
#define packetType_addOrRemovePlayer 23
#define packetType_updateBrick 24
#define packetType_debugLocs 25
//#define packetType_removeItem 26
#define packetType_addRemoveRope 26
#define packetType_addMessage 27
#define packetType_setNodeColors 28
#define packetType_waterOrDecal 29
#define packetType_newEmitterParticleType 30
#define packetType_emitterAddRemove 31

//This used to be in server.h until I decided to reuse it for the client
namespace syj
{
    //This structure is meant to be passed to the constructor of syj::server and syj::client
    struct networkingPreferences
    {
        //How many clients our server can handle at once, 1-255
        //Only used in server
        unsigned int maxClients = 32;
        //What port to host on, 1-65535 or what port to connect to (for client)
        unsigned short port = 20000;
        //How many packets we should try to receive per tick
        unsigned char maxPackets = 100;
        //How many bytes can a packet hold
        unsigned short maxPacketSize = 1500;
        //How many bits to use in critical packet IDs
        //Only needs to be set for server, otherwise it gets the value from the server being connected to
        unsigned char criticalIDBits = 14;
        //How long to wait before kicking clients for inactivity, or how long to try and wait for a response from server
        unsigned int timeoutMS = 30000;
        //How long to wait minimum before resending a critical packet
        //Baseline, later on it'll be decreased dynamically based on how much data is being sent
        unsigned int packetResendMS = 5000;
    };
}

#endif // COMMON_H_INCLUDED
