#include "simObject.h"

btDynamicsWorld *simObject::world = nullptr;

simObject::simObject()
{

}

simObject::~simObject()
{

}

bool simObject::requiresNetUpdate() const
{
    return requiresUpdate;
}

netIDtype simObject::getID() const
{
    return netID;
}

uint32_t simObject::getCreationTime() const
{
    return creationTime;
}

//objHolder should call this if requiresNetUpdate is true
void simObject::addToUpdatePacket(syj::packet *data)
{
    addToUpdatePacketFinal(data);
    requiresUpdate = false;
}
