#ifndef ITEM_H_INCLUDED
#define ITEM_H_INCLUDED

#include "code/base/server.h"
#include "code/dynamic.h"

#define bodyUserIndex_item 77

struct itemType
{
    float rotW=1.0,rotX=0.0,rotY=0.0,rotZ=0.0;
    float handOffsetX=0,handOffsetY=0,handOffsetZ=0;
    int itemTypeID = 0;
    int switchAnim = -1;
    int fireAnim = -1;
    bool useDefaultSwing = true;
    std::string uiName = "";
    std::string iconPath = "";
    dynamicType *model = 0;

    itemType(dynamicType *_model,int id);
    void sendToClient(serverClientHandle *netRef);
};

struct item : dynamic
{
    unsigned int lastFire = 0;

    void setHidden(bool newHidden)
    {
        if(newHidden != hidden)
            otherwiseChanged = true;
        hidden = newHidden;
    }

    void setSwinging(bool newSwinging)
    {
        if(swinging != newSwinging)
            otherwiseChanged = true;
        swinging = newSwinging;
    }

    bool performRaycast = false;
    int nextFireAnim = -1;
    float nextFireAnimSpeed = 1.0;
    int nextFireSound = -1;
    float nextFireSoundPitch = 1.0;
    float nextFireSoundGain = 1.0;
    int nextFireEmitter = -1;
    std::string nextFireEmitterMesh = "";
    int fireCooldownMS = 0;
    int lastFireEvent = 0;
    bool useBulletTrail = true;
    btVector3 bulletTrailColor = btVector3(1.0,0.5,0.0);
    float bulletTrailSpeed = 1.0;

    bool hidden = false;
    bool swinging = false;
    bool switchedHolder = false;
    dynamic *heldBy = 0;
    itemType *heldItemType = 0;
    void sendToClient(serverClientHandle *netRef);
    void addToPacket(packet *data);
    unsigned int getPacketBits();
    item(itemType *type,btDynamicsWorld *world,int idServer,int idType);
};

#endif // ITEM_H_INCLUDED
