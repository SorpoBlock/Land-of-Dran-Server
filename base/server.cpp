#include "server.h"

namespace syj
{
    unsigned int getServerTime() { return duration_cast< milliseconds >(steady_clock::now().time_since_epoch()).count() % 1000000000; }

    //Returns the last error and clears the error
    syjError server::getError()
    {
        syjError ret = lastError;
        lastError = syjError::noError;
        return ret;
    }

    //Send a packet exactly as it is and instantly, does not delete packet, true on success
    bool serverClientHandle::sendPacket(packet *toSend)
    {
        //Just use the last packet (we allocated an extra for this reason)
        UDPpacket *storage = outgoingData[parent->prefs.maxPackets];
        //Assuming the packet was just written to and stream pos is at the end of useful data
        storage->len = (toSend->getStreamPos()+7)/8;
        //Copy over the actual data
        memcpy(storage->data,toSend->data,storage->len);
        //Send the packet
        if(SDLNet_UDP_Send(parent->mainSocket,-1,storage) != 1)
        {
            parent->lastError = syjError::didNotSend;
            return false;
        }
        return true;
    }

    serverClientHandle::~serverClientHandle()
    {
        //TODO: This needs to be freed but is now causing a crash
        //if(outgoingData)
          //  SDLNet_FreePacketV(outgoingData);
        for(unsigned int a = 0; a<criticalPackets.size(); a++)
            if(criticalPackets[a])
                delete criticalPackets[a];
        for(unsigned int a = 0; a<queuedPackets.size(); a++)
            if(queuedPackets[a])
                delete queuedPackets[a];
    }

    //What do we do if a client wants to connect
    bool server::processConnectionPacket(packet *toProcess)
    {
        //Check and see if we have any free client slots
        int freeSlot = -1;
        for(unsigned int a = 0; a<prefs.maxClients; a++)
        {
            if(!clients[a])
            {
                freeSlot = a;
                break;
            }
        }

        //If the server was full
        if(freeSlot == -1)
        {
            //We'll just use the last packet in the incoming packet array (we allocated an extra)
            UDPpacket *toSendOut = incomingData[prefs.maxPackets];

            //The entire message is just the 32bit magic number that indicates a full server
            toSendOut->len = 4;
            unsigned int disconnectMagicNumber = syjNET_serverFullMessage;
            memcpy(toSendOut->data,&disconnectMagicNumber,toSendOut->len);

            //Send the packet to the potential client
            toSendOut->address = toProcess->source;
            if(SDLNet_UDP_Send(mainSocket,-1,toSendOut) == 0)
            {
                lastError = syjError::didNotSend;
                delete toProcess;
                toProcess = 0;
                return false;
            }

            delete toProcess;
            toProcess = 0;
            return true;
        }

        //Create a new client in the first open slot
        clientsConnected++;
        serverClientHandle *newClient = new serverClientHandle;
        clients[freeSlot] = newClient;

        //Setting up client
        newClient->timeOfOldestAck = 0;
        newClient->clientID = freeSlot;
        newClient->address = toProcess->source;
        newClient->lastActivity = SDL_GetTicks();
        newClient->parent = this;
        newClient->latestPacketID = 0;

        //Convert IP from 32bit uint to string for later easy access
        unsigned char qa = newClient->address.host>>24;
        unsigned char qb = (newClient->address.host>>16)&0xff;
        unsigned char qc = (newClient->address.host>>8)&0xff;
        unsigned char qd = newClient->address.host&0xff;
        std::stringstream ret;
        ret<<(unsigned int)qd<<"."<<(unsigned int)qc<<"."<<(unsigned int)qb<<"."<<(unsigned int)qa;
        newClient->ipStr = ret.str();

        //Allocate an array of packets just for this client
        newClient->outgoingData = SDLNet_AllocPacketV(prefs.maxPackets+1,prefs.maxPacketSize);
        //std::cout<<"Packet allocated: "<<newClient->outgoingData<<"\n";
        if(!newClient->outgoingData)
        {
            lastError = syjError::SDLError;
            delete toProcess;
            toProcess = 0;
            return false;
        }

        //Assign the client's IP to all of the packets ahead of time
        for(int a = 0; a<prefs.maxPackets+1; a++)
        {
            newClient->outgoingData[a]->address = newClient->address;
            newClient->outgoingData[a]->channel = -1;
        }

        //Start making the acceptance packet
        packet *acceptancePacket = new packet;
        //Magic number indicate acceptance
        acceptancePacket->writeUInt(syjNET_acceptClientMessage,32);
        //How many bits do we use to represent the clientID
        acceptancePacket->writeUInt(clientIDBits,8);
        //What is your clientID
        acceptancePacket->writeUInt(freeSlot,clientIDBits);
        //How many bits used to represent critical packet IDs
        acceptancePacket->writeUInt(prefs.criticalIDBits,8);

        //Send and delete
        newClient->sendPacket(acceptancePacket);
        delete acceptancePacket;

        if(connectHandle)
            connectHandle(this,newClient);

        delete toProcess;
        toProcess = 0;
        return true;
    }

