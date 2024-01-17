#include "Tdynamic.h"

Tdynamic::Tdynamic(dynamicType *_type,const btVector3 &initialPos) : type(_type)
{
    body = new btRigidBody(type->mass,type->defaultMotionState,type->defaultCollisionShape,type->defaultIntertia);
    world->addRigidBody(body);

    /*
        Note physics objects don't have smart pointers as userdata
        User pointers/data are pretty much only uses when looking at the results of raycasts or collision checks
        objHolder::deleteByPointer can be used to safely delete an object that's been collided with, for example
    */
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

void Tdynamic::addToCreationPacket(syj::packet * const data) const
{
    data->writeUInt(netID,netIDbits);
    //What model / collision box does this object use?
    data->writeUInt(type->dynamicTypeID,dynamicTypeIDBits);

    //Write initial transform
    btTransform t = body->getWorldTransform();
    btVector3 p = t.getOrigin();
    data->writeFloat(p.x());
    data->writeFloat(p.y());
    data->writeFloat(p.z());
    writeQuat(t.getRotation(),data);
}

unsigned int Tdynamic::getCreationPacketBits() const
{
    //Quaternions take 30 bits using writeQuat
    return netIDbits + dynamicTypeIDBits + sizeof(float)*3*8 + 30;
}

unsigned int Tdynamic::getUpdatePacketBits() const
{
    //Quaternions take 30 bits using writeQuat
    return netIDbits + sizeof(float)*3*8 + 30;
}

//Dynamics are a bit special because we want to check their position and rotation for updates
//They can require an update for something that happened internally inside the physics engine
//If the object is stationary, don't send an update packet
bool Tdynamic::requiresNetUpdate() const
{
    //One of our defined class functions already set this flag, no matter what we need an update packet sent out
    if(requiresUpdate)
        return true;

    //Might be redundant
    if(body->isActive())
        return true;

    //TODO: Maximum of 500 ms between packets for an object is low, this should be turned into a define and increased
    if(SDL_GetTicks() - lastSendTransformTime > 500)
        return true;

    //Arbitrary values meant to represent what a person might not notice
    if((lastPosition - body->getWorldTransform().getOrigin()).length() > 0.03)
        return true;

    if((lastRotation - body->getWorldTransform().getRotation()).length() > 0.03)
        return true;

    return false;
}

void Tdynamic::addToUpdatePacketFinal(syj::packet * const data)
{
    data->writeUInt(netID,netIDbits);

    //Write updated transform
    btTransform t = body->getWorldTransform();
    btVector3 p = t.getOrigin();
    data->writeFloat(p.x());
    data->writeFloat(p.y());
    data->writeFloat(p.z());
    writeQuat(t.getRotation(),data);

    data->writeBit(isPlayer); //Should always be false, as this function is overriden by the player class

    //Used to figure out how long until the next transform update packet is sent out
    lastPosition = p;
    lastRotation = t.getRotation();
    lastSendTransformTime = SDL_GetTicks();
}

void Tdynamic::pushLua(lua_State * const L)
{
    lua_newtable(L);
    lua_getglobal(L,"dynamic"); //TODO: dynamicMETATABLE
    lua_setmetatable(L,-2);
    lua_pushinteger(L,netID);
    lua_setfield(L,-2,"id");
}

void Tdynamic::sendClientPhysics(syj::serverClientHandle * const target) const
{
    packet data;
    data.writeUInt(packetType_clientPhysicsData,packetTypeBits);
    data.writeUInt(startClientPhysics,2); //only 3 options for this enum -> 2 bits
    data.writeUInt(netID,netIDbits);

    //Collision box size and offset
    data.writeFloat(type->finalHalfExtents.x());
    data.writeFloat(type->finalHalfExtents.y());
    data.writeFloat(type->finalHalfExtents.z());

    data.writeFloat(type->finalOffset.x());
    data.writeFloat(type->finalOffset.y());
    data.writeFloat(type->finalOffset.z());

    //Transform:
    btTransform t = body->getWorldTransform();
    data.writeFloat(t.getOrigin().x());
    data.writeFloat(t.getOrigin().y());
    data.writeFloat(t.getOrigin().z());

    data.writeFloat(t.getRotation().w());
    data.writeFloat(t.getRotation().x());
    data.writeFloat(t.getRotation().y());
    data.writeFloat(t.getRotation().z());

    //Velocity:
    data.writeFloat(body->getLinearVelocity().x());
    data.writeFloat(body->getLinearVelocity().y());
    data.writeFloat(body->getLinearVelocity().z());

    data.writeFloat(body->getAngularVelocity().x());
    data.writeFloat(body->getAngularVelocity().y());
    data.writeFloat(body->getAngularVelocity().z());

    target->send(&data,true);
}

void Tdynamic::pauseClientPhysics(syj::serverClientHandle * const target) const
{
    packet data;
    data.writeUInt(packetType_clientPhysicsData,packetTypeBits);
    data.writeUInt(suspendClientPhysics,2); //only 3 options for this enum -> 2 bits
    data.writeUInt(netID,netIDbits);

    target->send(&data,true);
}

void Tdynamic::stopClientPhysics(syj::serverClientHandle * const target) const
{
    packet data;
    data.writeUInt(packetType_clientPhysicsData,packetTypeBits);
    data.writeUInt(destroyClientPhysics,2); //only 3 options for this enum -> 2 bits
    data.writeUInt(netID,netIDbits);

    target->send(&data,true);
}

void Tdynamic::applyBuoyancy(float waterLevel)
{
    //Players can swim...
    if(isPlayer)
        return;

    //Vehicles have a more advanced buoyancy simulation than this
    if(body->getWorldTransform().getOrigin().y() < waterLevel-2)
    {
        body->setDamping(0.4,0);
        body->setGravity(btVector3(0,-0.5,0) * world->getGravity());
    }
    else if(body->getWorldTransform().getOrigin().y() < waterLevel)
    {
        body->setDamping(0.4,0);
        body->setGravity(btVector3(0,0,0));
    }
    else
    {
        body->setDamping(0,0);
        body->setGravity(world->getGravity());
    }
}

void Tdynamic::applyProjectileRotation()
{
    if(!isProjectile)
        return;

    if(body->getLinearVelocity().length() < 8)
        return;

    btTransform t = body->getWorldTransform();
    t.setOrigin(btVector3(0,0,0));
    btQuaternion oldQuat = t.getRotation();

    btVector3 currentUp = t * btVector3(0,1,0);
    btVector3 desiredUp = body->getLinearVelocity();
    desiredUp = desiredUp.normalized();

    float dot = btDot(currentUp,desiredUp);
    if(dot > 0.99999 || dot < -0.99999)
        return;

    btVector3 cross = btCross(currentUp,desiredUp);
    btQuaternion rot = btQuaternion(cross.x(),cross.y(),cross.z(),1+dot);
    rot = rot.normalized();

    t = body->getWorldTransform();
    t.setRotation(rot * oldQuat);
    body->setWorldTransform(t);
}

bool Tdynamic::getAppearancePacket(packet * const data) const
{
    if(nodeNames.size() < 1)
        return false;

    data->writeUInt(packetType_setNodeColors,packetTypeBits);
    data->writeUInt(netID,netIDbits);
    data->writeUInt(chosenDecal,7); //Maximum of 128 decals for now for some reason
    data->writeUInt(nodeNames.size(),7); //Maximum of 128 different node colors
    for(unsigned int a = 0; a<std::max((size_t)128,nodeNames.size()); a++)
    {
        data->writeString(nodeNames[a]);
        data->writeUInt(nodeColors[a].x() * 255,8);
        data->writeUInt(nodeColors[a].y() * 255,8);
        data->writeUInt(nodeColors[a].z() * 255,8);
    }

    return true;
}

void Tdynamic::setNodeColor(const std::string &node,const btVector3 &nodeColor)
{
    if(node == "")
        return;

    bool nodeFound = false;

    for(unsigned int a = 0; a<node.size(); a++)
    {
        if(nodeNames[a] == node)
        {
            nodeColors[a] = nodeColor;
            nodeFound = true;
            break;
        }
    }

    if(!nodeFound)
    {
        nodeNames.push_back(node);
        nodeColors.push_back(nodeColor);
    }
}

void Tdynamic::setDecal(unsigned int decalIdx)
{
    chosenDecal = decalIdx;
}

void Tdynamic::setShapeName(syj::server * const server,const std::string &text,float r,float g,float b)
{
    shapeName = text;
    shapeNameColor = btVector3(r,g,b);

    packet data;
    data.writeUInt(packetType_setShapeName,packetTypeBits);
    data.writeUInt(netID,netIDbits);
    data.writeString(shapeName);
    data.writeFloat(r);
    data.writeFloat(g);
    data.writeFloat(b);
    server->send(&data,true);
}

//This function is pretty sparse without the code for emitters, sounds, killing, vignette, respawn, etc.
bool Tdynamic::applyDamage(syj::server * const server,float amount)
{
    if(!canTakeDamage)
        return false;

    /*scope("Tdynamic::applyDamage");

    if(isPlayer)
        error("Somehow called base class dynamic::applyDamage instead of player specific function!");*/

    health -= amount;

    return health <= 0;
}

//TODO: Animations are either hard-coded for walking and sent over with transform update, or set for items via. fire event
//No generic animation system yet
void Tdynamic::playAnimation(syj::server * const server,const std::string &animationName,float speed,bool resetFrame,bool loop)
{

}

void Tdynamic::stopAnimation(syj::server * const server,const std::string &animationName)
{

}



