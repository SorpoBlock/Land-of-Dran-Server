#include "dynamic.h"

void dynamic::setNodeColor(std::string nodeName,btVector3 nodeColor,server *broadcast)
{
    if(nodeName == "")
        return;

    bool nodeFound = false;

    for(int a = 0; a<nodeNames.size(); a++)
    {
        if(nodeNames[a] == nodeName)
        {
            nodeColors[a] = nodeColor;
            nodeFound = true;
            break;
        }
    }

    if(!nodeFound)
    {
        nodeNames.push_back(nodeName);
        nodeColors.push_back(nodeColor);
    }

    if(broadcast)
    {
        packet data;
        data.writeUInt(packetType_setNodeColors,packetTypeBits);
        data.writeUInt(serverID,dynamicObjectIDBits);
        data.writeUInt(chosenDecal,7);
        data.writeUInt(1,7);
        data.writeString(nodeName);
        data.writeUInt(nodeColor.x() * 255,8);
        data.writeUInt(nodeColor.y() * 255,8);
        data.writeUInt(nodeColor.z() * 255,8);
        broadcast->send(&data,true);
    }
}

void dynamic::nodeColorsPacket(packet *data)
{
    if(nodeNames.size() < 1)
        return;

    data->writeUInt(packetType_setNodeColors,packetTypeBits);
    data->writeUInt(serverID,dynamicObjectIDBits);
    data->writeUInt(chosenDecal,7);
    data->writeUInt(nodeNames.size(),7);
    for(int a = 0; a<nodeNames.size(); a++)
    {
        data->writeString(nodeNames[a]);
        data->writeUInt(nodeColors[a].x() * 255,8);
        data->writeUInt(nodeColors[a].y() * 255,8);
        data->writeUInt(nodeColors[a].z() * 255,8);
    }
}

//TODO: Add animations
void dynamicType::sendTypeToClient(serverClientHandle *client,int id)
{
    packet data;
    data.writeUInt(packetType_addDynamicType,packetTypeBits);
    data.writeUInt(id,10);
    data.writeString(filePath);
    data.writeUInt(standingFrame,8);
    data.writeFloat(eyeOffsetX);
    data.writeFloat(eyeOffsetY);
    data.writeFloat(eyeOffsetZ);
    data.writeFloat(networkScale.x());
    data.writeFloat(networkScale.y());
    data.writeFloat(networkScale.z());
    data.writeUInt(animations.size(),8);
    for(unsigned int a = 0; a<animations.size(); a++)
    {
        data.writeUInt(animations[a].startFrame,8);
        data.writeUInt(animations[a].endFrame,8);
        data.writeFloat(animations[a].speedDefault);
    }
    client->send(&data,true);
}

void dynamic::sendToClient(serverClientHandle *client)
{
    packet data;
    data.writeUInt(packetType_addRemoveDynamic,packetTypeBits);
    data.writeUInt(serverID,dynamicObjectIDBits);
    data.writeBit(true); //adding, not removing
    data.writeUInt(typeID,10);
    data.writeFloat(redTint);
    data.writeFloat(greenTint);
    data.writeFloat(blueTint);
    client->send(&data,true);
}

aiMatrix4x4 getCollisionTransformMatrix(const aiScene *scene,aiNode *node,aiMatrix4x4 startTransform,bool &foundColMesh)
{
    startTransform = startTransform * node->mTransformation;

    for(unsigned int a = 0; a<node->mNumMeshes; a++)
    {
        std::string name = scene->mMeshes[node->mMeshes[a]]->mName.C_Str();
        if(name == "Collision")
        {
            foundColMesh = true;
            return startTransform;
        }
    }
    for(unsigned int a = 0; a<node->mNumChildren; a++)
    {
        aiMatrix4x4 ret = getCollisionTransformMatrix(scene,node->mChildren[a],startTransform,foundColMesh);
        if(foundColMesh)
            return ret;
    }

    return aiMatrix4x4();
}

