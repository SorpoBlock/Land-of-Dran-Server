#ifndef ONDISCONNECT_H_INCLUDED
#define ONDISCONNECT_H_INCLUDED

#include "code/unifiedWorld.h"

using namespace syj;

void onDisconnect(server *host,serverClientHandle *client)
{
    unifiedWorld *common = (unifiedWorld*)host->userData;

    info("Someone attempted to disconnect, IP: " + client->getIP());

    for(unsigned int a = 0; a<common->users.size(); a++)
    {
        if(common->users[a]->netRef == client)
        {
            clientLeaveEvent(common->users[a]);

            if(common->users[a]->authHandle)
            {
                curl_multi_remove_handle(common->curlHandle,common->users[a]->authHandle);
                curl_easy_cleanup(common->users[a]->authHandle);
                common->users[a]->authHandle = 0;
            }

            if(common->users[a]->driving)
                common->users[a]->driving->occupied = false;

            if(common->users[a]->adminOrb)
                common->removeEmitter(common->users[a]->adminOrb);

            if(common->users[a]->controlling)
            {
                /*for(unsigned int b = 0;b<common->items.size(); b++)
                {
                    if(common->items[b]->heldBy == common->users[a]->controlling)
                    {
                        common->dropItem(common->items[b]);
                    }
                }*/

                //for(unsigned int z = 0; z<inventorySize; z++)
                  //  common->dropItem(common->users[a]->controlling,z,false);
                /*for(unsigned int z = 0; z<inventorySize; z++)
                    if(common->users[a]->controlling->holding[z])
                        common->removeItem(common->users[a]->controlling->holding[z]);*/

                //std::cout<<"Succesfully deleted: "<<common->users[a]->controlling<<" dynamic id "<<common->users[a]->controlling->serverID<<"\n";
                common->removeDynamic(common->users[a]->controlling);
                common->users[a]->setControlling(0);
            }

            for(int z = 0; z<common->users.size(); z++)
            {
                if(z == a)
                    continue;

                //if(common->users[z]->hasLuaAccess)
                //{
                    packet data;
                    data.writeUInt(packetType_addOrRemovePlayer,packetTypeBits);
                    data.writeBit(true); // for player's list, not typing players list
                    data.writeBit(false); // remove
                    data.writeString(common->users[a]->name);
                    data.writeUInt(common->users[a]->playerID,16);
                    common->users[z]->netRef->send(&data,true);
                //}
            }

            info(common->users[a]->name + " left.");
            if(common->users[a]->name != "Unnamed")
            {
                common->messageAll("[colour='FF0000FF']" + common->users[a]->name + " has disconnected.","server");
                common->playSound("PlayerLeave");
            }

            for(int b = 0; b<common->queuedRespawn.size(); b++)
            {
                if(common->queuedRespawn[b] == common->users[a])
                {
                    common->queuedRespawn.erase(common->queuedRespawn.begin() + b);
                    common->queuedRespawnTime.erase(common->queuedRespawnTime.begin() + b);
                    break;
                }
            }

            delete common->users[a];
            common->users[a] = 0;
            common->users.erase(common->users.begin() + a);
            return;
        }
    }
}

#endif // ONDISCONNECT_H_INCLUDED