    bool serverClientHandle::kick(unsigned int reason)
    {
        bool retValue = true;

        if(parent->disconnectHandle)
            parent->disconnectHandle(parent,this);

        //std::cout<<"SYJNET debug after handle!\n";

        //0 means don't send a kick message (client already disconnected)
        //syjNET_clientKickedMessage is the default kick reason (kicked by server owner)
        //std::cout<<"SYJNET reason "<<reason<<"\n";
        if(reason != 0)
        {
            //Make a kick packet
            packet *kickPacket = new packet;

            //Kick packets start with 5 true bits 0b11111
            kickPacket->writeBit(true);
            kickPacket->writeBit(true);
            kickPacket->writeBit(true);
            kickPacket->writeBit(true);
            kickPacket->writeBit(true);

            //Give the reason so the client knows it wasn't just some odd error
            kickPacket->writeUInt(reason,32);

            if(!sendPacket(kickPacket))
                retValue = false;

            //std::cout<<"SYJNET sent kick packet!\n";
        }

        //Clear things allocated to the client
        //Never mind I just gave serverClientHandle a dstor
        /*if(outgoingData)
            SDLNet_FreePacketV(outgoingData);

        for(unsigned int a = 0; a<queuedPackets.size(); a++)
        {
            delete queuedPackets[a];
            queuedPackets[a] = 0;
        }*/

        return retValue;
    }

    //Calls client kick method and clears slot for the next client
    bool server::kick(serverClientHandle *client,unsigned int reason)
    {
        if(!client)
        {
            lastError = syjError::badArgument;
            return false;
        }

        //Send the kick packet and clear client held data structures
        bool retValue = true;
        retValue = client->kick(reason);

        //std::cout<<"SYJNET about to clear slot\n";

        //Even if that failed we still gotta clear the client's slot
        //std::cout<<"Deleting: "<<clients[client->clientID]<<" == "<<client<<" ID: " <<client->clientID<<"\n";
        clients[client->clientID] = 0;
        delete client;
        clientsConnected--;

        //std::cout<<"SYJNET cleared slot\n";

        return retValue;
    }

    //What happens when a client initiates a disconnect
    bool server::processDisconnectPacket(serverClientHandle *client,packet *toProcess)
    {
        if(toProcess->readUInt(32) != syjNET_disconnectMessage)
            lastError = syjError::improperPacket;
        else
            kick(client,0);
        delete toProcess;
        toProcess = 0;
        return true;
    }

    bool server::send(packet *toSend,bool critical,int clientID)
    {
        bool returnValue = true;

        //client ID of -1 means send to all clients
        for(unsigned int a = 0; a<prefs.maxClients; a++)
        {
            if(!clients[a])
                continue;

            if(clientID == -1 || (unsigned)clientID == clients[a]->clientID)
            {
                if(!clients[a]->send(toSend,critical))
                    returnValue = false;
            }
        }

        return returnValue;
    }

