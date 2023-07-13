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

    if(common->dynamicTypes.size() < 1)
    {
        error("No dynamic types registered whatsoever!");
        return;
    }

    /*float red = (rand() % 1000);
    red /= 1000.0;
    float green = (rand() % 1000);
    green /= 1000.0;
    float blue = (rand() % 1000);
    blue /= 1000.0;
    dynamic *theirPlayer = common->addDynamic(0,red,green,blue);

    btTransform t;
    t.setOrigin(btVector3(100,700,100));
    theirPlayer->setWorldTransform(t);
    theirPlayer->makePlayer();

    tmp->controlling = theirPlayer;
    tmp->cameraBoundToDynamic = true;
    tmp->cameraTarget = theirPlayer;*/
}

#endif // ONCONNECTION_H_INCLUDED