//Gets the folder from a full file path
//e.g. "assets/bob/bob.png" as filepath returns "assets/bob/"
std::string getFolderFromPath(std::string in)
{
    if(in.find("/") == std::string::npos)
        return "";
    std::string file = getFileFromPath(in);
    if(in.length() > file.length())
        in = in.substr(0,in.length() - file.length());
    return in;
}

void sayVec(btVector3 in)
{
    std::cout<<in.x()<<","<<in.y()<<","<<in.z()<<"\n";
}

dynamicType::dynamicType(std::string filePath,int id,btVector3 scale)
{
    networkScale = scale;
    dynamicTypeID = id;
    filePath = filePath;

    if(filePath.substr(filePath.length()-3,3) == "txt")
    {
        preferenceFile quick;
        quick.importFromFile(filePath);
        if(quick.getPreference("filepath"))
        {
            filePath = getFolderFromPath(filePath) + quick.getPreference("filepath")->toString();
        }
        else
        {
            error("Dynamic description file lacked filepath preference.");
            return;
        }
    }

    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(filePath,0);
    if(!scene)
    {
        error("Could not load model " + filePath);
        return;
    }

    aiVector3D aabbMaxAI,aabbMinAI;
    bool foundColMesh = false;

    for(unsigned int a = 0; a<scene->mNumMeshes; a++)
    {
        aiMesh *src = scene->mMeshes[a];
        std::string name = src->mName.C_Str();
        if(name.find("Collision") == std::string::npos)
            continue;
        foundColMesh = true;
        float maxX = src->mVertices[0].x;
        float maxY = src->mVertices[0].y;
        float maxZ = src->mVertices[0].z;
        float minX = src->mVertices[0].x;
        float minY = src->mVertices[0].y;
        float minZ = src->mVertices[0].z;
        for(unsigned int a = 0; a<src->mNumVertices; a++)
        {
            aiVector3D v = src->mVertices[a];
            if(v.x > maxX)
                maxX = v.x;
            if(v.y > maxY)
                maxY = v.y;
            if(v.z > maxZ)
                maxZ = v.z;
            if(v.x < minX)
                minX = v.x;
            if(v.y < minY)
                minY = v.y;
            if(v.z < minZ)
                minZ = v.z;
        }
        aabbMaxAI = aiVector3D(maxX,maxY,maxZ);
        aabbMinAI = aiVector3D(minX,minY,minZ);
        break;
    }

    if(!foundColMesh)
    {
        error("Could not find collision mesh on model!");
        return;
    }

    aiMatrix4x4 colTransform;

    bool foundIt = false;
    colTransform = getCollisionTransformMatrix(scene,scene->mRootNode,colTransform,foundIt);

    if(!foundIt)
    {
        error("No collision node found on model!");
        return;
    }

    aabbMaxAI = colTransform * aabbMaxAI;
    aabbMinAI = colTransform * aabbMinAI;

    if(aabbMinAI.x > aabbMaxAI.x)
        std::swap(aabbMinAI.x,aabbMaxAI.x);
    if(aabbMinAI.y > aabbMaxAI.y)
        std::swap(aabbMinAI.y,aabbMaxAI.y);
    if(aabbMinAI.z > aabbMaxAI.z)
        std::swap(aabbMinAI.z,aabbMaxAI.z);

    aabbMax = btVector3(aabbMaxAI.x,aabbMaxAI.y,aabbMaxAI.z);
    aabbMin = btVector3(aabbMinAI.x,aabbMinAI.y,aabbMinAI.z);

    btVector3 size = aabbMax-aabbMin;
    size /= 2.0;
    size *= scale;
    //defaultCollisionShape = new btBoxShape(size);

    defaultCollisionShape = new btCompoundShape();
    btTransform t;
    t.setIdentity();
    //t.setOrigin(aabbMin * btVector3(0.02,0.02,0.02));
    btVector3 offset = size+aabbMin*scale;
    t.setOrigin(offset);
    btBoxShape *box = new btBoxShape(size);
    defaultCollisionShape->addChildShape(t,box);
    //defaultCollisionShape->calculateLocalInertia(1.0,defaultIntertia);

    /*std::cout<<"Min: "<<aabbMin.x()<<","<<aabbMin.y()<<","<<aabbMin.z()<<"\n";
    std::cout<<"Max: "<<aabbMax.x()<<","<<aabbMax.y()<<","<<aabbMax.z()<<"\n";
    std::cout<<"Size: "<<size.x()<<","<<size.y()<<","<<size.z()<<"\n";
    std::cout<<"Offset: "<<offset.x()<<","<<offset.y()<<","<<offset.z()<<"\n";*/

    finalHalfExtents = size;
    finalOffset = offset;

    btScalar masses[1];
    masses[0] = 1.0;
    t.setIdentity();
    defaultCollisionShape->calculatePrincipalAxisTransform(masses,t,defaultIntertia);

    btTransform startTrans = btTransform::getIdentity();
    startTrans.setOrigin(btVector3(0,200,0));
    defaultMotionState = new btDefaultMotionState(startTrans);
    mass = 1.0;
}