    //The actual send function for the end user
    bool serverClientHandle::send(packet *toSend,bool critical)
    {
        bool bad = toSend->getStreamPos() < 1;

        if(bad)
        {
            std::cout<<"serverClientHandle::send - Attempted to send size 0 packet!\n";
            return true;
        }

        packet *encapsulatedPacket = new packet;
        encapsulatedPacket->writeBit(true);

        if(critical)
        {
            encapsulatedPacket->writeBit(true);  //Critical packets start with 0b110
            encapsulatedPacket->writeBit(false);

            //Write the critical packet ID
            encapsulatedPacket->writeUInt(latestPacketID,parent->prefs.criticalIDBits);
            encapsulatedPacket->packetID = latestPacketID;
            //Let sender be able to read what packetID their packet got sent with
            toSend->packetID = latestPacketID;

            //Increment packet ID for next packet
            latestPacketID++;
            if(latestPacketID+1 >= parent->highestPacketID)
                latestPacketID = 0;

            //Copy over data to send
            encapsulatedPacket->writePacket(*toSend);

            //This means it won't be ignored until the packet resend MS cooldown is up
            encapsulatedPacket->creationTime = 0;

            //Add to critical packets list, who knows when it'll actually be sent
            encapsulatedPacket->critical = true;
            criticalPackets.push_back(encapsulatedPacket);
        }
        else
        {
            encapsulatedPacket->writeBit(false); //Normal packets start with 0b10
            encapsulatedPacket->writePacket(*toSend); //Copy packet data to send over

            encapsulatedPacket->critical = false;
            queuedPackets.push_back(encapsulatedPacket); //Add to immediate send out list
        }

        return true;
    }

    bool serverClientHandle::processStreamPacket(packet *toProcess)
    {
        //TODO: Write this
        delete toProcess;
        toProcess = 0;
        return true;
    }

    //The simplest packet type. Just a packet type header (0b10) and the data itself.
    bool serverClientHandle::processNormalPacket(packet *toProcess)
    {
        //There shouldn't be much to do here, just pass the packet to the end user
        parent->receiveHandle(parent,this,toProcess);

        delete toProcess;
        toProcess = 0;
        return true;
    }

    bool serverClientHandle::processCriticalPacket(packet *toProcess)
    {
        //Get the packet ID
        unsigned int packetID = toProcess->readUInt(parent->prefs.criticalIDBits);

        //Record how old the current iteration of the stack in packetsToAcknowledge is
        packetsToAcknowledge.push_back(packetID);
        if(packetsToAcknowledge.size() == 1 || timeOfOldestAck == 0)
            timeOfOldestAck = SDL_GetTicks();

        //Make sure it's not on the ignore list (the client will try and resend unacknowledged packets) we don't want duplicates
        for(unsigned int a = 0; a<packetsToIgnore.size(); a++)
        {
            if(packetsToIgnore[a] == packetID)
            {
                delete toProcess;
                toProcess = 0;
                return true;
            }
        }

        //Add to acknowledge and ignore lists
        packetsToIgnore.push_back(packetID);

        //Pass packet onto end user
        toProcess->critical = true;
        toProcess->packetID = packetID;

        if(parent->receiveHandle)
            parent->receiveHandle(parent,this,toProcess);

        delete toProcess;
        toProcess = 0;

        return true;
    }

    bool serverClientHandle::sendCriticalPackets(int maxPackets)
    {
        //If we have any critical packets in queue
        int packetsToSend = criticalPackets.size();
        if(packetsToSend > 0)
        {
            //Cap it in case of network issues
            if(packetsToSend > maxPackets)
                packetsToSend = maxPackets;

            //We're figuring out which packets haven't been resent in the longest period of time
            std::vector<packet*> oldestPackets;
            for(int a = 0; a<packetsToSend; a++)
            {
                //Shouldn't be an issue but just in case
                if(!criticalPackets[a])
                    continue;

                //std::cout<<criticalPackets[a]<<" aka "<<criticalPackets[a]->packetID<<" Comp: "<<SDL_GetTicks()<<"-"<<criticalPackets[a]->creationTime<<"<"<<parent->prefs.packetResendMS;
                //No matter what never resend a packet if it it's been less than packetResendMS milliseconds since its last send
                if(SDL_GetTicks() - criticalPackets[a]->creationTime < parent->prefs.packetResendMS)
                {
                   // std::cout<<"no\n";
                    continue;
                }
                //std::cout<<"yes\n";

                //If there's not enough packets in the vector yet, just add the next packet
                //TODO: Signed?
                if((signed)oldestPackets.size() < maxPackets)
                {
                    //std::cout<<"Add "<<criticalPackets[a]<<" aka "<<criticalPackets[a]->packetID<<"\n";
                    oldestPackets.push_back(criticalPackets[a]);
                    continue;
                }

                //sendCriticalPackets is at worst an O(10*n) function with respect to amount of queued critical packets. Neat.
                //TODO: Actual sorting algorithm?
                for(unsigned int b = 0; b<oldestPackets.size(); b++)
                {
                    //If one of the packets in our little vector is newest (higher creation time) replace it
                    if(oldestPackets[b]->creationTime > criticalPackets[a]->creationTime)
                    {
                        //std::cout<<"Replace ("<<b<<") "<<oldestPackets[b]->packetID<<" with ("<<a<<") "<<criticalPackets[a]->packetID<<"\n";
                        oldestPackets[b] = criticalPackets[a];
                        break;
                    }
                }
            }

            for(unsigned int a = 0; a<oldestPackets.size(); a++)
            {
                //std::cout<<"Sen"<<criticalPackets[a]<<" aka "<<criticalPackets[a]->packetID<<"|"<<SDL_GetTicks()-criticalPackets[a]->creationTime<<"\n";
                //Update last send time
                oldestPackets[a]->creationTime = SDL_GetTicks();

                //Push it back to the queued packets vector, this one actually sends the packets
                //TODO: Copy it or just rewrite sendPacketQueues to not delete critical packets hmm
                queuedPackets.push_back(new packet(*oldestPackets[a]));
            }
        }

        //TODO: How would this function mess up and throw an error?
        return true;
    }

