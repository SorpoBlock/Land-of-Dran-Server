#include "player.h"

player::player(dynamicType *_type,const btVector3 &initialPos) : Tdynamic(_type,initialPos)
{
    //Physics engine can't try and safe CPU by not simulating this object if it stops moving
    body->setActivationState(DISABLE_DEACTIVATION);
    body->setFriction(1);
    body->setRestitution(0.5); //Moderate bounciness
    body->setAngularFactor(btVector3(0,0,0)); //Cannot rotate as a result of physics, but can have its rotation manually set
    isPlayer = true;
}

//Has this object been updated in such a way that we need to resend its properties to clients?
bool player::requiresNetUpdate() const
{
    return Tdynamic::requiresNetUpdate();
}

//Does not reset requiresUpdate, intended to be called by objHolder
void player::addToUpdatePacketFinal(syj::packet * const data)
{
    data->writeUInt(netID,netIDbits);

    //Write updated transform
    btTransform t = body->getWorldTransform();
    btVector3 p = t.getOrigin();
    data->writeFloat(p.x());
    data->writeFloat(p.y());
    data->writeFloat(p.z());
    writeQuat(t.getRotation(),data);

    data->writeBit(isPlayer); //Should always be true

    //The only part different from Tdynamic
    //TODO: This line lets clients do the walking animation, it will be replaced with a proper animation system soon
    data->writeBit(walking);
    //This handles the players head tilting:
    data->writeFloat(asin(-lastCamY) - (0.5*3.1415*crouchProgress));

    //Used to figure out how long until the next transform update packet is sent out
    lastPosition = p;
    lastRotation = t.getRotation();
    lastSendTransformTime = SDL_GetTicks();
}

//Networking to be done (along with other objects of this type) when a client joins the game or the object is created
void player::addToCreationPacket(syj::packet * const data) const
{
    Tdynamic::addToCreationPacket(data);
}

//How many bytes does addToUpdatePacket add to a syj::packet
unsigned int player::getCreationPacketBits() const
{
    return Tdynamic::getCreationPacketBits();
}

//How many bytes does addToUpdatePacket add to a syj::packet
unsigned int player::getUpdatePacketBits() const
{
    //Quaternions take 30 bits using writeQuat
    return netIDbits + sizeof(float)*3*8 + 30 + sizeof(float)*1*8 + 1;
}

/*
   Create a lua variable / return value that refers to this object
   This function cannot be const because lua needs a non const pointer
   It's worth noting this is a situation where the use of shared_ptr kind of breaks down
   Lua calls can still crash if they use an object that's been deleted after all shared_ptrs are out of scope
*/
void player::pushLua(lua_State * const L)
{
    lua_newtable(L);
    lua_getglobal(L,"playerMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,netID);
    lua_setfield(L,-2,"id");
}

player::~player()
{

}
