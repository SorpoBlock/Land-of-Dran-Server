#include "Titem.h"

Titem::Titem(itemType *_type,const btVector3 &initialPos)
    : Tdynamic(_type->model,initialPos)
{
    itemProperties = _type;
    body->setUserIndex(bodyUserIndex_item);
    isItem = true;
}

bool Titem::requiresNetUpdate() const
{
    //We will set requiresUpdate to true if the item is picked up or dropped
    //But that's about the only change from dynamic base class
    return Tdynamic::requiresNetUpdate();
}

void Titem::addToUpdatePacketFinal(syj::packet * const data)
{
    data->writeUInt(netID,netIDbits);

    data->writeBit(isBeingHeld);
    if(isBeingHeld)
    {
        //ID of the player holding the item
        data->writeUInt(holderNetID,netIDbits);
        data->writeBit(isHidden); //If this item isnt the one the player is currently using
        data->writeBit(isSwinging);
        return;
    }

    //Write updated transform
    btTransform t = body->getWorldTransform();
    btVector3 p = t.getOrigin();
    data->writeFloat(p.x());
    data->writeFloat(p.y());
    data->writeFloat(p.z());
    writeQuat(t.getRotation(),data);

    //Used to figure out how long until the next transform update packet is sent out
    lastPosition = p;
    lastRotation = t.getRotation();
    lastSendTransformTime = SDL_GetTicks();
}

void Titem::addToCreationPacket(syj::packet * const data) const
{
    data->writeUInt(netID,netIDbits);
    //Client knows dynamic type from item type
    data->writeUInt(itemProperties->itemTypeID,dynamicTypeIDBits);

    //Write initial transform
    btTransform t = body->getWorldTransform();
    btVector3 p = t.getOrigin();
    data->writeFloat(p.x());
    data->writeFloat(p.y());
    data->writeFloat(p.z());
    writeQuat(t.getRotation(),data);
}

//How many bytes does addToUpdatePacket add to a syj::packet
unsigned int Titem::getCreationPacketBits() const
{
    //Only difference is item type instead of dynamic type, both use same # of bits
    return Tdynamic::getCreationPacketBits();
}

//How many bytes does addToUpdatePacket add to a syj::packet
unsigned int Titem::getUpdatePacketBits() const
{
    if(isBeingHeld)
        return netIDbits + 1 + netIDbits + 2;
    else
        return netIDbits + 1 + sizeof(float) * 3 * 8 + 30;
}

/*
   Create a lua variable / return value that refers to this object
   This function cannot be const because lua needs a non const pointer
   It's worth noting this is a situation where the use of shared_ptr kind of breaks down
   Lua calls can still crash if they use an object that's been deleted after all shared_ptrs are out of scope
*/
void Titem::pushLua(lua_State * const L)
{
    lua_newtable(L);
    lua_getglobal(L,"itemMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,netID);
    lua_setfield(L,-2,"id");
}

void Titem::setHidden(bool value)
{
    isHidden = value;
    requiresUpdate = true;
}

void Titem::setSwinging(bool value)
{
    isSwinging = value;
    requiresUpdate = true;
}

void Titem::setFireAnim(syj::server * const server,const std::string &animName,float animSpeed)
{
    nextFireAnim = type->getAnimationIdx(animName);
    nextFireAnimSpeed = std::clamp(animSpeed,0.01f,10.0f);

    packet updateAnimPacket;
    updateAnimPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    updateAnimPacket.writeString("setItemFireAnim");
    updateAnimPacket.writeUInt(netID,netIDbits);

    if(nextFireAnim >= 0 && nextFireAnim < 1024)
    {
        updateAnimPacket.writeBit(true); //Use an animation
        updateAnimPacket.writeUInt(nextFireAnim,10);
        updateAnimPacket.writeFloat(animSpeed);
    }
    else
        updateAnimPacket.writeBit(false); //No animation

    server->send(&updateAnimPacket,true);
}

void Titem::setFireSound(syj::server * const server,int soundIdx,float pitch,float gain)
{
    nextFireSoundGain  = std::clamp(gain,0.01f,2.0f);
    nextFireSoundPitch = std::clamp(pitch,0.01f,10.0f);

    packet updateSoundPacket;
    updateSoundPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    updateSoundPacket.writeString("setItemFireSound");
    updateSoundPacket.writeUInt(netID,netIDbits);

    if(soundIdx >= 0 && soundIdx < 1024)
    {
        updateSoundPacket.writeBit(true); //Use a sound
        updateSoundPacket.writeUInt(soundIdx,10);
        updateSoundPacket.writeFloat(nextFireSoundPitch);
        updateSoundPacket.writeFloat(nextFireSoundGain);
    }
    else
        updateSoundPacket.writeBit(false); //Do not use a sound

    server->send(&updateSoundPacket,true);
}

void Titem::setFireEmitter(syj::server * const server,int emitterIdx,const std::string &emitterMesh)
{
    //Should do some checking for mesh name here but eh, client will report any errors as well

    nextFireEmitterMesh = emitterMesh;
    nextFireEmitter = emitterIdx;

    packet updateFireEmitter;
    updateFireEmitter.writeUInt(packetType_serverCommand,packetTypeBits);
    updateFireEmitter.writeString("setItemFireEmitter");
    updateFireEmitter.writeUInt(netID,netIDbits);

    if(emitterIdx >= 0 && emitterIdx < 1024)
    {
        updateFireEmitter.writeBit(true); //Use an emitter
        updateFireEmitter.writeUInt(emitterIdx,10);
        updateFireEmitter.writeString(emitterMesh);
    }
    else
        updateFireEmitter.writeBit(false); //Do not use an emitter

    server->send(&updateFireEmitter,true);
}

void Titem::setItemCooldown(syj::server * const server,int milliseconds)
{
    //Needs to fit in 16 bits
    fireCooldownMS = std::clamp(milliseconds,0,64000);

    packet updateCooldownPacket;
    updateCooldownPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    updateCooldownPacket.writeString("setItemCooldown");
    updateCooldownPacket.writeUInt(netID,netIDbits);
    updateCooldownPacket.writeUInt(fireCooldownMS,16);
    server->send(&updateCooldownPacket,true);
}

void Titem::setItemBulletTrail(syj::server * const server,bool use,btVector3 color,float speed)
{
    bulletTrailColor = color;
    bulletTrailSpeed = speed;
    useBulletTrail = use;

    packet bulletTrailPacket;
    bulletTrailPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    bulletTrailPacket.writeString("setItemBulletTrail");
    bulletTrailPacket.writeUInt(netID,netIDbits);
    bulletTrailPacket.writeBit(use);
    bulletTrailPacket.writeFloat(bulletTrailColor.x());
    bulletTrailPacket.writeFloat(bulletTrailColor.y());
    bulletTrailPacket.writeFloat(bulletTrailColor.z());
    bulletTrailPacket.writeFloat(bulletTrailSpeed);
    server->send(&bulletTrailPacket,true);
}

void Titem::setPerformRaycast(bool toggle)
{
    performRaycast = toggle;
}

void Titem::removeHolder()
{
    requiresUpdate = true;
    isBeingHeld = false;
}

void Titem::addHolder(unsigned int netID)
{
    requiresUpdate = true;
    isBeingHeld = true;
    holderNetID = netID;
}

std::string Titem::getTypeName() const
{
    return itemProperties->uiName;
}

Titem::~Titem()
{

}