    bool serverClientHandle::processAcknowledgePacket(packet *toProcess)
    {
        //How many packets to clear from criticalPackets
        unsigned int toAcknowledge = toProcess->readUInt(10);

        //Like processClearPacket, if it's telling us to acknowledge 0 packets that's probably an error on their end
        if(toAcknowledge == 0)
        {
            parent->lastError = syjError::improperPacket;
            delete toProcess;
            toProcess = 0;
            return false;
        }

        //Respond to an acknowledge packet with a clear (acknowledgment of acknowledgment) packet
        packet *clearPacket = new packet;
        //The header for a clear packet
        clearPacket->writeBit(true);
        clearPacket->writeBit(true);
        clearPacket->writeBit(true);
        clearPacket->writeBit(true);
        clearPacket->writeBit(false);

        //It's just a number by number copy of the packet the server send us
        //TODO: Just copy the entire data stream directly and at once?
        clearPacket->writeUInt(toAcknowledge,10);

        for(unsigned int a = 0; a<toAcknowledge; a++)
        {
            //What's the actual ID of the packet to acknowledge
            unsigned int theID = toProcess->readUInt(parent->prefs.criticalIDBits);

            //Tell the server to clear each packet they told us to acknowledge
            clearPacket->writeUInt(theID,parent->prefs.criticalIDBits);

            //Search for the packet and see if we're still trying to resend it, if so, delete
            for(unsigned int b=0; b<criticalPackets.size(); b++)
            {
                if(criticalPackets[b] == 0)
                    continue;
                if(criticalPackets[b]->packetID == theID)
                {
                    delete criticalPackets[b];
                    criticalPackets[b] = 0;
                    break;
                }
            }
        }

        queuedPackets.push_back(clearPacket);

        delete toProcess;
        toProcess = 0;
        return true;
    }

    bool serverClientHandle::processClearPacket(packet *toProcess)
    {
        //How many ids to clear from the ignore list
        unsigned int toClear = toProcess->readUInt(10);

        //Well I mean there's gotta be one id or else what was the point of the packet
        if(toClear == 0)
        {
            //That's an error on the senders end
            parent->lastError = syjError::improperPacket;
            delete toProcess;
            toProcess = 0;
            return false;
        }

        for(unsigned int a = 0; a<toClear; a++)
        {
            //Read the actual ID off the list of ids that makes up the rest of the packet
            unsigned int theID = toProcess->readUInt(parent->prefs.criticalIDBits);

            //Push the ids that match and are to clear to the end of the packet
            auto it = remove(packetsToIgnore.begin(),packetsToIgnore.end(),theID);
            //If there was at least one ID pushed to the end of the packet, resize the vector to omit those ids
            if(it != packetsToIgnore.end())
                packetsToIgnore.erase(it,packetsToIgnore.end());
        }

        delete toProcess;
        toProcess = 0;
        return true;
    }