unsigned int dynamic::getPacketBits()
{
    if(isPlayer)
        return 161 + dynamicObjectIDBits;
    else
        return 128 + dynamicObjectIDBits;
}

void dynamic::addToPacket(packet *data)
{
    data->writeUInt(serverID,dynamicObjectIDBits);
    data->writeBit(false);// is not item
    data->writeFloat(getWorldTransform().getOrigin().x());
    data->writeFloat(getWorldTransform().getOrigin().y());
    data->writeFloat(getWorldTransform().getOrigin().z());
    writeQuat(getWorldTransform().getRotation(),data);
    /*data->writeFloat(getWorldTransform().getRotation().x());
    data->writeFloat(getWorldTransform().getRotation().y());
    data->writeFloat(getWorldTransform().getRotation().z());
    data->writeFloat(getWorldTransform().getRotation().w());*/
    data->writeBit(isPlayer);
    if(isPlayer)
    {
        data->writeBit(walking);
        data->writeFloat(asin(-lastCamY) - (0.5*3.1415*crouchProgress));
    }
}

dynamic::dynamic(dynamicType *type_,btDynamicsWorld *world,int idServer,int idType)
: btRigidBody(type_->mass,type_->defaultMotionState,type_->defaultCollisionShape,type_->defaultIntertia)
{
    setUserPointer(this);
    setUserIndex(bodyUserIndex_dynamic);
    type = type_;
    serverID = idServer;
    typeID = idType;
    world->addRigidBody(this);
}

//Apparently only used in compileBrickCar
dynamic::dynamic(btScalar mass,btMotionState *ms,btCollisionShape *shape,btVector3 inertia,int idServer,btDynamicsWorld *world)
: btRigidBody(mass,ms,shape,inertia)
{
    setUserPointer(this);
    setUserIndex(bodyUserIndex_dynamic);
    serverID = idServer;
    typeID = -1;
    type = 0;
    world->addRigidBody(this);
}

void dynamic::makePlayer()
{
    setActivationState(DISABLE_DEACTIVATION);
    setFriction(1);
    setRestitution(0.5);
    setAngularFactor(btVector3(0,0,0));
    isPlayer = true;
}

btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world)
{
    btCollisionWorld::AllHitsRayResultCallback ground(start,end);
    world->rayTest(start,end,ground);

    float closestDist = 100;
    int closestIdx = -1;

    for(int a = 0; a<ground.m_collisionObjects.size(); a++)
    {
        float dist = (ground.m_hitPointWorld[a]-start).length();
        if(dist < closestDist)
        {
            closestIdx = a;
            closestDist = dist;
        }
    }

    if(closestIdx != -1)
        return (btRigidBody*)ground.m_collisionObjects[closestIdx];
    else
        return 0;
}

btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world,btRigidBody *ignore,btVector3 &hitPos)
{
    btCollisionWorld::AllHitsRayResultCallback ground(start,end);
    world->rayTest(start,end,ground);

    float closestDist = 100;
    int closestIdx = -1;

    for(int a = 0; a<ground.m_collisionObjects.size(); a++)
    {
        if(ground.m_collisionObjects[a] == ignore)
            continue;

        float dist = (ground.m_hitPointWorld[a]-start).length();
        if(dist < closestDist)
        {
            hitPos = ground.m_hitPointWorld[a];
            closestIdx = a;
            closestDist = dist;
        }
    }

    if(closestIdx != -1)
        return (btRigidBody*)ground.m_collisionObjects[closestIdx];
    else
        return 0;
}

btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world,btRigidBody *ignore,btVector3 &hitPos,btVector3 &normal)
{
    btCollisionWorld::AllHitsRayResultCallback ground(start,end);
    world->rayTest(start,end,ground);

    float closestDist = 100;
    int closestIdx = -1;

    for(int a = 0; a<ground.m_collisionObjects.size(); a++)
    {
        if(ground.m_collisionObjects[a] == ignore)
            continue;

        float dist = (ground.m_hitPointWorld[a]-start).length();
        if(dist < closestDist)
        {
            hitPos = ground.m_hitPointWorld[a];
            closestIdx = a;
            closestDist = dist;
            normal = ground.m_hitNormalWorld[a];
        }
    }

    if(closestIdx != -1)
        return (btRigidBody*)ground.m_collisionObjects[closestIdx];
    else
        return 0;
}

btRigidBody *getClosestBody(btVector3 start,btVector3 end,btDynamicsWorld *world,btVector3 &normal)
{
    btCollisionWorld::AllHitsRayResultCallback ground(start,end);
    world->rayTest(start,end,ground);

    float closestDist = 100;
    int closestIdx = -1;

    for(int a = 0; a<ground.m_collisionObjects.size(); a++)
    {
        float dist = (ground.m_hitPointWorld[a]-start).length();
        if(dist < closestDist)
        {
            closestIdx = a;
            closestDist = dist;
            normal = ground.m_hitNormalWorld[a];
        }
    }

    if(closestIdx != -1)
        return (btRigidBody*)ground.m_collisionObjects[closestIdx];
    else
        return 0;
}

void sayVec3(btVector3 in)
{
    std::cout<<std::fixed<<std::setprecision(4)<<in.x()<<","<<in.y()<<","<<in.z()<<"\n";
}

