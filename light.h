#ifndef LIGHT_H_INCLUDED
#define LIGHT_H_INCLUDED

#include "code/base/server.h"
#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include "code/livingBrick.h"
#include "code/brick.h"
#include "code/item.h"
using namespace syj;

struct light
{
    int serverID = -1;
    btVector3 position = btVector3(0,0,0);
    btVector3 offset = btVector3(0,0,0);
    btVector3 color = btVector3(0,0,0);
    btVector4 direction = btVector4(0,0,0,0); //w = angle phi
    bool isSpotlight = false;

    int lifeTimeMS = 0;
    std::string node = "";
    brick *attachedBrick = 0;
    brickCar *attachedCar = 0;
    item *attachedItem = 0;
    dynamic *attachedDynamic = 0;

    void sendToClient(serverClientHandle *client);
    void removeFromClient(serverClientHandle *client);
};

#endif // LIGHT_H_INCLUDED