    //Called after receivePackets, sorts packets by type then hands them to the proper client and calls more specific processing functions
    bool server::processPackets(unsigned int howMany)
    {
        bool returnValue = true;

        //How many packets do we have to process?
        if(howMany > toProcess.size())
            howMany = toProcess.size();
        if(howMany == 0)
            return returnValue;

        for(unsigned int a = 0; a<howMany; a++)
        {
            packet *current = toProcess[a];
            //We should always have a packet here but just in case
            if(!current)
                continue;
            current->seek(0);

            //If our packet has the magic number that indicates a connection request
            if(current->readUInt(32) == syjNET_clientConnectMessage)
            {
                if(!processConnectionPacket(current))
                    returnValue = false;
                continue;
            }

            //Go back to start of packet if we didn't see the magic number
            current->seek(0);

            //All other packets should start with a valid clientID
            unsigned int clientID = current->readUInt(clientIDBits);

            //Does the clientID point to a valid client?
            if(clientID >= prefs.maxClients)
            {
                //If not, delete the packet here as there is nothing to be processed
                delete current;
                current = 0;
                continue;
            }

            //Same as above
            serverClientHandle *client = clients[clientID];
            //std::cout<<"Client ID: "<<clientID<<" Handle: "<<client<<"\n";
            if(!client)
            {
                delete current;
                current = 0;
                continue;
            }

            //We got a packet with a certain user ID but not the same IP they connected with, just kick them to be safe
            /*if(client->address.host != current->source.host)
            {
                //std::cout<<"Kicking because multiclient...\n";
                kick(client,syjNET_compromisedMessage);
                delete current;
                current = 0;
                continue;
            }*/
            //std::cout<<"again Client ID: "<<clientID<<" Handle: "<<client<<"\n";

            //This keeps the clients from timing out
            if(client->lastActivity < current->creationTime)
                client->lastActivity = current->creationTime;

            //The packet's type header is basically a linked list of sorts
            //Most common packet types need fewer bits, rarer packet types use more bits

            if(!current->readBit())
            {
                if(!client->processStreamPacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!client->processNormalPacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!client->processCriticalPacket(current))
                    returnValue = false;
                continue;
            }

            if(!current->readBit())
            {
                if(!client->processAcknowledgePacket(current))
                    returnValue = false;
                continue;
            }

            //The packet type specification won't be longer than 5 bits ever
            if(!current->readBit())
            {
                if(!client->processClearPacket(current))
                    returnValue = false;
                continue;
            }
            else
            {
                if(!processDisconnectPacket(client,current))
                    returnValue = false;
                continue;
            }
        }

        //Remove processed packets from the queue but note that this does not delete the, that's done in type specific processing functions
        toProcess.erase(toProcess.begin(),toProcess.begin() + howMany);

        return returnValue;
    }

    //Called in the beginning of run, receives packets and puts them in toProcess. Returns true on success.
    bool server::receivePackets()
    {
        //Check how many sockets are ready, since there's one socket in the set it's 1 on ready 0 if not
        int socketsReady = SDLNet_CheckSockets(socketSet,0);
        if(socketsReady == -1)
        {
            //There was an error
            lastError = syjError::SDLError;
            return false;
        }

        //Nothing to be received
        if(socketsReady == 0)
            return true;

        //If checkSockets says one (our only one) socket is ready, but socketReady says it isn't that's weird
        if(!SDLNet_SocketReady(mainSocket))
        {
            lastError = syjError::readyButNoData;
            return false;
        }

        //Actually receive packets
        int packetsReceived = SDLNet_UDP_RecvV(mainSocket,incomingData);

        //SDLNet threw an error
        if(packetsReceived == -1)
        {
            lastError = syjError::SDLError;
            return false;
        }

        //We received 0 packets despite SocketReady *and* CheckSockets saying there was data to be received
        if(packetsReceived == 0)
        {
            lastError = syjError::readyButNoData;
            return false;
        }

        //Iterate though all SDLNet packets and turn them into syj packets then store them for later
        for(int a = 0; a<packetsReceived; a++)
        {
            packet *tmp = new packet(incomingData[a]->data,incomingData[a]->len);
            tmp->source = incomingData[a]->address;
            toProcess.push_back(tmp);
        }

        return true;
    }

