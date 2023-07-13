#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "common.h"

namespace syj
{
    unsigned int getServerTime();

    class server;

    //This is used by the server to keep track of it's connections
    class serverClientHandle
    {
        //Handle to the server that owns us
        server *parent = 0;

        //Packets we send to clients
        UDPpacket **outgoingData = 0;

        //This will also be copied into all packets for this client
        IPaddress address;

        //An ascii representation of the IP address of the client
        std::string ipStr;

        //The ID we expect to see on all incoming packets from the client and also its position in the server's client array
        unsigned int clientID;

        //The last time we got something from the client, measured in milliseconds since the server started
        unsigned int lastActivity;

        //The packet ID of the next critical packet to be sent
        unsigned int latestPacketID;

        //When we started to fill up packetsToAcknowledge most recently
        unsigned int timeOfOldestAck;

        //Process specific types of packets
        bool processStreamPacket(packet *toProcess);
        bool processNormalPacket(packet *toProcess);
        bool processCriticalPacket(packet *toProcess);
        bool processAcknowledgePacket(packet *toProcess);
        bool processClearPacket(packet *toProcess);

        //Packet IDs of critical packets added to both lists on receipt of a critical packet
        //The first list is (usually) emptied with a call to run, the second on receipt of a clear packet
        std::vector<unsigned int> packetsToAcknowledge;
        std::vector<unsigned int> packetsToIgnore;

        //Critical packets that we are (re)sending, only deleted upon receipt of a corresponding ack packet
        std::vector<packet*> criticalPackets;

        //Packets to hopefully be sent the next tick
        std::vector<packet*> queuedPackets;

        //Send a single packet to the server instantly, only called upon connection acceptance and kicking for now, returns true on success
        bool sendPacket(packet *toSend);

        //Kick a client, calls the client method, true on success
        bool kick(unsigned int reason = syjNET_clientKickedMessage);

        //Send a few of the packets in the queuedPackets vector then delete them and resize vector
        bool sendPacketQueues(int maxPackets);

        //Adds a couple critical packets to the queued packets array (doesn't actually send packets over the internet)
        bool sendCriticalPackets(int maxPackets);

        friend class server;

        ~serverClientHandle();

        public:

        //Return ipStr
        std::string getIP() const;

        //The actual send function for the end user
        bool send(packet *toSend,bool critical);
    };

    struct server
    {
        friend class serverClientHandle;

        //Stores settings for the server
        networkingPreferences prefs;

        //The last error that was thrown
        syjError lastError;

        //Things that we figure out based on preferences
        //How many bits required for client ID
        unsigned int clientIDBits;
        //How many ids can we acknowledge in one packet
        unsigned int acksPerPacket;
        //The highest ID for any packet
        unsigned int highestPacketID;

        //Stuff for interfacing with SDLNet
        UDPsocket mainSocket;
        SDLNet_SocketSet socketSet;
        UDPpacket **incomingData;

        //Information about our clients
        serverClientHandle **clients;

        //Packets that were just received and still need to be processed
        std::vector<packet*> toProcess;

        //Called in the beginning of run, receives packets and puts them in toProcess. Returns true on success.
        bool receivePackets();

        //Process a connection request from a client
        bool processConnectionPacket(packet *toProcess);
        bool processDisconnectPacket(serverClientHandle *client,packet *toProcess);

        //Called after receivePackets, sorts packets by type then hands them to the proper client and calls more specific processing functions
        bool processPackets(unsigned int howMany);

        public:

        //Set these to the functions you want to be triggered on the proper event
        std::function<void(server *host,serverClientHandle *client)>                connectHandle;
        std::function<void(server *host,serverClientHandle *client)>                disconnectHandle;
        std::function<void(server *host,serverClientHandle *client,packet *data)>   receiveHandle;

        //How many clients are currently connected (not authoritative)
        unsigned int clientsConnected;

        //Calls client kick method and clears slot for the next client
        bool kick(serverClientHandle *client,unsigned int reason = syjNET_clientKickedMessage);

        //Receives data from clients, sends acks, responds to acks with clears, kicks inactive clients, resends what needs to be resent, etc.
        //Run once per tick in your main loop
        //Returns true on success, false if an error is thrown
        bool run();

        //Just calls the client method serverClientHandle::send, pass -1 as the ID to broadcast to all clients
        bool send(packet *toSend,bool critical,int clientID = -1);

        //Returns the last error and clears the error
        syjError getError();

        void *userData = 0;

        //Make a server and start hosting based on preferences
        server(const networkingPreferences &preferences);

        //Shutdown server and free memory
        ~server();
    };
}

#endif // SERVER_H_INCLUDED

























