#ifndef ONCONNECTION_H_INCLUDED
#define ONCONNECTION_H_INCLUDED

#include "code/unifiedWorld.h"

using namespace syj;

void onConnection(server *host,serverClientHandle *client)
{
    unifiedWorld *common = (unifiedWorld*)host->userData;

    info("Someone attempted to connect, IP: " + client->getIP());

    clientData *tmp = new clientData;
    tmp->netRef = client;
    common->users.push_back(tmp);
}

#endif // ONCONNECTION_H_INCLUDED