    bool serverClientHandle::sendPacketQueues(int maxPackets)
    {
        bool returnValue = true;

        int packetsToSend = queuedPackets.size();
        if(packetsToSend > 0)
        {
            //Cap it in case of network issues
            //TODO: Figure out dynamic maximum amount of packets to send out
            if(packetsToSend > maxPackets)
                packetsToSend = maxPackets;

            //Copy data to UDPpackets
            for(int b = 0; b<packetsToSend; b++)
            {
                if(!queuedPackets[b])
                    continue;

                //Assumes stream pos is at end of relevant data
                outgoingData[b]->len = (queuedPackets[b]->getStreamPos()+7)/8;
                /*std::cout<<"Outgoingdata["<<b<<"]->len = "<<outgoingData[b]->len<<"\n";
                std::cout<<"Outgoingdata["<<b<<"]->maxlen = "<<outgoingData[b]->maxlen<<"\n";*/
                if(outgoingData[b]->len >= outgoingData[b]->maxlen || outgoingData[b]->len >= queuedPackets[b]->allocatedChunks * syjNET_PacketChunkSize)
                {
                    std::cout<<"serverClientHandle::sendPacketQueues outgoingData[b]->len >= outgoingData[b]->maxlen or queuedPackets[b]->allocatedChunks * syjNET_PacketChunkSize\n";
                    continue;
                }
                else
                    memcpy(outgoingData[b]->data,queuedPackets[b]->data,outgoingData[b]->len);

                //Don't need the packet anymore
                delete queuedPackets[b];
                queuedPackets[b] = 0;
            }

            //Actually send all of the packets we're going to this tick
            if(SDLNet_UDP_SendV(parent->mainSocket,outgoingData,packetsToSend) != packetsToSend)
            {
                parent->lastError = syjError::didNotSend;
                returnValue = false;
            }

            //Clear the sent packets off the list
            queuedPackets.erase(queuedPackets.begin(),queuedPackets.begin() + packetsToSend);
        }

        return returnValue;
    }

    //Receives data from clients, sends acks, responds to acks with clears, kicks inactive clients, resends what needs to be resent, etc.
    //Run once per tick in your main loop
    //Returns true on success, false if an error is thrown
    bool server::run()
    {
        //Have we gone without getting an error
        bool returnValue = true;

        //Actually receive packets
        if(!receivePackets())
            returnValue = false;

        //Sort packets based on type and client, then act upon them
        //TODO: Should howMany be varied based on how busy the server is?
        if(!processPackets(prefs.maxPackets))
            returnValue = false;

        //Check all clients to see if any timed out
        for(unsigned int a = 0; a<prefs.maxClients; a++)
        {
            if(!clients[a])
                continue;
            if(SDL_GetTicks() - clients[a]->lastActivity > prefs.timeoutMS)
            {
                if(!kick(clients[a],syjNET_timedOutMessage))
                    returnValue = false;
                continue;
            }
        }

        //Resize criticalPacket array to remove critical packets that were acknowledged in processPackets
        for(unsigned int a = 0; a<prefs.maxClients; a++)
        {
            if(!clients[a])
                continue;
            std::vector<packet*>::iterator iter;
            for(iter = clients[a]->criticalPackets.begin(); iter != clients[a]->criticalPackets.end();)
            {
                packet *current = (*iter);
                if(!current)
                {
                    iter = clients[a]->criticalPackets.erase(iter);
                    continue;
                }

                ++iter;
            }
        }

        //TODO: Just merge this with previous loop?
        //This loop sends out needed acknowledgments for received critical packets
        for(unsigned int a = 0; a<prefs.maxClients; a++)
        {
            if(!clients[a])
                continue;

            //No need to send out acks more often than every quarter second
            if(SDL_GetTicks() - clients[a]->timeOfOldestAck < 250)
                continue;
            clients[a]->timeOfOldestAck = SDL_GetTicks();

            //We got packets to acknowledge
            unsigned int acksToSend = clients[a]->packetsToAcknowledge.size();
            if(acksToSend > 0)
            {
                //Can't fit all of them in one packet
                if(acksToSend >= acksPerPacket)
                    acksToSend = acksPerPacket-1;
                //Number of ids in packet is stored in a 10 bit number, limit it in the unlikely case someone's using massive packets.
                if(acksToSend >= 1023)
                    acksToSend = 1023;

                packet *ackPacket = new packet;
                ackPacket->writeBit(true);  //Ack packets start with 0b1110
                ackPacket->writeBit(true);
                ackPacket->writeBit(true);
                ackPacket->writeBit(false);

                //Tell the recipient how many ids are in this packet
                ackPacket->writeUInt(acksToSend,10);

                //Write the actual ids to the packet
                for(unsigned int b = 0; b<acksToSend; b++)
                    ackPacket->writeUInt(clients[a]->packetsToAcknowledge[b],prefs.criticalIDBits);

                //Send that packet off someday
                clients[a]->queuedPackets.push_back(ackPacket);

                //Clear all of the acknowledged ids from the to acknowledge list
                clients[a]->packetsToAcknowledge.erase(clients[a]->packetsToAcknowledge.begin(),clients[a]->packetsToAcknowledge.begin() + acksToSend);
            }
        }

        //TODO: Just merge this with previous loop?
        //This loop actually sends out a certain amount of packets to each client
        for(unsigned int a = 0; a<prefs.maxClients; a++)
        {
            if(!clients[a])
                continue;

            //Copy a few critical packets into queuedPackets stack
            if(!clients[a]->sendCriticalPackets(80))
                returnValue = false;

            //Pop some packets off the stack (a vector actually) and send them before resizing vector
            if(!clients[a]->sendPacketQueues(90))
                returnValue = false;
        }

        //True on success
        return returnValue;
    }

