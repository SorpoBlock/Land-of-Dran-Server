#include "light.h"

void light::sendToClient(serverClientHandle *client)
{
    packet data;
    data.writeUInt(packetType_addRemoveLight,packetTypeBits);
    data.writeUInt(serverID,20);
    data.writeBit(true); // adding, not removing

    if(lifeTimeMS < 0)
        lifeTimeMS = 0;
    data.writeUInt(lifeTimeMS,16);

    if(attachedBrick)
    {
        data.writeUInt(1,3);
        data.writeUInt(attachedBrick->serverID,20);
    }
    else if(attachedCar)
    {
        std::cout<<"Send light packet has attached car: "<<attachedCar->serverID<<"\n";
        data.writeUInt(2,3);
        data.writeUInt(attachedCar->serverID,10);
        data.writeFloat(offset.x());
        data.writeFloat(offset.y());
        data.writeFloat(offset.z());
    }
    else if(attachedDynamic)
    {
        data.writeUInt(3,3);
        data.writeUInt(attachedDynamic->serverID,dynamicObjectIDBits);
        data.writeString(node);
    }
    else if(attachedItem)
    {
        data.writeUInt(4,3);
        data.writeUInt(attachedItem->serverID,dynamicObjectIDBits);
        data.writeString(node);
    }
    else
    {
        data.writeUInt(0,3);
        data.writeFloat(position.x());
        data.writeFloat(position.y());
        data.writeFloat(position.z());
    }

    data.writeFloat(color.x());
    data.writeFloat(color.y());
    data.writeFloat(color.z());

    data.writeFloat(blinkVel.x());
    data.writeFloat(blinkVel.y());
    data.writeFloat(blinkVel.z());
    data.writeFloat(yawVel);

    data.writeBit(isSpotlight);
    if(isSpotlight)
    {
        data.writeFloat(direction.x());
        data.writeFloat(direction.y());
        data.writeFloat(direction.z());
        data.writeFloat(direction.w());
    }

    client->send(&data,true);
}

void light::removeFromClient(serverClientHandle *client)
{
    packet data;
    data.writeUInt(packetType_addRemoveLight,packetTypeBits);
    data.writeUInt(serverID,20);
    data.writeBit(false); //removing, not adding
    client->send(&data,true);
}
