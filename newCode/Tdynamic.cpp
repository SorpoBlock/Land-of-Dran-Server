#include "Tdynamic.h"

Tdynamic::Tdynamic(dynamicType *_type,btVector3 initialPos)
{
    type = _type;
    body = new btRigidBody(type->mass,type->defaultMotionState,type->defaultCollisionShape,type->defaultIntertia);
    world->addRigidBody(body);
    body->setUserPointer(this);
    body->setUserIndex(bodyUserIndex_dynamic);
    btTransform t;
    t.setIdentity();
    t.setOrigin(initialPos);
    body->setWorldTransform(t);
}

Tdynamic::~Tdynamic()
{
    world->removeRigidBody(body);
    delete body;
}

void Tdynamic::addToCreationPacket(syj::packet *data) const
{

}

unsigned int Tdynamic::getUpdatePacketBits() const
{
    return 0;
}

unsigned int Tdynamic::getCreationPacketBits() const
{
    return 0;
}

void Tdynamic::addToUpdatePacketFinal(syj::packet * const data)
{

}

void Tdynamic::pushLua(lua_State * const L)
{

}
