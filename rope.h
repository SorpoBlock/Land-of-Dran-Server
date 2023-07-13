#ifndef ROPE_H_INCLUDED
#define ROPE_H_INCLUDED

#define BT_USE_DOUBLE_PRECISION
#include "BulletSoftBody/btSoftRigidDynamicsWorld.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h"

#include "code/base/server.h"
using namespace syj;

struct rope
{
    int serverID = -1;
    btRigidBody *anchorA = 0;
    btRigidBody *anchorB = 0;
    btSoftBody *handle = 0;

    inline unsigned int getDataBits() { return 108; }
    void sendToClient(serverClientHandle *netRef);
    void removeFromClient(serverClientHandle *netRef);
    void addToPacket(packet *data);
};

#endif // ROPE_H_INCLUDED