    //Make a server and start hosting based on preferences
    server::server(const networkingPreferences &preferences)
    {
        connectHandle = 0;
        disconnectHandle = 0;
        receiveHandle = 0;
        clientsConnected = 0;
        clients = 0;
        socketSet = 0;
        mainSocket = 0;
        incomingData = 0;
        lastError = syjError::noError;

        //Keep a permanent copy of all preferences
        prefs = preferences;

        //Check all of our preferences:

        //Need at least one bit for a critical packet id
        if(prefs.criticalIDBits == 0)
        {
            lastError = syjError::badPreference;
            return;
        }

        //A server needs at least one client, and we're not allowing more than 8-bits for the client for now but that might change
        if(prefs.maxClients < 1 || prefs.maxClients > 255)
        {
            lastError = syjError::badPreference;
            return;
        }

        //Setting port to 0 would just choose a random port
        if(prefs.port == 0)
        {
            lastError = syjError::badPreference;
            return;
        }

        //Calculate a few things based on preferences:

        //First: what's the fewest amount of bits we can represent our maxClients with
        clientIDBits = 0;
        unsigned char tmp = (prefs.maxClients == 1) ? 1 : prefs.maxClients-1;
        while(tmp)
        {
            tmp >>= 1;
            clientIDBits++;
        }

        //How many IDs can we fit in an ack packet
        acksPerPacket = (8*(prefs.maxPacketSize-4))/prefs.criticalIDBits;

        //Highest ID for any incoming or outgoing critical packet
        highestPacketID = (1 << prefs.criticalIDBits) - 1;

        //Create socket and return upon failure
        mainSocket = SDLNet_UDP_Open(prefs.port);
        if(!mainSocket)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Allocate memory for the socket set and return upon failure
        socketSet = SDLNet_AllocSocketSet(1);
        if(!socketSet)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Add socket to socket set and return upon failure
        if(SDLNet_UDP_AddSocket(socketSet,mainSocket) != 1)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Allocate packet array that will be pass to recvV
        incomingData = SDLNet_AllocPacketV(prefs.maxPackets+1,prefs.maxPacketSize);
        if(!incomingData)
        {
            lastError = syjError::SDLError;
            return;
        }

        //Allocate space for client pointers and make sure all slots are set to empty
        clients = new serverClientHandle*[prefs.maxClients];
        for(unsigned int a = 0; a<prefs.maxClients; a++)
            clients[a] = 0;
    }

    std::string serverClientHandle::getIP() const
    {
        return ipStr;
    }

    //Shutdown server and free memory
    server::~server()
    {
        for(unsigned int a = 0; a<toProcess.size(); a++)
            delete toProcess[a];
        if(socketSet)
            SDLNet_FreeSocketSet(socketSet);
        if(mainSocket)
            SDLNet_UDP_Close(mainSocket);
        if(incomingData)
            SDLNet_FreePacketV(incomingData);
        if(clients)
        {
            for(unsigned int a = 0; a<prefs.maxClients; a++)
                if(clients[a])
                    delete clients[a];
            delete clients;
        }
    }
}
