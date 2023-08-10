#ifndef ITEM_H_INCLUDED
#define ITEM_H_INCLUDED

#include "code/base/server.h"
#include "code/dynamic.h"

#define bodyUserIndex_item 77

struct itemType
{
    float handOffsetX=0,handOffsetY=0,handOffsetZ=0;
    int itemTypeID = 0;
    int switchAnim = -1;
    int fireAnim = -1;
    bool useDefaultSwing = true;
    std::string uiName = "";
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