//Returns true if player has just jumped
bool dynamic::control(float yaw,bool forward,bool backward,bool left,bool right,bool jump,bool crouch,btDynamicsWorld *world,bool allowTurning,bool relativeSpeed,std::vector<btVector3> &colors,std::vector<btVector3> &poses)
{
    if(sittingOn)
    {
        forward = false;
        backward = false;
        left = false;
        right = false;
        jump = false;
    }

    bool sideWays = false;
    btVector3 walkVelocity = btVector3(0,0,0);
    btVector3 walkVelocityDontTouch = btVector3(0,0,0);
    if(forward)
    {
        walkVelocity += btVector3(sin(yaw),0,cos(yaw));
        walkVelocityDontTouch += btVector3(sin(yaw),0,cos(yaw));
    }
    if(backward)
    {
        walkVelocity += btVector3(sin(yaw+3.1415),0,cos(yaw+3.1415));
        walkVelocityDontTouch += btVector3(sin(yaw+3.1415),0,cos(yaw+3.1415));
    }
    if(left)
    {
        sideWays = true;
        walkVelocity += btVector3(sin(yaw-1.57),0,cos(yaw-1.57));
        walkVelocityDontTouch += btVector3(sin(yaw-1.57),0,cos(yaw-1.57));
    }
    if(right)
    {
        sideWays = true;
        walkVelocity += btVector3(sin(yaw+1.57),0,cos(yaw+1.57));
        walkVelocityDontTouch += btVector3(sin(yaw+1.57),0,cos(yaw+1.57));
    }

    int deltaMS = SDL_GetTicks() - lastPlayerControl;

    if(deltaMS > 60)
        deltaMS = 60;
    if(deltaMS < 0)
        deltaMS = 0;

    btTransform rotMatrix = getWorldTransform();
    rotMatrix.setOrigin(btVector3(0,0,0));
    btVector3 position = getWorldTransform().getOrigin();

    btRigidBody *crouchForcer = 0;
    if(!crouch && crouchProgress > 0.5)
    {
        btVector3 from = position + btVector3(0,0.1,0);
        btVector3 to = position + btVector3(0,type->finalHalfExtents.y(),0);
        btVector3 hitpos;
        crouchForcer = getClosestBody(from,to,world,this,hitpos);
    }

    if(crouch || !crouchForcer)
        crouchProgress += (crouch?1.0:-1.0) * ((float)deltaMS)/200.0;
    crouchProgress = std::clamp(crouchProgress,0.0f,1.0f);

    if(isJetting)
    {
        setGravity(btVector3(crouchProgress*20*sin(yaw),20*(1.0-crouchProgress),crouchProgress*20*cos(yaw)));
        //applyCentralForce(walkVelocity * 20);
    }
    else
    {
        btVector3 gravity = world->getGravity();
        setGravity(gravity);
    }

    float speed = 15.0;
    float blendTime = 50;

    speed *= (1.0-crouchProgress)*0.5 + 0.5;

    if(!type)
    {
        error("Tried to control (i.e. like a player) dynamic without a type (i.e. a vehicle)?");
        return false;
    }
    if(allowTurning)
    {
        float crouchRot = crouchProgress * -(3.1415/2.0);
        btTransform t = getWorldTransform();//3.1415/2.0
        t.setRotation(btQuaternion(3.1415 + yaw,crouchRot,0));
        setWorldTransform(t);
    }

    btVector3 normal;
    btRigidBody *ground = 0;

    if(crouchProgress > 0.5)
    {
        btVector3 from = position - btVector3(0,type->finalHalfExtents.z(),0) + btVector3(0,0.1,0);
        btVector3 to   = position - btVector3(0,type->finalHalfExtents.z(),0) - btVector3(0,0.1,0);
        btVector3 hitpos;
        ground = getClosestBody(from,to,world,this,hitpos,normal);
    }
    else
    {
        for(int gx = -1; gx<=1; gx++)
        {
            for(int gz = -1; gz<=1; gz++)
            {
                btVector3 lateralOffsets = btVector3(type->finalHalfExtents.x() * gx, 0, type->finalHalfExtents.z() * gz);
                lateralOffsets = rotMatrix * lateralOffsets;

                btVector3 hitpos;
                ground = getClosestBody(
                                        position+btVector3(0,1.0*crouchProgress+type->finalHalfExtents.y()*0.1,0),
                                        position - btVector3(lateralOffsets.x(),type->finalHalfExtents.y()*0.1,lateralOffsets.z()),
                                        world,
                                        this,
                                        hitpos,
                                        normal);

                if(ground)
                    break;
            }

            if(ground)
                break;
        }
    }

    lastPlayerControl = SDL_GetTicks();

    if(jump && ground && getLinearVelocity().y() < 1)
    {
        btVector3 vel = getLinearVelocity();
        vel.setY(45);
        setLinearVelocity(vel);
        return true;
    }

    if(walkVelocity.length2() < 0.01)
    {
        walking = false;
        setFriction(1.0);
        return false;
    }

    if(!ground)
    {
        if(isJetting)
        {
            speed *= 2.0;
            blendTime = 150;
        }
        else
            blendTime = 600;
        walkVelocity *= speed;
    }
    else
    {
        walking = true;
        setFriction(0.0);
        btVector3 halfWay = (normal+walkVelocity)/2.0;
        float dot = fabs(btDot(normal,walkVelocity));
        walkVelocity = halfWay * fabs(dot) + walkVelocity * (1.0-fabs(dot)); //1.0 = use half way, 0.0 = use normal vel
        if(dot > 0)
            walkVelocity.setY(-walkVelocity.getY());
        btVector3 oldVelNoY = walkVelocity.normalized();
        oldVelNoY.setY(0);
        oldVelNoY.normalized();
        walkVelocity = oldVelNoY * speed + btVector3(0,walkVelocity.getY() * speed,0);
    }

    float blendCoefficient = deltaMS / blendTime;
    btVector3 velocity = getLinearVelocity();
    btVector3 endVel = (1.0-blendCoefficient) * velocity + blendCoefficient * walkVelocity;
    endVel.setY(velocity.getY());

    if(isinf(endVel.x()) || isinf(endVel.y()) || isinf(endVel.z()) || isnanf(endVel.x()) || isnanf(endVel.y()) || isnanf(endVel.z()))
    {
        std::cout<<"Player walking bad result...\n";
        std::cout<<"Normal "; sayVec3(normal);
        std::cout<<"Walk vel "; sayVec3(walkVelocity);
        std::cout<<"End "; sayVec3(endVel);
        std::cout<<"\n";
    }
    else
        setLinearVelocity(endVel);

    if(!ground)
        return false;

    if(crouchProgress > 0.5)
        return false;

    btVector3 dir = walkVelocityDontTouch.normalize();
    btVector3 perpDir = walkVelocityDontTouch.cross(btVector3(0,1,0));

    /*colors.clear();
    poses.clear();

    colors.push_back(btVector3(1,1,1));
    poses.push_back(position);*/

    float bodyX = type->finalHalfExtents.x();
    float bodyZ = type->finalHalfExtents.z();
    if(sideWays)
        std::swap(bodyX,bodyZ);

    btVector3 footPos;
    btVector3 hitpos;
    btRigidBody *step;
    for(float footUp = 0; footUp <= 1.2; footUp += 0.3)
    {
        for(int across = -1; across <= 1; across++)
        {
            footPos = position + (2 * dir * bodyZ + perpDir * across * bodyX);
            footPos += btVector3(0,footUp,0);
            /*colors.push_back(btVector3(0,0,0));
            poses.push_back(footPos);*/

            step = getClosestBody(position,footPos,world,this,hitpos,normal);
            if(step)
            {
                if(btDot(normal,btVector3(0,1,0)) < 0.15)
                {
                    //float dist = (hitpos-position).length();
                    if(hitpos.y() <= position.y())
                        step = 0;
                    else
                        break;
                }
                else
                    step = 0;
            }
        }
        if(step)
            break;
    }

    if(step)
    {
        /*colors.push_back(btVector3(1,0,0));
        poses.push_back(hitpos);*/

        btVector3 rayStart = hitpos + btVector3(0,type->finalHalfExtents.y()*2,0) + walkVelocityDontTouch.normalized() * 0.1;

        /*colors.push_back(btVector3(1,1,0));
        poses.push_back(rayStart);*/

        btVector3 newHitPos;
        btRigidBody *secondCheck = getClosestBody(rayStart,hitpos,world,this,newHitPos);

        /*colors.push_back(btVector3(0,0,1));
        poses.push_back(newHitPos);*/

        if(secondCheck != step)
            return false;

        float stepUp = newHitPos.getY() - position.getY();
        //std::cout<<"Step up: "<<stepUp<<"\n";
        if(stepUp > 0 && stepUp < 1.75)
        {
            btTransform t = getWorldTransform();
            btVector3 pos = t.getOrigin();
            pos.setY(pos.getY()+stepUp+0.4);
            t.setOrigin(pos);
            setWorldTransform(t);
        }
    }


    return false;
}























