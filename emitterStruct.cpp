#include "emitterStruct.h"

void emitter::sendToClient(serverClientHandle *netRef)
{
    if(!type)
    {
        error("emitter::sendToClient lacked type!");
        return;
    }

    packet data;
    data.writeUInt(packetType_emitterAddRemove,packetTypeBits);
    data.writeBit(true); //adding one, not removing
    data.writeUInt(serverID,20);
    data.writeUInt(type->serverID,10);
    data.writeFloat(color.x());
    data.writeFloat(color.y());
    data.writeFloat(color.z());
    data.writeBit(shootFromHand);
    if(attachedToBrick)
    {
        data.writeUInt(0,2);
        data.writeUInt(attachedToBrick->serverID,20);
    }
    else if(attachedToModel)
    {
        data.writeUInt(1,2);
        data.writeUInt(attachedToModel->serverID,dynamicObjectIDBits);
        data.writeString(nodeName);
    }
    else
    {
        data.writeUInt(2,2);
        data.writeFloat(position.x());
        data.writeFloat(position.y());
        data.writeFloat(position.z());
    }
    netRef->send(&data,true);
}

void particleType::sendToClient(serverClientHandle *netRef)
{
    packet data;
    data.writeUInt(packetType_newEmitterParticleType,packetTypeBits);
    data.writeBit(false); // particle, not emitter
    data.writeUInt(serverID,10);
    data.writeString(filePath);
    for(int a = 0; a<4; a++)
    {
        data.writeFloat(colors[a].x());
        data.writeFloat(colors[a].y());
        data.writeFloat(colors[a].z());
        data.writeFloat(colors[a].w());
        data.writeFloat(sizes[a]);
        data.writeFloat(times[a]);
    }
    data.writeFloat(drag.x());
    data.writeFloat(drag.y());
    data.writeFloat(drag.z());
    data.writeFloat(gravity.x());
    data.writeFloat(gravity.y());
    data.writeFloat(gravity.z());
    data.writeFloat(inheritedVelFactor);
    data.writeFloat(lifetimeMS);
    data.writeFloat(lifetimeVarianceMS);
    data.writeFloat(spinSpeed);
    data.writeBit(useInvAlpha);
    data.writeBit(needsSorting);
    netRef->send(&data,true);
}

void emitterType::sendToClient(serverClientHandle *netRef)
{
    packet data;
    data.writeUInt(packetType_newEmitterParticleType,packetTypeBits);
    data.writeBit(true); // emitter, not particle
    data.writeUInt(serverID,10);
    data.writeUInt(particleTypes.size(),8);
    for(int a = 0; a<particleTypes.size(); a++)
        data.writeUInt(particleTypes[a]->serverID,10);
    data.writeFloat(ejectionOffset);
    data.writeFloat(ejectionPeriodMS);
    data.writeFloat(ejectionVelocity);
    data.writeFloat(periodVarianceMS);
    data.writeFloat(phiReferenceVel);
    data.writeFloat(phiVariance);
    data.writeFloat(thetaMin);
    data.writeFloat(thetaMax);
    data.writeFloat(velocityVariance);
    data.writeFloat(lifetimeMS);
    if(uiName.length() > 0)
    {
        data.writeBit(true);
        data.writeString(uiName);
    }
    else
        data.writeBit(false);
    netRef->send(&data,true);
}
