#include "item.h"

void itemType::sendToClient(serverClientHandle *netRef)
{
    packet data;
    data.writeUInt(packetType_addItemType,packetTypeBits);
    data.writeUInt(itemTypeID,10);
    data.writeUInt(model->dynamicTypeID,10);
    data.writeBit(useDefaultSwing);
    if(!useDefaultSwing)
    {
        data.writeBit(fireAnim != -1);
        if(fireAnim != -1)
            data.writeUInt(fireAnim,8);
    }
    data.writeBit(switchAnim != -1);
    if(switchAnim != -1)
        data.writeUInt(switchAnim,8);

    data.writeString(uiName);

    data.writeFloat(handOffsetX);
    data.writeFloat(handOffsetY);
    data.writeFloat(handOffsetZ);

    data.writeString(iconPath);

    netRef->send(&data,true);
}

itemType::itemType(dynamicType *_model,int id)
{
    model = _model;
    itemTypeID = id;
}

void item::sendToClient(serverClientHandle *netRef)
{
    packet data;
    //data.writeUInt(packetType_addRemoveItem,packetTypeBits);
    data.writeUInt(packetType_addRemoveItem,packetTypeBits);
    data.writeBit(true); //add
    data.writeUInt(heldItemType->itemTypeID,10);
    data.writeUInt(serverID,dynamicObjectIDBits);
    netRef->send(&data,true);
}

unsigned int item::getPacketBits()
{
    if(heldBy)
        return (2*dynamicObjectIDBits)+4;//24;
    else
        return dynamicObjectIDBits+128;
}

void item::addToPacket(packet *data)
{
    /*if(heldBy)
    {
        if(!switchedHolder)
            return;
        switchedHolder = false;
    }*/
    //std::cout<<"Adding item: "<<serverID<<" rot: "<<getWorldTransform().getRotation().w()<<"\n";

    data->writeUInt(serverID,dynamicObjectIDBits);
    data->writeBit(true);// is item
    data->writeBit(heldBy);
    if(heldBy)
    {
        data->writeUInt(heldBy->serverID,dynamicObjectIDBits);
        data->writeBit(hidden);
        data->writeBit(swinging);
    }
    else
    {
        data->writeFloat(getWorldTransform().getOrigin().x());
        data->writeFloat(getWorldTransform().getOrigin().y());
        data->writeFloat(getWorldTransform().getOrigin().z());
        writeQuat(getWorldTransform().getRotation(),data);
        /*data->writeFloat(getWorldTransform().getRotation().x());
        data->writeFloat(getWorldTransform().getRotation().y());
        data->writeFloat(getWorldTransform().getRotation().z());
        data->writeFloat(getWorldTransform().getRotation().w());*/
    }
}

item::item(itemType *type,btDynamicsWorld *world,int idServer,int idType)
: dynamic(type->model,world,idServer,idType)
{
    heldItemType = type;
    setUserIndex(bodyUserIndex_item);
    setUserPointer(this);
}
