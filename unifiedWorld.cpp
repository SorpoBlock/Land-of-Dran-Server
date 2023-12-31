#include "code/unifiedWorld.h"

fileType discernExtension(std::string extension)
{
    //TODO: Add the rest of the files lol

    if(extension == ".wav" || extension == ".ogg" || extension == ".mod")
        return audioFile;
    else if(extension == ".blb" || extension == ".obj")
        return brickFile;
    else if(extension == ".png")
        return textureFile;
    else
        return unknownFile;
}

void unifiedWorld::sendEnvironmentToClient(clientData *client)
{
    packet data;
    data.writeUInt(packetType_serverCommand,packetTypeBits);
    data.writeString("environment");
    data.writeBit(!cycle.useDNC);
    if(!cycle.useDNC)
    {
        data.writeString(cycle.skyBoxPath);
        data.writeString(cycle.radiancePath);
        data.writeString(cycle.irradiancePath);
        data.writeFloat(sunDirection.x);
        data.writeFloat(sunDirection.y);
        data.writeFloat(sunDirection.z);
        data.writeFloat(cycle.dncFogColors[2].r);//day colors
        data.writeFloat(cycle.dncFogColors[2].g);//may not be loaded, but there are defaults set anyway
        data.writeFloat(cycle.dncFogColors[2].b);
    }
    else
    {
        /*unsigned char *asdf = new unsigned char[sizeof(dayNightCycle)];
        memcpy(asdf,&cycle,sizeof(dayNightCycle));
        data.writeString(asdf);
        for(int a = 0; a<sizeof(dayNightCycle); a++)
            std::cout<<(int)asdf[a]<<"-"<<(int)data.data[a]<<"\n";
        delete asdf;*/
        data.writeFloat(cycle.dawnStart);
        data.writeFloat(cycle.dawnEnd);
        data.writeFloat(cycle.duskStart);
        data.writeFloat(cycle.duskEnd);
        data.writeFloat(cycle.secondsInDay);

        for(int a = 0; a<4; a++)
        {
            data.writeFloat(cycle.dncSkyColors[a].r);
            data.writeFloat(cycle.dncSkyColors[a].g);
            data.writeFloat(cycle.dncSkyColors[a].b);

            data.writeFloat(cycle.dncFogColors[a].r);
            data.writeFloat(cycle.dncFogColors[a].g);
            data.writeFloat(cycle.dncFogColors[a].b);

            data.writeFloat(cycle.dncSunColors[a].r);
            data.writeFloat(cycle.dncSunColors[a].g);
            data.writeFloat(cycle.dncSunColors[a].b);
            data.writeFloat(cycle.dncSunColors[a].a);
        }
    }
    client->netRef->send(&data,true);
}

void clientData::forceTransformUpdate()
{
    if(!controlling)
        return;

    packet forceTransform;
    forceTransform.writeUInt(packetType_clientPhysicsData,packetTypeBits);
    forceTransform.writeUInt(3,2); //subtype three, force transform update (obvious lol)

    btTransform t = controlling->getWorldTransform();

    forceTransform.writeFloat(t.getOrigin().x());
    forceTransform.writeFloat(t.getOrigin().y());
    forceTransform.writeFloat(t.getOrigin().z());
    forceTransform.writeFloat(t.getRotation().w());
    forceTransform.writeFloat(t.getRotation().x());
    forceTransform.writeFloat(t.getRotation().y());
    forceTransform.writeFloat(t.getRotation().z());
    forceTransform.writeFloat(controlling->getLinearVelocity().x());
    forceTransform.writeFloat(controlling->getLinearVelocity().y());
    forceTransform.writeFloat(controlling->getLinearVelocity().z());
    forceTransform.writeBit(canJet);

    netRef->send(&forceTransform,true);
}

void unifiedWorld::radiusImpulse(btVector3 pos,float radius,float power,bool doDamage)
{
    btGhostObject *sphere = new btGhostObject();
    sphere->setUserPointer(this);
    sphere->setCollisionShape(new btSphereShape(radius));
    btTransform t = btTransform::getIdentity();
    t.setOrigin(pos);
    sphere->setWorldTransform(t);
    physicsWorld->addCollisionObject(sphere);
    btAlignedObjectArray<btCollisionObject*> pairs = sphere->getOverlappingPairs();

    for(int a = 0; a<pairs.size(); a++)
    {
        btCollisionObject *obj = pairs.at(a);
        if(!obj)
            continue;

        if(obj->getUserIndex() != 99)
            continue;

        btVector3 offset = obj->getWorldTransform().getOrigin() - pos;
        float dist = offset.length();
        dist = 1.0 - (dist / radius);

        obj->activate();
        ((btRigidBody*)obj)->setLinearVelocity(((btRigidBody*)obj)->getLinearVelocity() + offset.normalized() * dist * power);

        dynamic *player = (dynamic*)obj;

        if(doDamage)
            applyDamage(player,fabs(dist * power * 0.5),"explosion");

        if(!player->isPlayer)
            continue;

        for(int b = 0; b<users.size(); b++)
        {
            if(!users[b]->controlling)
                continue;
            if(users[b]->controlling == obj)
                users[b]->forceTransformUpdate();
        }
    }

    physicsWorld->removeCollisionObject(sphere);
    delete sphere;
    sphere = 0;
}

void clientData::setControlling(dynamic *player)
{
    if(controlling && (player != controlling))
    {
        controlling->isPlayer = false;
        packet removePhysicsData;
        removePhysicsData.writeUInt(packetType_clientPhysicsData,packetTypeBits);
        removePhysicsData.writeUInt(2,2); //subtype 2, delete client physics object (for old player if it exists)
        removePhysicsData.writeUInt(controlling->serverID,dynamicObjectIDBits);
        netRef->send(&removePhysicsData,true);
    }

    controlling = player;

    if(!controlling)
        return;

    controlling->isPlayer = true;

    packet physicsData;
    physicsData.writeUInt(packetType_clientPhysicsData,packetTypeBits);
    physicsData.writeUInt(0,2); //subtype 0, creating physics for a client to control
    physicsData.writeUInt(player->serverID,dynamicObjectIDBits);
    physicsData.writeFloat(player->type->finalHalfExtents.x());
    physicsData.writeFloat(player->type->finalHalfExtents.y());
    physicsData.writeFloat(player->type->finalHalfExtents.z());
    physicsData.writeFloat(player->type->finalOffset.x());
    physicsData.writeFloat(player->type->finalOffset.y());
    physicsData.writeFloat(player->type->finalOffset.z());
    physicsData.writeFloat(player->getWorldTransform().getOrigin().x());
    physicsData.writeFloat(player->getWorldTransform().getOrigin().y());
    physicsData.writeFloat(player->getWorldTransform().getOrigin().z());
    physicsData.writeBit(canJet);

    netRef->send(&physicsData,true);
}

void unifiedWorld::removeLight(light *l)
{
    if(l->attachedBrick)
        l->attachedBrick->attachedLight = 0;

    if(l->attachedCar)
    {
        for(int a = 0; a<l->attachedCar->lights.size(); a++)
        {
            if(l->attachedCar->lights[a] == l)
            {
                l->attachedCar->lights[a] = 0;
                l->attachedCar->lights.erase(l->attachedCar->lights.begin() + a);
                break;
            }
        }
    }

    for(int a = 0; a<users.size(); a++)
        l->removeFromClient(users[a]->netRef);

    for(int a = 0; a<lights.size(); a++)
    {
        if(lights[a] == l)
        {
            delete lights[a];
            lights[a] = 0;
            lights.erase(lights.begin() + a);
            return;
        }
    }
}

light *unifiedWorld::addLight(btVector3 color,brickCar *attachedCar,btVector3 offset,bool forgoSending)
{
    light *l = new light;
    l->serverID = lastLightID;
    lastLightID++;
    l->color = color;
    l->offset = offset;
    l->attachedCar = attachedCar;
    attachedCar->lights.push_back(l);

    if(!forgoSending)
    {
        for(int a = 0; a<users.size(); a++)
            l->sendToClient(users[a]->netRef);
    }

    lights.push_back(l);

    return l;
}

light *unifiedWorld::addLight(btVector3 color,btVector3 position,bool forgoSending)
{
    light *l = new light;
    l->serverID = lastLightID;
    lastLightID++;
    l->color = color;
    l->position = position;

    if(!forgoSending)
    {
        for(int a = 0; a<users.size(); a++)
            l->sendToClient(users[a]->netRef);
    }

    lights.push_back(l);

    return l;
}

light *unifiedWorld::addLight(btVector3 color,brick *attachedBrick,bool forgoSending)
{
    if(!attachedBrick)
        return 0;

    if(attachedBrick->attachedLight)
    {
        error("Brick already has light!");
        return 0;
    }

    light *l = new light;
    l->serverID = lastLightID;
    lastLightID++;
    l->color = color;
    l->attachedBrick = attachedBrick;
    attachedBrick->attachedLight = l;

    if(!forgoSending)
    {
        for(int a = 0; a<users.size(); a++)
            l->sendToClient(users[a]->netRef);
    }

    lights.push_back(l);

    return l;
}

void unifiedWorld::removeRope(rope *toRemove,bool removeFromVector)
{
    if(removeFromVector)
    {
        for(unsigned int a = 0; a<ropes.size(); a++)
        {
            if(ropes[a] == toRemove)
            {
                ropes.erase(ropes.begin() + a);
                break;
            }
        }
    }

    for(int a = 0; a<users.size(); a++)
        toRemove->removeFromClient(users[a]->netRef);

    toRemove->handle->removeAnchor(0);
    toRemove->handle->removeAnchor(toRemove->handle->m_nodes.size()-1);

    ((btSoftRigidDynamicsWorld*)physicsWorld)->removeSoftBody(toRemove->handle);
    delete toRemove;
    toRemove = 0;
}

rope *unifiedWorld::addRope(btRigidBody *a,btRigidBody *b,btVector3 posA,btVector3 posB,int links,int fixeds)
{
    rope *tmp = new rope;
    tmp->serverID = lastRopeID;
    tmp->anchorA = a;
    tmp->anchorB = b;
    lastRopeID++;

    /*if(useCenterPos)
        tmp->handle = btSoftBodyHelpers::CreateRope(softBodyWorldInfo,a->getWorldTransform().getOrigin()+btVector3(0,2.5,0),b->getWorldTransform().getOrigin()+btVector3(0,2.5,0),15,0);
    else*/
        tmp->handle = btSoftBodyHelpers::CreateRope(softBodyWorldInfo,posB,posA,links,fixeds);

    tmp->handle->setTotalMass(0.2);
    tmp->handle->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);

    tmp->handle->appendAnchor(0,a);
    tmp->handle->appendAnchor(tmp->handle->m_nodes.size() - 1,b);

    tmp->handle->m_cfg.piterations = 10;
    tmp->handle->m_cfg.kDP = 0.1;
    tmp->handle->m_cfg.kSHR = 1;
    tmp->handle->m_cfg.kCHR = 1;
    tmp->handle->m_cfg.kKHR = 1;
    tmp->handle->m_cfg.kDF = 1;
    tmp->handle->m_cfg.kDG = 1;
    tmp->handle->m_cfg.kVC = 1;
    /*tmp->handle->getCollisionShape()->setMargin(1.0);
    tmp->handle->getCollisionShape()->setLocalScaling(btVector3(1,1,1));
    tmp->handle->generateBendingConstraints(2);
    tmp->handle->randomizeConstraints();
    tmp->handle->setPose(true,true);

    tmp->handle->m_cfg.collisions = btSoftBody::fCollision::CL_SS + btSoftBody::fCollision::CL_RS + btSoftBody::fCollision::SDF_RS + btSoftBody::fCollision::VF_SS;*/

    ((btSoftRigidDynamicsWorld*)physicsWorld)->addSoftBody(tmp->handle);

    ropes.push_back(tmp);

    for(int a = 0; a<users.size(); a++)
        tmp->sendToClient(users[a]->netRef);

    return tmp;
}

emitterType *unifiedWorld::getEmitterType(std::string name)
{
    for(unsigned int a = 0; a<emitterTypes.size(); a++)
    {
        if(emitterTypes[a]->dbname == name)
            return emitterTypes[a];
    }
    return 0;
}

void clientData::sendPing()
{
    waitingOnPing = true;
    lastPingID++;
    lastPingSentAtMS = SDL_GetTicks();
    packet data;
    data.writeUInt(packetType_ping,packetTypeBits);
    data.writeUInt(lastPingID,32);
    netRef->send(&data,true);
}

void unifiedWorld::removeEmitter(emitter *e)
{
    if(!e)
        return;

    if(e->isJetEmitter)
    {
        for(int a = 0; a<users.size(); a++)
        {
            if(users[a]->leftJet == e)
                users[a]->leftJet = 0;
            if(users[a]->rightJet == e)
                users[a]->rightJet = 0;
            if(users[a]->paint == e)
                users[a]->paint = 0;
            if(users[a]->adminOrb == e)
                users[a]->adminOrb = 0;
        }
    }

    for(unsigned int a = 0; a<emitters.size(); a++)
    {
        if(emitters[a] == e)
        {
            packet data;
            data.writeUInt(packetType_emitterAddRemove,packetTypeBits);
            data.writeBit(false); //removing, not adding
            data.writeUInt(e->serverID,20);
            theServer->send(&data,true);

            delete e;
            e = 0;
            emitters.erase(emitters.begin() + a);

            return;
        }
    }

    error("How did an emitter get outside the emitters vector!");
}

emitter *unifiedWorld::addEmitter(emitterType *type,float x,float y,float z,bool dontSend)
{
    if(!type)
        return 0;
    scope("unifiedWorld::addEmitter");

    emitter *tmp = new emitter;
    tmp->creationTime = SDL_GetTicks();
    tmp->position = btVector3(x,y,z);
    tmp->type = type;
    tmp->serverID = lastEmitterID;
    lastEmitterID++;
    emitters.push_back(tmp);
    if(!dontSend)
    {
        for(unsigned int a = 0; a<users.size(); a++)
            tmp->sendToClient(users[a]->netRef);
    }
    return tmp;
}

void unifiedWorld::applyAppearancePrefs(clientData *client,dynamic *player)
{
    player->chosenDecal = client->wantedDecal;
    for(int a = 0; a<client->wantedNodeColors.size(); a++)
        player->setNodeColor(client->wantedNodeNames[a],client->wantedNodeColors[a],0);

    if(player->nodeNames.size() < 1)
        return;

    packet data;
    player->nodeColorsPacket(&data);
    theServer->send(&data,true);
}

void unifiedWorld::loadDecals(std::string searchFolder)
{
    for (recursive_directory_iterator i(searchFolder.c_str()), end; i != end; ++i)
    {
        if (!is_directory(i->path()))
        {
            if(i->path().extension() == ".png")
            {
                std::string name = i->path().filename().string();
                if(name.length() < 1)
                    continue;
                decalFilePaths.push_back(name);
            }
        }
    }
}

void unifiedWorld::sendDecals(serverClientHandle *target)
{
    int sentSoFar = 0;
    int leftToSend = decalFilePaths.size();

    while(leftToSend > 0)
    {
        int toSendThisTime = 0;
        int bytesLeft = 1370;
        while(toSendThisTime < leftToSend)
        {
            if(bytesLeft - (decalFilePaths[toSendThisTime + sentSoFar].length() + 3) > 0)
            {
                toSendThisTime++;
                bytesLeft -= decalFilePaths[toSendThisTime + sentSoFar].length() + 3;
            }
            else
                break;
        }

        packet data;
        data.writeUInt(packetType_serverCommand,packetTypeBits);
        data.writeString("sendDecals");
        data.writeUInt(toSendThisTime,7);

        for(int a = sentSoFar; a<sentSoFar+toSendThisTime; a++)
            data.writeString(decalFilePaths[a]);
        target->send(&data,true);

        sentSoFar += toSendThisTime;
        leftToSend -= toSendThisTime;
    }
}

void clientData::message(std::string text,std::string category)
{
    packet data;
    data.writeUInt(packetType_addMessage,packetTypeBits);
    data.writeUInt(0,2); //is chat message
    //data.writeBit(false); //is not bottom print
    data.writeString(text);
    data.writeString(category);
    netRef->send(&data,true);
}

void unifiedWorld::messageAll(std::string text,std::string category)
{
    packet data;
    data.writeUInt(packetType_addMessage,packetTypeBits);
    data.writeUInt(0,2); //is chat message
    //data.writeBit(false); //is not bottom print
    data.writeString(text);
    data.writeString(category);
    theServer->send(&data,true);
}

void unifiedWorld::playSound(std::string scriptName,float pitch,float vol)
{
    int idx = -1;
    for(unsigned int a = 0; a<soundScriptNames.size(); a++)
    {
        if(soundScriptNames[a] == scriptName)
        {
            idx = a;
            break;
        }
    }
    if(idx == -1)
    {
        error("Could not find sound " + scriptName);
        return;
    }

    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(idx,10);
    data.writeBit(false);
    data.writeBit(true);
    data.writeFloat(pitch);
    data.writeFloat(vol);
    theServer->send(&data,true);
}

void unifiedWorld::playSound(std::string scriptName,float pitch,float vol,clientData *target)
{
    int idx = -1;
    for(unsigned int a = 0; a<soundScriptNames.size(); a++)
    {
        if(soundScriptNames[a] == scriptName)
        {
            idx = a;
            break;
        }
    }
    if(idx == -1)
    {
        error("Could not find sound " + scriptName);
        return;
    }

    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(idx,10);
    data.writeBit(false);
    data.writeBit(true);
    data.writeFloat(pitch);
    data.writeFloat(vol);
    target->netRef->send(&data,true);
}

void unifiedWorld::setBrickColor(brick *theBrick,float r,float g,float b,float a)
{
    theBrick->r = r;
    theBrick->g = g;
    theBrick->b = b;
    theBrick->a = a;

    packet data;
    theBrick->createUpdatePacket(&data);
    theServer->send(&data,true);
}

void unifiedWorld::setBrickPosition(brick *theBrick,int x,int y,int z)
{
    theBrick->posX = x;
    theBrick->uPosY = y;
    theBrick->posZ = z;

    packet data;
    theBrick->createUpdatePacket(&data);
    theServer->send(&data,true);
}

void unifiedWorld::setBrickMaterial(brick *theBrick,int material)
{
    theBrick->material = material;

    packet data;
    theBrick->createUpdatePacket(&data);
    theServer->send(&data,true);
}

void unifiedWorld::setBrickAngleID(brick *theBrick,int angleID)
{
    if(angleID < 0)
        angleID = 0;
    if(angleID > 3)
        angleID = 3;

    theBrick->angleID = angleID;

    packet data;
    theBrick->createUpdatePacket(&data);
    theServer->send(&data,true);
}

void unifiedWorld::playSound(int soundId,float x,float y,float z,float pitch,float vol)
{
    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(soundId,10);
    data.writeBit(false);
    data.writeBit(false);
    data.writeBit(false);
    data.writeFloat(x);
    data.writeFloat(y);
    data.writeFloat(z);
    data.writeFloat(pitch);
    data.writeFloat(vol);
    theServer->send(&data,true);
}

void unifiedWorld::removeItem(item *toRemove)
{
    if(!toRemove)
        return;

    for(unsigned int a = 0; a<dynamics.size(); a++)
    {
        for(unsigned int b = 0; b<inventorySize; b++)
        {
            if(dynamics[a]->holding[b] == toRemove)
            {
                dropItem(dynamics[a],b,true);
            }
        }
    }

    for(unsigned int a = 0; a<items.size(); a++)
    {
        if(items[a] == toRemove)
        {
            items.erase(items.begin() + a);
            break;
        }
    }

    packet data;
    //data.writeUInt(packetType_removeItem,packetTypeBits);
    data.writeUInt(packetType_addRemoveItem,packetTypeBits);
    data.writeBit(false); //remove
    data.writeUInt(toRemove->serverID,dynamicObjectIDBits);
    theServer->send(&data,true);

    /*delete toRemove;
    toRemove = 0;*/
    removeDynamic(toRemove);
}

void unifiedWorld::playSound(std::string scriptName,float x,float y,float z,float pitch,float vol)
{
    int idx = -1;
    for(unsigned int a = 0; a<soundScriptNames.size(); a++)
    {
        if(soundScriptNames[a] == scriptName)
        {
            idx = a;
            break;
        }
    }
    if(idx == -1)
    {
        error("Could not find sound " + scriptName);
        return;
    }

    playSound(idx,x,y,z,pitch,vol);
}

void unifiedWorld::playSoundExcept(std::string scriptName,float x,float y,float z,clientData *except,float pitch,float vol)
{
    int idx = -1;
    for(unsigned int a = 0; a<soundScriptNames.size(); a++)
    {
        if(soundScriptNames[a] == scriptName)
        {
            idx = a;
            break;
        }
    }
    if(idx == -1)
    {
        error("Could not find sound " + scriptName);
        return;
    }

    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(idx,10);
    data.writeBit(false);
    data.writeBit(false);
    data.writeBit(false);
    data.writeFloat(x);
    data.writeFloat(y);
    data.writeFloat(z);
    data.writeFloat(pitch);
    data.writeFloat(vol);
    for(int a = 0; a<users.size(); a++)
        if(users[a] != except)
            users[a]->netRef->send(&data,true);
}

int unifiedWorld::startSoundLoop(int soundID,brickCar *mount,float pitch)
{
    if(!mount)
        return -1;
    if(soundID < 0 || soundID >= soundIsMusic.size())
        return -1;

    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(soundID,10);
    data.writeBit(true);
    data.writeUInt(currentMusicLoopID,24);
    data.writeFloat(pitch);
    data.writeBit(true);
    data.writeUInt(mount->serverID,10);
    theServer->send(&data,true);

    carMusicLoops.push_back({currentMusicLoopID,soundID,mount->serverID,pitch});

    currentMusicLoopID++;
    return currentMusicLoopID-1;
}

int unifiedWorld::startSoundLoop(int soundID,float x,float y,float z,float pitch)
{
    if(soundID < 0 || soundID >= soundIsMusic.size())
        return -1;

    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(soundID,10);
    data.writeBit(true);
    data.writeUInt(currentMusicLoopID,24);
    data.writeFloat(pitch);
    data.writeBit(false);
    data.writeFloat(x);
    data.writeFloat(y);
    data.writeFloat(z);
    theServer->send(&data,true);

    worldMusicLoops.push_back({currentMusicLoopID,soundID,x,y,z,pitch});

    currentMusicLoopID++;
    return currentMusicLoopID-1;
}

void unifiedWorld::stopSoundLoop(int loopID)
{
    packet data;
    data.writeUInt(packetType_serverCommand,packetTypeBits);
    data.writeString("stopLoop");
    data.writeUInt(loopID,24);
    theServer->send(&data,true);

    for(int a = 0; a<worldMusicLoops.size(); a++)
    {
        int id = std::get<0>(worldMusicLoops[a]);
        if(id == loopID)
        {
            worldMusicLoops.erase(worldMusicLoops.begin() + a);
            return;
        }
    }

    for(int a = 0; a<carMusicLoops.size(); a++)
    {
        int id = std::get<0>(carMusicLoops[a]);
        if(id == loopID)
        {
            carMusicLoops.erase(carMusicLoops.begin() + a);
            return;
        }
    }
}

/*void unifiedWorld::setMusic(brick *theBrick,int music)
{
    if(music <= 0)
    {
        if(!theBrick->music)
        {
            return;
        }

        theBrick->music = 0;

        for(unsigned int a = 0; a<maxMusicBricks; a++)
        {
            if(musicBricks[a] == theBrick)
            {
                if(theBrick->car)
                    loopSound(0,(brickCar*)theBrick->car,a);
                else
                    playSound(0,theBrick->getX(),theBrick->getY(),theBrick->getZ(),true,a);
                musicBricks[a] = 0;
                return;
            }
        }
        error("Tried to remove brick that wasn't in list of music bricks!");
        return;
    }

    int idx = -1;
    for(unsigned int a = 0; a<maxMusicBricks; a++)
    {
        if(!musicBricks[a])
        {
            idx = a;
            break;
        }
    }
    if(idx == -1)
    {
        error("Too many music bricks already!");
        return;
    }

    setMusic(theBrick,0);

    if(theBrick->car)
    {
        std::cout<<"Music brick is actually car...\n";
        loopSound(music,(brickCar*)theBrick->car,idx,theBrick->musicPitch);
    }
    else
        playSound(music,theBrick->getX(),theBrick->getY(),theBrick->getZ(),true,idx,theBrick->musicPitch);

    theBrick->music = music;
    musicBricks[idx] = theBrick;
}

void unifiedWorld::loopSound(int songId,brickCar *mount,int loopId,float pitch)
{
    packet data;
    data.writeUInt(packetType_playSound,packetTypeBits);
    data.writeUInt(songId,10);
    data.writeBit(true);
    data.writeUInt(loopId,5);
    data.writeFloat(pitch);
    data.writeBit(true);
    data.writeUInt(mount->serverID,10);
    theServer->send(&data,true);
}

void unifiedWorld::loopSound(std::string scriptName,brickCar *mount,int loopId)
{
    int idx = -1;
    for(unsigned int a = 0; a<soundScriptNames.size(); a++)
    {
        if(soundScriptNames[a] == scriptName)
        {
            idx = a;
            break;
        }
    }
    if(idx == -1)
    {
        error("Could not find music for car " + scriptName);
        return;
    }

    loopSound(idx,mount,loopId);
}*/

void unifiedWorld::addSoundType(std::string scriptName,std::string filePath)
{
    packet data;
    data.writeUInt(packetType_addSoundType,packetTypeBits);
    data.writeString(filePath);
    data.writeString(scriptName);
    data.writeUInt(soundFilePaths.size(),10);
    data.writeBit(false);
    theServer->send(&data,true);

    soundIsMusic.push_back(false);
    soundFilePaths.push_back(filePath);
    soundScriptNames.push_back(scriptName);
}

void unifiedWorld::addMusicType(std::string scriptName,std::string filePath)
{
    packet data;
    data.writeUInt(packetType_addSoundType,packetTypeBits);
    data.writeString(filePath);
    data.writeString(scriptName);
    data.writeUInt(soundFilePaths.size(),10);
    data.writeBit(true);
    theServer->send(&data,true);

    soundIsMusic.push_back(true);
    soundFilePaths.push_back(filePath);
    soundScriptNames.push_back(scriptName);
}

void clientData::bottomPrint(std::string text,int ms)
{
    packet data;
    data.writeUInt(packetType_addMessage,packetTypeBits);
    data.writeUInt(1,2); //is bottom print
    //data.writeBit(true);  //is bottom text
    data.writeString(text);
    data.writeUInt(ms,16);
    netRef->send(&data,true);
}

void unifiedWorld::pickUpItem(dynamic *holder,item* i,int slot,clientData *source)
{
    holder->pickUpItemFireCooldownStart = SDL_GetTicks();
    physicsWorld->removeRigidBody(i);
    i->heldBy = holder;
    i->switchedHolder = true;
    i->setHidden(true);
    holder->holding[slot] = i;
    i->otherwiseChanged = true;

    if(source)
    {
        packet data;
        data.writeUInt(packetType_setInventory,packetTypeBits);
        data.writeUInt(slot,3);
        data.writeBit(true);
        data.writeUInt(i->heldItemType->itemTypeID,10);
        source->netRef->send(&data,true);
    }
}

void unifiedWorld::dropItem(dynamic *holder,int slot,bool tryUpdateInventory)
{
    if(!holder)
        return;
    if(slot >= inventorySize)
        return;
    item *toDrop = holder->holding[slot];
    if(!toDrop)
        return;

    if(tryUpdateInventory)
    {
        for(unsigned int a= 0; a<users.size(); a++)
        {
            if(users[a]->controlling == holder)
            {
                packet data;
                data.writeUInt(packetType_setInventory,packetTypeBits);
                data.writeUInt(slot,3);
                data.writeBit(false);
                users[a]->netRef->send(&data,true);
                break;
            }
        }
    }

    btTransform t = toDrop->heldBy->getWorldTransform();
    btVector3 vel = btMatrix3x3(t.getRotation()) * btVector3(0,0,5);
    vel.setX(-vel.getX());
    float oldy = vel.getY();
    vel.setY(3);
    vel.setZ(-vel.getZ());
    t.setOrigin(t.getOrigin() + vel);
    vel.setY(oldy);
    toDrop->setWorldTransform(t);
    toDrop->setLinearVelocity(vel*2.5);

    physicsWorld->addRigidBody(toDrop);
    toDrop->activate();

    holder->holding[slot] = 0;
    toDrop->heldBy = 0;
    toDrop->switchedHolder = true;
    toDrop->otherwiseChanged = true;
}

void clientData::sendCameraDetails()
{
    packet data;
    data.writeUInt(packetType_cameraDetails,packetTypeBits);
    data.writeBit(cameraAdminOrb);
    data.writeBit(cameraBoundToDynamic && cameraTarget);
    if(cameraBoundToDynamic && cameraTarget)
    {
        data.writeUInt(cameraTarget->serverID,dynamicObjectIDBits);
        if(cameraTarget->sittingOn)
            data.writeBit(cameraTarget->sittingOn->getUserIndex() == bodyUserIndex_builtCar); //lean
        else
            data.writeBit(cameraTarget->getUserIndex() == bodyUserIndex_builtCar); //lean
    }
    else
    {
        data.writeFloat(camX);
        data.writeFloat(camY);
        data.writeFloat(camZ);
        data.writeBit(cameraFreelookEnabled);
        if(!cameraFreelookEnabled)
        {
            data.writeFloat(camDirX);
            data.writeFloat(camDirY);
            data.writeFloat(camDirZ);
        }
    }
    if(driving)
    {
        data.writeBit(true);
        data.writeUInt(driving->serverID,10);
    }
    else
        data.writeBit(false);
    netRef->send(&data,true);
}

glm::mat4 makeRotateMatrix(glm::vec3 axis, float angle)
{
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return glm::mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0,
                oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0,
                oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0,
                0.0,                                0.0,                                0.0,                                1.0);
}

glm::quat rotations[4] = {

glm::quat(0,0,1,0),
glm::quat(4.7122,0,1,0),
glm::quat(3.1415,0,1,0),
glm::quat(1.5708,0,1,0)};

void unifiedWorld::removeBrickCar(brickCar *toRemove)
{
    if(!toRemove)
        return;

    /*for(unsigned int a = 0; a<maxMusicBricks; a++)
    {
        if(toRemove->bricks[0] == musicBricks[a])
        {
            setMusic(toRemove->bricks[0],0);
            break;
        }
    }*/
    if(toRemove->musicLoopId != -1)
    {
        stopSoundLoop(toRemove->musicLoopId);
        toRemove->musicLoopId = -1;
    }

    /*for(int a = 0; a<toRemove->bricks.size(); a++)
    {
        if(toRemove->bricks[a]->attachedLight)
        {
            removeLight(toRemove->bricks[a]->attachedLight);
            toRemove->bricks[a]->attachedLight = 0;
        }
    }*/

    while(toRemove->lights.size() > 0)
        removeLight(toRemove->lights[0]);

    for(unsigned int a = 0; a<users.size(); a++)
    {
        if(users[a]->controlling)
        {
            if(users[a]->controlling->sittingOn == toRemove->body)
            {
                btTransform t = users[a]->controlling->getWorldTransform();
                btVector3 o = t.getOrigin();
                o.setY(o.getY() + 10.0);
                t.setOrigin(o);
                users[a]->controlling->setWorldTransform(t);
                users[a]->controlling->setLinearVelocity(users[a]->controlling->sittingOn->getLinearVelocity());

                users[a]->controlling->sittingOn = 0;
                users[a]->sendCameraDetails();
            }
        }
        if(users[a]->driving == toRemove)
        {
            users[a]->driving = 0;
            if(users[a]->controlling)
            {
                users[a]->setControlling(users[a]->controlling);
                users[a]->forceTransformUpdate();
                users[a]->controlling->setCollisionFlags(users[a]->controlling->oldCollisionFlags);
            }
        }
    }

    for(unsigned int i = 0; i<brickCars.size(); i++)
    {
        if(brickCars[i] == toRemove)
        {
            brickCars.erase(brickCars.begin() + i);
            break;
        }
    }

    physicsWorld->removeRigidBody(toRemove->body);
    physicsWorld->removeVehicle(toRemove->vehicle);

    packet data;
    data.writeUInt(packetType_removeBrickVehicle,packetTypeBits);
    data.writeUInt(toRemove->serverID,10);
    theServer->send(&data,true);

    delete toRemove;
}

bool unifiedWorld::compileBrickCar(brickCar *toAdd,float &heightCorrection,bool wheelsAlready,btVector3 origin)
{
    toAdd->serverID = lastBuiltVehicleID;
    lastBuiltVehicleID++;

    vehicleSettings vehiclePrefs;

    bool confirmedEvenRotation = false;
    bool confirmedOddRotation = false;

    brick *steeringWheel = 0;

    std::vector<light*> lightsToShiftUp;

    if(!wheelsAlready)
    {
        float minX = 999999;
        float minY = 999999;
        float minZ = 999999;
        float maxX = -999999;
        float maxY = -999999;
        float maxZ = -999999;

        for(unsigned int a = 0; a<toAdd->bricks.size(); a++)
        {
            brick *tmp = toAdd->bricks[a];

            if(tmp->isSpecial)
            {
                if(tmp->isSteeringWheel)
                {
                    if(steeringWheel)
                    {
                        std::cout<<"Too many steering wheels.\n";
                        return false;
                    }
                    else
                    {
                        vehiclePrefs = ((steeringBrick*)tmp)->steeringSettings;
                        steeringWheel = tmp;
                    }
                }

                if(tmp->isWheel)
                {
                    wheelData wheel = ((wheelBrick*)tmp)->wheelSettings;

                    wheel.offset = btVector3(tmp->getX(),tmp->getY(),tmp->getZ());
                    wheel.scale = btVector3(brickTypes->brickTypes[tmp->typeID]->width,brickTypes->brickTypes[tmp->typeID]->height,brickTypes->brickTypes[tmp->typeID]->length);
                    wheel.scale = wheel.scale / btVector3(2.0,2.0*2.5,2.0);
                    wheel.brickTypeID = tmp->typeID;
                    if(tmp->angleID == 0 || tmp->angleID == 2)
                    {
                        if(confirmedOddRotation)
                            return false;
                        confirmedEvenRotation = true;
                        wheel.wheelAxleCS = btVector3(-1, 0, 0);
                        //wheel.offset -= wheel.scale / 2.0;
                    }
                    else
                    {
                        if(confirmedEvenRotation)
                            return false;
                        confirmedOddRotation = true;
                        wheel.wheelAxleCS = btVector3(0, 0, -1);
                        //wheel.offset -= btVector3(wheel.scale.z(),wheel.scale.y(),wheel.scale.x()) / 2.0;
                    }

                    wheel.offset.setY(wheel.offset.getY() + wheel.suspensionLength);

                    /*wheel.brickX =      toAdd->bricks[a]->posX;
                    wheel.brickHalfX =  toAdd->bricks[a]->xHalfPosition;
                    wheel.brickY =      toAdd->bricks[a]->uPosY;
                    wheel.brickHalfY =  toAdd->bricks[a]->yHalfPosition;
                    wheel.brickZ =      toAdd->bricks[a]->posZ;
                    wheel.brickHalfZ =  toAdd->bricks[a]->zHalfPosition;
                    wheel.oldBrickDatablockName = brickTypes->brickTypes[toAdd->bricks[a]->typeID]->fileName;
                    wheel.angleID = toAdd->bricks[a]->angleID;*/
                    wheel.carX = wheel.offset.x();//toAdd->bricks[a]->getX();
                    wheel.carY = wheel.offset.y();//toAdd->bricks[a]->getY();
                    wheel.carZ = wheel.offset.z();//toAdd->bricks[a]->getZ();

                    toAdd->wheels.push_back(wheel);

                    delete toAdd->bricks[a];
                    toAdd->bricks[a] = 0;

                    continue;
                }
            }

            tmp->car = toAdd;
            glm::vec4 dims = glm::vec4(tmp->width,tmp->height,tmp->length,0.0);
            glm::quat rotDir = rotations[tmp->angleID];
            dims = makeRotateMatrix(glm::vec3(rotDir.x,rotDir.y,rotDir.z),rotDir.w) * dims;
            float x = dims.x / 2.0 + tmp->getX();
            float y = tmp->height / (2.0*2.5) + tmp->getY();
            float z = dims.z / 2.0 + tmp->getZ();
            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;

            x = -dims.x / 2.0 + tmp->getX();
            y = -tmp->height / (2.0*2.5) + tmp->getY();
            z = -dims.z / 2.0 + tmp->getZ();

            if(x < minX)
                minX = x;
            if(y < minY)
                minY = y;
            if(z < minZ)
                minZ = z;
            if(x > maxX)
                maxX = x;
            if(y > maxY)
                maxY = y;
            if(z > maxZ)
                maxZ = z;
        }

        if(!steeringWheel)
            return false;

        if(toAdd->wheels.size() > 24)
        {
            error("More than 2 dozen wheels on car!");
            return false;
        }

        if(steeringWheel->angleID % 2 && confirmedOddRotation)
            return false;
        if(steeringWheel->angleID % 2 == 0 && confirmedEvenRotation)
            return false;

        if(steeringWheel->angleID == 1)
            toAdd->builtBackwards = !toAdd->builtBackwards;

        auto iter = toAdd->bricks.begin();
        while(iter != toAdd->bricks.end())
        {
            if(!*iter)
                iter = toAdd->bricks.erase(iter);
            else
                ++iter;
        }

        float sizeX = maxX - minX;
        float sizeY = maxY - minY;
        float sizeZ = maxZ - minZ;

        float midX = minX + sizeX / 2.0;
        float midY = minY + sizeY / 2.0;
        float midZ = minZ + sizeZ / 2.0;

        std::cout<<"Min: "<<minX<<","<<minY<<","<<minZ<<"\n";
        std::cout<<"Max: "<<maxX<<","<<maxY<<","<<maxZ<<"\n";
        std::cout<<"Size: "<<sizeX<<","<<sizeY<<","<<sizeZ<<"\n";
        std::cout<<"Center: "<<midX<<","<<midY<<","<<midZ<<"\n";

        if(sizeX < 0)
        {
            error("SizeX of car < 0 what happened?");
            return false;
        }
        if(sizeY < 0)
        {
            error("SizeY of car < 0 what happened?");
            return false;
        }
        if(sizeZ < 0)
        {
            error("SizeZ of car < 0 what happened?");
            return false;
        }
        if(sizeX > 40)
        {
            error("SizeX of car > 40");
            return false;
        }
        if(sizeY > 40)
        {
            error("SizeY of car > 40");
            return false;
        }
        if(sizeZ > 40)
        {
            error("SizeZ of car > 40");
            return false;
        }

        for(unsigned int a = 0; a<toAdd->wheels.size(); a++)
        {
            toAdd->wheels[a].carX -= midX;
            toAdd->wheels[a].carY -= midY;
            toAdd->wheels[a].carZ -= midZ;
        }

        for(unsigned int a = 0; a<toAdd->bricks.size(); a++)
        {
            brick *tmp = toAdd->bricks[a];
            tmp->carX = tmp->getX() - midX;
            tmp->carY = tmp->getY() - midY;
            tmp->carZ = tmp->getZ() - midZ;

            if(tmp->attachedLight)
            {
                light *l = addLight(tmp->attachedLight->color,toAdd,btVector3(tmp->carX,tmp->carY,tmp->carZ),true);
                l->isSpotlight = tmp->attachedLight->isSpotlight;
                l->direction = tmp->attachedLight->direction;
                lightsToShiftUp.push_back(l);
                //for(int z=0;z<users.size(); z++)
                    //l->sendToClient(users[z]->netRef);
                removeLight(tmp->attachedLight);
            }
        }

        origin = btVector3(midX,midY,midZ);
    }
    else
    {
        confirmedOddRotation = true;

        for(unsigned int a = 0; a<toAdd->bricks.size(); a++)
        {
            brick *tmp = toAdd->bricks[a];
            tmp->car = toAdd;

            if(tmp->isSpecial)
            {
                if(tmp->isWheel)
                {
                    delete tmp;
                    toAdd->bricks[a] = 0;
                    continue;
                }

                if(tmp->isSteeringWheel)
                {
                    steeringWheel = tmp;
                    vehiclePrefs = ((steeringBrick*)tmp)->steeringSettings;
                    break;
                }
            }
        }
    }

    auto brickIter = toAdd->bricks.begin();
    while(brickIter != toAdd->bricks.end())
    {
        if((*brickIter) == 0)
            brickIter = toAdd->bricks.erase(brickIter);
        else
            ++brickIter;
    }

    if(!wheelsAlready)
    {
        unsigned int minYPos = 9999999;
        for(unsigned int a = 0 ; a<toAdd->bricks.size(); a++)
        {
            if(toAdd->bricks[a]->uPosY < minYPos)
                minYPos = toAdd->bricks[a]->uPosY;
        }

        for(unsigned int a = 0 ; a<toAdd->bricks.size(); a++)
            toAdd->bricks[a]->carPlatesUp = toAdd->bricks[a]->uPosY - minYPos;
    }

    if(steeringWheel)
        toAdd->wheelPosition = btVector3(steeringWheel->carX,steeringWheel->carY,steeringWheel->carZ);

    double totalWheelY=0;
    for(int a = 0; a<toAdd->wheels.size(); a++)
    {
        std::cout<<"Wheel offset: "<<toAdd->wheels[a].offset.getX()<<","<<toAdd->wheels[a].offset.getY()<<","<<toAdd->wheels[a].offset.getZ()<<"\n";
        totalWheelY += toAdd->wheels[a].carY;
    }
    totalWheelY /= ((double)toAdd->wheels.size());

    double totalBrickY=0;
    for(int a = 0; a<toAdd->bricks.size(); a++)
    {
        totalBrickY += toAdd->bricks[a]->carY;
    }
    totalBrickY /= ((double)toAdd->bricks.size());

    toAdd->baseLinearDamping = vehiclePrefs.angularDamping;

    heightCorrection = (totalWheelY - totalBrickY) + 0.4;
    if(vehiclePrefs.realisticCenterOfMass)
        heightCorrection = 0;

    for(unsigned int a = 0; a<lightsToShiftUp.size(); a++)
    {
        lightsToShiftUp[a]->offset.setY(lightsToShiftUp[a]->offset.y()+heightCorrection);
        for(int z=0;z<users.size(); z++)
            lightsToShiftUp[a]->sendToClient(users[z]->netRef);
    }

    for(int a = 0; a<toAdd->bricks.size(); a++)
        toAdd->bricks[a]->carY -= heightCorrection;
    for(int a = 0; a<toAdd->wheels.size(); a++)
    {
        toAdd->wheels[a].carY -= heightCorrection;
        toAdd->wheels[a].offset.setY(toAdd->wheels[a].offset.getY() - heightCorrection);
    }

    toAdd->wheelPosition -= btVector3(0,heightCorrection,0);

    btCompoundShape *wholeShape = new btCompoundShape();

    for(unsigned int a = 0; a<toAdd->bricks.size(); a++)
    {
        brick *tmp = toAdd->bricks[a];
        //std::cout<<tmp->carX<<","<<tmp->carY<<","<<tmp->carZ<<" car pos\n";
        /*tmp->x -= midX;
        tmp->y -= midY;
        tmp->z -= midZ;*/

        //std::cout<<"Dims: "<<tmp->width<<","<<tmp->height<<","<<tmp->length<<"\n";
        btBoxShape *partShape = brickTypes->collisionShapes->at(tmp->width,tmp->height,tmp->length);
        if(!partShape)
        {
            btVector3 dim = btVector3(((float)tmp->width)/2.0,((float)tmp->height)/(2.0*2.5),((float)tmp->length)/2.0);
            partShape = new btBoxShape(dim);
            brickTypes->collisionShapes->set(tmp->width,tmp->height,tmp->length,partShape);
        }

        float vx=tmp->carX,vy=tmp->carY,vz=tmp->carZ;

        if(tmp->angleID == 0 || tmp->angleID == 2)
        {
            vx -= tmp->width/2.0;
            vy -= tmp->height/2.0;
            vz -= tmp->length/2.0;
        }
        else
        {
            vz -= tmp->width/2.0;
            vy -= tmp->height/2.0;
            vx -= tmp->length/2.0;
        }
        btTransform startTrans = btTransform::getIdentity();

        if(tmp->angleID == 0 || tmp->angleID == 2)
            startTrans.setOrigin(btVector3(vx+(tmp->width/2.0),vy+(tmp->height/2.0),vz+(tmp->length/2.0)));
        else
        {
            startTrans.setOrigin(btVector3(vx+(tmp->length/2.0),vy+(tmp->height/2.0),vz+(tmp->width/2.0)));
            startTrans.setRotation(btQuaternion(1.57,0,0));
        }

        wholeShape->addChildShape(startTrans,partShape);
    }

    btVector3 inertia;
    /*if(vehiclePrefs.mass == 0 || vehiclePrefs.mass < 0)
    {*/
        btScalar *masses = new btScalar[toAdd->bricks.size()];
        for(unsigned int a = 0; a<toAdd->bricks.size(); a++)
            masses[a] = vehiclePrefs.mass;
        btTransform t;
        wholeShape->calculatePrincipalAxisTransform(masses,t,inertia);
        delete masses;
    /*}
    else
    {
        btVector3 inertia;
        float mass = vehiclePrefs.mass;
        wholeShape->calculateLocalInertia(mass,inertia);
    }*/


    btVector3 aabbMax,aabbMin;
    btTransform dontwantthis;
    wholeShape->getAabb(dontwantthis,aabbMin,aabbMax);
    toAdd->halfExtents = (aabbMax-aabbMin)*btScalar(0.5); //btVector3(sizeX/2.0,sizeY/2.0,sizeZ/2.0);
    /*btBoxShape *tmp = new btBoxShape(halfExtents);
    btVector3 inertia;
    float mass = 5;
    tmp->calculateLocalInertia(mass,inertia);*/


    btTransform startTrans = btTransform::getIdentity();
    startTrans.setOrigin(origin + btVector3(0,heightCorrection,0));
    btMotionState *ms = new btDefaultMotionState(startTrans);

    //toAdd->body = new dynamic(mass,ms,wholeShape,inertia,lastDynamicID,physicsWorld);
    toAdd->mass = ((float)toAdd->bricks.size());//*2.0;
    toAdd->body = new dynamic(toAdd->mass,ms,wholeShape,inertia,lastDynamicID,physicsWorld);
    if(toAdd->wheels.size() == 2)
    {
        /*std::cout<<"Setting gyro!\n";
        toAdd->body->setDamping(0,0.4);
        toAdd->body->setAngularFactor(0.6);
        toAdd->body->setFlags(BT_ENABLE_GYROSCOPIC_FORCE_EXPLICIT);*/
    }

    lastDynamicID++;
    //physicsWorld->addRigidBody(toAdd->body);
    toAdd->body->setUserIndex(bodyUserIndex_builtCar);
    toAdd->body->setUserPointer(toAdd);
    toAdd->body->setFriction(0.9);

    toAdd->compiled = true;

    toAdd->vehicleRayCaster = new btDefaultVehicleRaycaster(physicsWorld);
    toAdd->vehicle = new btRaycastVehicle(toAdd->tuning,toAdd->body,toAdd->vehicleRayCaster);

    if(confirmedEvenRotation || true)
        toAdd->vehicle->setCoordinateSystem(0,1,2);
    else
        toAdd->vehicle->setCoordinateSystem(2,1,0);

    toAdd->body->setLinearVelocity(btVector3(0,20,0));
    /*toAdd->body->setActivationState(DISABLE_DEACTIVATION);
    toAdd->body->setGravity(btVector3(0,0,0));*/

    //std::cout<<"Angular damping before: "<<toAdd->body->getAngularDamping()<<"\n";
    toAdd->body->setDamping(0,vehiclePrefs.angularDamping);
    physicsWorld->addVehicle(toAdd->vehicle);

    for(unsigned int a = 0; a<toAdd->wheels.size(); a++)
    {
        if(!wheelsAlready)
            toAdd->wheels[a].offset -= origin;
        float radius = toAdd->wheels[a].scale.y();
        //std::cout<<"Wheel scale: "<<toAdd->wheels[a].scale.x()<<","<<toAdd->wheels[a].scale.y()<<","<<toAdd->wheels[a].scale.z()<<"\n";
        toAdd->vehicle->addWheel(toAdd->wheels[a].offset, btVector3(0,-1,0), toAdd->wheels[a].wheelAxleCS, toAdd->wheels[a].suspensionLength, radius, toAdd->tuning, a > 1);

        btWheelInfo& wheel = toAdd->vehicle->getWheelInfo(a);
        wheel.m_suspensionStiffness = toAdd->wheels[a].suspensionStiffness;
        wheel.m_wheelsDampingCompression = toAdd->wheels[a].dampingCompression;//btScalar(0.8);
        wheel.m_wheelsDampingRelaxation = toAdd->wheels[a].dampingRelaxation;//1;
        wheel.m_frictionSlip = toAdd->wheels[a].frictionSlip;
        wheel.m_rollInfluence = toAdd->wheels[a].rollInfluence;
        toAdd->wheels[a].lastInWater = SDL_GetTicks();
    }
    //toAdd->addWheels(toAdd->halfExtents);

    brickCars.push_back(toAdd);

    for(unsigned int a = 0; a<users.size(); a++)
        toAdd->sendBricksToClient(users[a]->netRef);

    //toAdd->body->setAngularFactor(btVector3(1,0.5,1));

    int musicIdx = -1;
    for(unsigned int a = 0; a<soundScriptNames.size(); a++)
    {
        if(soundScriptNames[a] == "Engine")
        {
            musicIdx = a;
            break;
        }
    }

    if(musicIdx != -1)
    {
        toAdd->music = musicIdx;
        toAdd->musicPitch = 1.0;
        toAdd->musicLoopId = startSoundLoop(musicIdx,toAdd,toAdd->musicPitch);
    }

    return true;
}

void unifiedWorld::setShapeName(dynamic *toSet,std::string text,float r,float g,float b)
{
    toSet->shapeName = text;
    toSet->shapeNameR = r;
    toSet->shapeNameG = g;
    toSet->shapeNameB = b;

    packet data;
    data.writeUInt(packetType_setShapeName,packetTypeBits);
    data.writeUInt(toSet->serverID,dynamicObjectIDBits);
    data.writeString(toSet->shapeName);
    data.writeFloat(toSet->shapeNameR);
    data.writeFloat(toSet->shapeNameG);
    data.writeFloat(toSet->shapeNameB);
    theServer->send(&data,true);
}

bool StopAtFirstHit(brick *val){return false;}

bool unifiedWorld::addBrick(brick *theBrick,bool stopOverlaps,bool colliding,bool networking)
{
    if(theBrick->isSpecial)
    {
        theBrick->width = brickTypes->brickTypes[theBrick->typeID]->width;
        theBrick->height = brickTypes->brickTypes[theBrick->typeID]->height;
        theBrick->length = brickTypes->brickTypes[theBrick->typeID]->length;
    }

    int effectiveWidth = 1;
    int effectiveLength = 1;
    if(theBrick->angleID % 2 == 0)
    {
        effectiveWidth = theBrick->width;
        effectiveLength = theBrick->length;
    }
    else
    {
        effectiveWidth = theBrick->length;
        effectiveLength = theBrick->width;
    }

    if(theBrick->posX + 1 + effectiveWidth >= brickTreeSize || (theBrick->posX - effectiveWidth) - 1 <= -brickTreeSize)
    {
        std::cout<<"Didn't make it!\n";
        return false;
    }
    if(theBrick->uPosY + 1 + theBrick->height >= brickTreeSize)
        return false;
    float lowestY = theBrick->uPosY + (theBrick->yHalfPosition ? 0.5 : 0.0) - ((float)theBrick->height)/2.0;
    if(lowestY < 0)
        return false;
    if(theBrick->posZ + 1 + effectiveLength >= brickTreeSize || (theBrick->posZ - effectiveLength) - 1 <= -brickTreeSize)
    {
        std::cout<<"Didn't make it!\n";
        return false;
    }

    theBrick->serverID = lastBrickID;
    lastBrickID++;

    if(!theBrick->isWheel)
    {
        int fx = theBrick->posX + brickTreeSize;
        /*if(effectiveWidth % 2 == 0)
            fx++;*/
        int fy = ((float)theBrick->uPosY) + (theBrick->yHalfPosition ? 1.0 : 0.0) - ((float)theBrick->height)/2.0;
        int fz = theBrick->posZ + brickTreeSize;
        /*if(effectiveLength % 2 == 0)
            fz++;*/
        if(theBrick->angleID % 2 == 0)
        {
            fx -= ((float)theBrick->width)/2.0 - (theBrick->xHalfPosition ? 0.5 : 0.0);
            fz -= ((float)theBrick->length)/2.0 - (theBrick->zHalfPosition ? 0.5 : 0.0);
        }
        else
        {
            fz -= ((float)theBrick->width)/2.0 - (theBrick->zHalfPosition ? 0.5 : 0.0);
            fx -= ((float)theBrick->length)/2.0 - (theBrick->xHalfPosition ? 0.5 : 0.0);
        }

        if(stopOverlaps)
        {
            if(theBrick->angleID % 2 == 0)
            {
                double fxd = fx;
                double fyd = fy;
                double fzd = fz;

                double searchMin[3];
                searchMin[0] = fxd + 0.01;
                searchMin[1] = fyd + 0.01;
                searchMin[2] = fzd + 0.01;

                fxd += theBrick->width;
                fyd += theBrick->height;
                fzd += theBrick->length;

                double searchMax[3];
                searchMax[0] = fxd - 0.01;
                searchMax[1] = fyd - 0.01;
                searchMax[2] = fzd - 0.01;

                int hits = overlapTree->Search(searchMin,searchMax,StopAtFirstHit);
                if(hits > 0)
                    return false;
            }
            else
            {
                double fxd = fx;
                double fyd = fy;
                double fzd = fz;

                double searchMin[3];
                searchMin[0] = fxd + 0.01;
                searchMin[1] = fyd + 0.01;
                searchMin[2] = fzd + 0.01;

                fzd += theBrick->width;
                fyd += theBrick->height;
                fxd += theBrick->length;

                double searchMax[3];
                searchMax[0] = fxd - 0.01;
                searchMax[1] = fyd - 0.01;
                searchMax[2] = fzd - 0.01;

                int hits = overlapTree->Search(searchMin,searchMax,StopAtFirstHit);
                if(hits > 0)
                    return false;
            }
        }

        double insertMin[3];
        double insertMax[3];

        insertMin[0] = fx + 0.01;
        insertMin[1] = fy + 0.01;
        insertMin[2] = fz + 0.01;

        if(theBrick->angleID % 2 == 0)
        {
            insertMax[0] = fx + theBrick->width;
            insertMax[1] = fy + theBrick->height;
            insertMax[2] = fz + theBrick->length;
        }
        else
        {
            insertMax[0] = fx + theBrick->length;
            insertMax[1] = fy + theBrick->height;
            insertMax[2] = fz + theBrick->width;
        }

        insertMax[0] -= 0.01;
        insertMax[1] -= 0.01;
        insertMax[2] -= 0.01;

        overlapTree->Insert(insertMin,insertMax,theBrick);

    }

    brickTypes->addPhysicsToBrick(theBrick,physicsWorld);
    if(!colliding)
        theBrick->setColliding(physicsWorld,false);

    bricks.push_back(theBrick);

    if(networking)
    {
        packet data;
        data.writeUInt(packetType_addBricks,packetTypeBits);
        data.writeUInt(1,8);
        theBrick->addToPacket(&data);
        theServer->send(&data,true);
    }

    return true;
}

void unifiedWorld::clearBricks(clientData *source)
{
    for(int a = 0; a<source->ownedBricks.size(); a++)
        removeBrick(source->ownedBricks[a],true);
    source->ownedBricks.clear();

    /*int bricksToRemove = source->ownedBricks.size();
    unsigned int bricksRemoved = 0;

    if(bricksToRemove == 0)
        return;

    int bricksPerPacket = 255;
    while(bricksToRemove > 0)
    {
        int bricksInThisPacket = bricksPerPacket;
        if(bricksInThisPacket > bricksToRemove)
            bricksInThisPacket = bricksToRemove;

        packet data;
        data.writeUInt(packetType_removeBrick,packetTypeBits);
        data.writeUInt(bricksInThisPacket,8);

        for(unsigned int i = bricksRemoved; i < bricksRemoved + bricksInThisPacket; i++)
        {
            brick *theBrick = source->ownedBricks[i];

            for(int j = 0; j<bricks.size(); j++)
            {
                if(bricks[j] == theBrick)
                {
                    bricks.erase(bricks.begin() + j);
                    break;
                }
            }

            data.writeUInt(theBrick->serverID,20);

            //if(theBrick->music > 0)
              //  setMusic(theBrick,0);
            if(theBrick->musicLoopId != -1)
            {
                stopSoundLoop(theBrick->musicLoopId);
                theBrick->musicLoopId = -1;
            }

            if(theBrick->attachedLight)
            {
                removeLight(theBrick->attachedLight);
                theBrick->attachedLight = 0;
            }

            if(theBrick->body)
            {
                physicsWorld->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }

            setBrickName(theBrick,"");

            int fx = theBrick->posX + brickTreeSize;
            int fy = ((float)theBrick->uPosY) + (theBrick->yHalfPosition ? 1.0 : 0.0) - ((float)theBrick->height)/2.0;
            int fz = theBrick->posZ + brickTreeSize;
            if(theBrick->angleID % 2 == 0)
            {
                fx -= ((float)theBrick->width)/2.0 - (theBrick->xHalfPosition ? 0.5 : 0.0);
                fz -= ((float)theBrick->length)/2.0 - (theBrick->zHalfPosition ? 0.5 : 0.0);
            }
            else
            {
                fz -= ((float)theBrick->width)/2.0 - (theBrick->zHalfPosition ? 0.5 : 0.0);
                fx -= ((float)theBrick->length)/2.0 - (theBrick->xHalfPosition ? 0.5 : 0.0);
            }

            double insertMin[3];
            double insertMax[3];

            insertMin[0] = fx + 0.01;
            insertMin[1] = fy + 0.01;
            insertMin[2] = fz + 0.01;

            if(theBrick->angleID % 2 == 0)
            {
                insertMax[0] = fx + theBrick->width;
                insertMax[1] = fy + theBrick->height;
                insertMax[2] = fz + theBrick->length;
            }
            else
            {
                insertMax[0] = fx + theBrick->length;
                insertMax[1] = fy + theBrick->height;
                insertMax[2] = fz + theBrick->width;
            }

            insertMax[0] -= 0.01;
            insertMax[1] -= 0.01;
            insertMax[2] -= 0.01;

            overlapTree->Remove(insertMin,insertMax,theBrick);
        }

        theServer->send(&data,true);

        bricksRemoved += bricksInThisPacket;
        bricksToRemove -= bricksInThisPacket;
    }

    for(unsigned int a = 0; a<source->ownedBricks.size(); a++)
        if(source->ownedBricks[a])
            delete source->ownedBricks[a];

    source->ownedBricks.clear();*/
}

void unifiedWorld::removeBrick(brick *theBrick,bool skipOwnershipClear)
{
    if(!theBrick)
        return;

    if(theBrick->builtBy != -1 && !skipOwnershipClear)
    {
        for(unsigned int a = 0; a<users.size(); a++)
        {
            if(users[a]->accountID == (unsigned int)theBrick->builtBy)
            {
                for(unsigned int b = 0; b<users[a]->ownedBricks.size(); b++)
                {
                    if(users[a]->ownedBricks[b] == theBrick)
                    {
                        users[a]->ownedBricks.erase(users[a]->ownedBricks.begin() + b);
                        break;
                    }
                }

                break;
            }
        }
    }

    if(theBrick->body)
    {
        auto ropeIter = ropes.begin();
        while(ropeIter != ropes.end())
        {
            rope *tmp = *ropeIter;
            if(tmp->anchorA == theBrick->body || tmp->anchorB == theBrick->body)
            {
                removeRope(tmp,false);
                tmp = 0;
                ropeIter = ropes.erase(ropeIter);
            }
            else
                ++ropeIter;
        }
    }

    if(theBrick->attachedLight)
    {
        removeLight(theBrick->attachedLight);
        theBrick->attachedLight = 0;
    }

    setBrickName(theBrick,"");

    if(theBrick->musicLoopId != -1)
    {
        stopSoundLoop(theBrick->musicLoopId);
        theBrick->musicLoopId = -1;
    }
    /*if(theBrick->music > 0)
        setMusic(theBrick,0);*/

    if(theBrick->body)
    {
        physicsWorld->removeRigidBody(theBrick->body);
        delete theBrick->body;
        theBrick->body = 0;
    }

    int fx = theBrick->posX + brickTreeSize;
    int fy = ((float)theBrick->uPosY) + (theBrick->yHalfPosition ? 1.0 : 0.0) - ((float)theBrick->height)/2.0;
    int fz = theBrick->posZ  + brickTreeSize;
    if(theBrick->angleID % 2 == 0)
    {
        fx -= ((float)theBrick->width)/2.0 - (theBrick->xHalfPosition ? 0.5 : 0.0);
        fz -= ((float)theBrick->length)/2.0 - (theBrick->zHalfPosition ? 0.5 : 0.0);
    }
    else
    {
        fz -= ((float)theBrick->width)/2.0 - (theBrick->zHalfPosition ? 0.5 : 0.0);
        fx -= ((float)theBrick->length)/2.0 - (theBrick->xHalfPosition ? 0.5 : 0.0);
    }

    double insertMin[3];
    double insertMax[3];

    insertMin[0] = fx + 0.01;
    insertMin[1] = fy + 0.01;
    insertMin[2] = fz + 0.01;

    if(theBrick->angleID % 2 == 0)
    {
        insertMax[0] = fx + theBrick->width;
        insertMax[1] = fy + theBrick->height;
        insertMax[2] = fz + theBrick->length;
    }
    else
    {
        insertMax[0] = fx + theBrick->length;
        insertMax[1] = fy + theBrick->height;
        insertMax[2] = fz + theBrick->width;
    }

    insertMax[0] -= 0.01;
    insertMax[1] -= 0.01;
    insertMax[2] -= 0.01;

    overlapTree->Remove(insertMin,insertMax,theBrick);

    playSound("BrickBreak",theBrick->getX(),theBrick->getY(),theBrick->getZ());

    packet data;
    data.writeUInt(packetType_removeBrick,packetTypeBits);
    data.writeUInt(1,8);
    data.writeUInt(theBrick->serverID,20);

    theServer->send(&data,true);
    for(unsigned int a = 0; a<bricks.size(); a++)
    {
        if(bricks[a] == theBrick)
        {
            bricks.erase(bricks.begin() + a);
            break;
        }
    }
    delete theBrick;
}

void unifiedWorld::removeDynamic(dynamic *toRemove,bool dontSendPacket)
{
    if(toRemove->isProjectile)
    {
        for(int a = 0; a<projectiles.size(); a++)
        {
            if(projectiles[a] == toRemove)
            {
                projectiles.erase(projectiles.begin() + a);
                break;
            }
        }
    }

    for(unsigned int a = 0; a<items.size(); a++)
    {
        if(items[a] == toRemove)
        {
            error("You can't remove an item with removeDynamic");
            return;
        }
    }

    auto ropeIter = ropes.begin();
    while(ropeIter != ropes.end())
    {
        rope *tmp = *ropeIter;
        if(tmp->anchorA == toRemove || tmp->anchorB == toRemove)
        {
            removeRope(tmp,false);
            tmp = 0;
            ropeIter = ropes.erase(ropeIter);
        }
        else
            ++ropeIter;
    }

    for(unsigned int a = 0; a<inventorySize; a++)
    {
        if(toRemove->holding[a])
        {
            removeItem(toRemove->holding[a]);
        }
    }

    for(unsigned int a = 0; a<users.size(); a++)
    {
        if(users[a]->controlling == toRemove)
        {
            users[a]->setControlling(0);
        }
    }

    for(unsigned int a = 0; a<dynamics.size(); a++)
    {
        if(dynamics[a]->sittingOn == toRemove)
        {
            dynamics[a]->sittingOn = 0;
        }
    }

    for(unsigned int a = 0; a<dynamics.size(); a++)
    {
        if(dynamics[a] == toRemove)
        {
            dynamics.erase(dynamics.begin() + a);
            break;
        }
    }

    auto emitterIter = emitters.begin();
    while(emitterIter != emitters.end())
    {
        emitter *e = *emitterIter;

        if(!e)
        {
            ++emitterIter;
            continue;
        }

        if(e->attachedToModel == toRemove)
        {
            if(e->isJetEmitter)
            {
                for(int a = 0; a<users.size(); a++)
                {
                    if(users[a]->leftJet == e)
                        users[a]->leftJet = 0;
                    if(users[a]->rightJet == e)
                        users[a]->rightJet = 0;
                    if(users[a]->paint == e)
                        users[a]->paint = 0;
                    if(users[a]->adminOrb == e)
                        users[a]->adminOrb = 0;
                }
            }

            packet data;
            data.writeUInt(packetType_emitterAddRemove,packetTypeBits);
            data.writeBit(false); //removing, not adding
            data.writeUInt(e->serverID,20);
            theServer->send(&data,true);

            delete e;
            e = 0;
            emitterIter = emitters.erase(emitterIter);

            continue;
        }

        ++emitterIter;
    }

    int id = toRemove->serverID;
    physicsWorld->removeRigidBody(toRemove);
    delete toRemove;

    if(!dontSendPacket)
    {
        packet data;
        //data.writeUInt(packetType_removeDynamic,packetTypeBits);
        data.writeUInt(packetType_addRemoveDynamic,packetTypeBits);
        data.writeUInt(id,dynamicObjectIDBits);
        data.writeBit(false); //Removing, not adding
        theServer->send(&data,true);
    }
}

void unifiedWorld::loadLodSave(std::string filePath,std::vector<brick*> &loadedBricks,int xOffset,int yOffset,int zOffset)
{
    int start = SDL_GetTicks();
    std::ifstream bls(filePath.c_str(),std::ios::binary);

    if(!bls.is_open())
    {
        error("Could not open save file " + filePath);
        return;
    }
    else
        debug("Opened save file!");

    unsigned int uIntBuf = 0;
    float floatBuf = 0;
    unsigned char charBuf = 0;


    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    bool isNextVersion = uIntBuf == (landOfDranBuildMagic + 1);
    if(uIntBuf != landOfDranBuildMagic && !isNextVersion)
    {
        error("Invalid Land of Dran binary save file or outdated version.");
        bls.close();
        return;
    }

    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    int toLoad = uIntBuf;
    info("Loading " + std::to_string(uIntBuf) + " bricks...");

    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    int specialBrickTypes = uIntBuf;
    debug("Save file has " + std::to_string(specialBrickTypes) + " special brick types.");

    std::vector<int> saveToServerBrickTypeIDs;

    for(int a = 0; a<specialBrickTypes; a++)
    {
        bls.read((char*)&charBuf,sizeof(unsigned char));
        if(charBuf == 0)
        {
            error("Invalid string length for special brick type in binary save file!");
            continue;
        }

        char *dbString = new char[charBuf+1];
        dbString[charBuf] = 0;
        bls.read(dbString,charBuf);
        std::string db = lowercase(dbString);

        bool foundIt = false;
        for(unsigned int b = 0; b<brickTypes->brickTypes.size(); b++)
        {
            std::string dbName = lowercase(getFileFromPath(brickTypes->brickTypes[b]->fileName));
            if(dbName.length() > 4)
            {
                if(dbName.substr(dbName.length()-4,4) == ".blb")
                    dbName = dbName.substr(0,dbName.length()-4);
            }

            if(brickTypes->brickTypes[b]->uiname == db)
            {
                foundIt = true;
                saveToServerBrickTypeIDs.push_back(b);
                break;
            }
            else if(dbName == db)
            {
                foundIt = true;
                saveToServerBrickTypeIDs.push_back(b);
                break;
            }
        }

        if(!foundIt)
        {
            saveToServerBrickTypeIDs.push_back(-1);
            error("Could not find brick type with dbName " + db);
        }

        delete dbString;
    }

    int brickCount = 0;

    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    int opaqueBasicBricks = uIntBuf;

    for(int a = 0; a<opaqueBasicBricks; a++)
    {
        unsigned char r,g,b,alpha;
        bls.read((char*)&r,sizeof(unsigned char));
        bls.read((char*)&g,sizeof(unsigned char));
        bls.read((char*)&b,sizeof(unsigned char));
        bls.read((char*)&alpha,sizeof(unsigned char));
        //std::cout<<"OB Alpha: "<<(int)alpha;

        float x,y,z;
        bls.read((char*)&x,sizeof(float));
        bls.read((char*)&y,sizeof(float));
        bls.read((char*)&z,sizeof(float));

        unsigned char width,height,length,oldflags;
        bls.read((char*)&width,sizeof(unsigned char));
        bls.read((char*)&height,sizeof(unsigned char));
        bls.read((char*)&length,sizeof(unsigned char));
        bls.read((char*)&oldflags,sizeof(unsigned char));

        unsigned char angleID,material;
        bls.read((char*)&angleID,sizeof(unsigned char));
        bls.read((char*)&material,sizeof(unsigned char));

        bool isColliding = true;

        bool hasMusic = false;
        float musicPitch = 1.0;
        unsigned int musicTrack = 0;

        bool hasLight = false;
        float lightR,lightG,lightB;

        bool hasPrint = false;
        int printMask = 0;
        std::string printName = "";

        int ownerID = -1;
        std::string brickName = "";

        if(isNextVersion)
        {
            bls.read((char*)&ownerID,sizeof(int));

            bls.read((char*)&charBuf,sizeof(unsigned char));
            if(charBuf > 0)
            {
                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                brickName = std::string(tempName);
                delete tempName;
            }

            unsigned char flags;
            bls.read((char*)&flags,sizeof(unsigned char));
            isColliding = flags & 1;
            hasMusic = flags & 2;
            hasLight = flags & 4;
            hasPrint = flags & 8;

            if(hasMusic)
            {
                bls.read((char*)&musicTrack,sizeof(unsigned int));
                bls.read((char*)&musicPitch,sizeof(float));
            }

            if(hasLight)
            {
                bls.read((char*)&lightR,sizeof(float));
                bls.read((char*)&lightG,sizeof(float));
                bls.read((char*)&lightB,sizeof(float));
            }

            if(hasPrint)
            {
                bls.read((char*)&printMask,sizeof(unsigned char));
                bls.read((char*)&charBuf,sizeof(unsigned char));

                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                printName = std::string(tempName);
                delete tempName;
            }
        }

        if(width == 0 || height == 0 || length == 0)
        {
            error("ob Brick had dimension of 0, error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        if(angleID > 3)
        {
            error("ob Brick had angleID of " + std::to_string((int)angleID) + " error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        if(x < -16000 || x > 16000 || y < - 16000 || y > 16000 || z < -16000 || z > 16000)
        {
            error("ob Brick had position of " + std::to_string(x)+","+std::to_string(y)+","+std::to_string(z)+" this is out of bounds! Brick: " + std::to_string(brickCount));
            continue;
        }

        brick *tmp = new brick;

        tmp->posX = floor(x);
        tmp->xHalfPosition = fabs(floor(x) - x) > 0.25;

        float platesHigh = (y/1.2)*3.0;
        tmp->uPosY = floor(platesHigh);
        tmp->yHalfPosition = fabs(floor(platesHigh) - platesHigh) > 0.25;
        tmp->uPosY += 1;

        tmp->posZ = floor(z);
        tmp->zHalfPosition = fabs(floor(z) - z) > 0.25;

        tmp->posX += xOffset;
        tmp->uPosY += yOffset;
        tmp->posZ += zOffset;

        tmp->r = r;
        tmp->g = g;
        tmp->b = b;
        tmp->a = alpha;

        tmp->r /= 255.0;
        tmp->g /= 255.0;
        tmp->b /= 255.0;
        tmp->a /= 255.0;

        //std::cout<<" "<<tmp->a<<"\n";

        tmp->angleID = angleID;
        tmp->material = material;

        tmp->width = width;
        tmp->height = height;
        tmp->length = length;

        //tmp->printMask = flags;

        if(hasPrint)
        {
            for(unsigned int a = 0; a<brickTypes->printNames.size(); a++)
                if(brickTypes->printNames[a] == printName)
                    tmp->printID = a;
            tmp->printMask = printMask;
            tmp->printName = printName;
        }

        loadedBricks.push_back(tmp);
        addBrick(tmp,false,isColliding,false);
        brickCount++;

        if(brickName.length() > 0)
            setBrickName(tmp,brickName);
        tmp->builtBy = ownerID;
        if(hasLight)
            addLight(btVector3(lightR,lightG,lightB),tmp);

        if(hasMusic)
        {
            tmp->musicPitch = musicPitch;
            tmp->music = musicTrack;
            tmp->musicLoopId = startSoundLoop(musicTrack,tmp->getX(),tmp->getY(),tmp->getZ(),musicPitch);
        }

        if(brickCount > toLoad)
        {
            error("Loaded too many bricks? OB val: " + std::to_string(opaqueBasicBricks));
            break;
        }
    }

    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    int transparentBasicBricks = uIntBuf;

    for(int a = 0; a<transparentBasicBricks; a++)
    {
        unsigned char r,g,b,alpha;
        bls.read((char*)&r,sizeof(unsigned char));
        bls.read((char*)&g,sizeof(unsigned char));
        bls.read((char*)&b,sizeof(unsigned char));
        bls.read((char*)&alpha,sizeof(unsigned char));
        //std::cout<<"TB alpha: "<<(int)alpha;

        float x,y,z;
        bls.read((char*)&x,sizeof(float));
        bls.read((char*)&y,sizeof(float));
        bls.read((char*)&z,sizeof(float));

        unsigned char width,height,length,oldflags;
        bls.read((char*)&width,sizeof(unsigned char));
        bls.read((char*)&height,sizeof(unsigned char));
        bls.read((char*)&length,sizeof(unsigned char));
        bls.read((char*)&oldflags,sizeof(unsigned char));

        unsigned char angleID,material;
        bls.read((char*)&angleID,sizeof(unsigned char));
        bls.read((char*)&material,sizeof(unsigned char));

        bool isColliding = true;

        bool hasMusic = false;
        float musicPitch = 1.0;
        unsigned int musicTrack = 0;

        bool hasLight = false;
        float lightR,lightG,lightB;

        bool hasPrint = false;
        int printMask = 0;
        std::string printName = "";

        int ownerID = -1;
        std::string brickName = "";

        if(isNextVersion)
        {
            bls.read((char*)&ownerID,sizeof(int));

            bls.read((char*)&charBuf,sizeof(unsigned char));
            if(charBuf > 0)
            {
                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                brickName = std::string(tempName);
                delete tempName;
            }

            unsigned char flags;
            bls.read((char*)&flags,sizeof(unsigned char));
            isColliding = flags & 1;
            hasMusic = flags & 2;
            hasLight = flags & 4;
            hasPrint = flags & 8;

            if(hasMusic)
            {
                bls.read((char*)&musicTrack,sizeof(unsigned int));
                bls.read((char*)&musicPitch,sizeof(float));
            }

            if(hasLight)
            {
                bls.read((char*)&lightR,sizeof(float));
                bls.read((char*)&lightG,sizeof(float));
                bls.read((char*)&lightB,sizeof(float));
            }

            if(hasPrint)
            {
                bls.read((char*)&printMask,sizeof(unsigned char));
                bls.read((char*)&charBuf,sizeof(unsigned char));

                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                printName = std::string(tempName);
                delete tempName;
            }
        }

        if(width == 0 || height == 0 || length == 0)
        {
            error("tb Brick had dimension of 0, error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        if(angleID > 3)
        {
            error("tb Brick had angleID of " + std::to_string((int)angleID) + " error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        if(x < -16000 || x > 16000 || y < - 16000 || y > 16000 || z < -16000 || z > 16000)
        {
            error("tb Brick had position of " + std::to_string(x)+","+std::to_string(y)+","+std::to_string(z)+" this is out of bounds! Brick: " + std::to_string(brickCount));
            continue;
        }

        brick *tmp = new brick;

        tmp->posX = floor(x);
        tmp->xHalfPosition = fabs(floor(x) - x) > 0.25;

        float platesHigh = (y/1.2)*3.0;
        tmp->uPosY = floor(platesHigh);
        tmp->yHalfPosition = fabs(floor(platesHigh) - platesHigh) > 0.25;
        tmp->uPosY += 1;

        tmp->posZ = floor(z);
        tmp->zHalfPosition = fabs(floor(z) - z) > 0.25;

        tmp->posX += xOffset;
        tmp->uPosY += yOffset;
        tmp->posZ += zOffset;

        tmp->r = r;
        tmp->g = g;
        tmp->b = b;
        tmp->a = alpha;

        tmp->r /= 255.0;
        tmp->g /= 255.0;
        tmp->b /= 255.0;
        tmp->a /= 255.0;
        //std::cout<<" "<<tmp->a<<"\n";

        tmp->angleID = angleID;
        tmp->material = material;

        tmp->width = width;
        tmp->height = height;
        tmp->length = length;

        //tmp->printMask = flags;

        if(hasPrint)
        {
            for(unsigned int a = 0; a<brickTypes->printNames.size(); a++)
                if(brickTypes->printNames[a] == printName)
                    tmp->printID = a;
            tmp->printMask = printMask;
            tmp->printName = printName;
        }

        loadedBricks.push_back(tmp);
        addBrick(tmp,false,isColliding,false);
        brickCount++;

        if(brickName.length() > 0)
            setBrickName(tmp,brickName);
        tmp->builtBy = ownerID;
        if(hasLight)
            addLight(btVector3(lightR,lightG,lightB),tmp);

        if(hasMusic)
        {
            tmp->musicPitch = musicPitch;
            tmp->music = musicTrack;
            tmp->musicLoopId = startSoundLoop(musicTrack,tmp->getX(),tmp->getY(),tmp->getZ(),musicPitch);
        }

        if(brickCount > toLoad)
        {
            error("Loaded too many bricks? TB val: " + std::to_string(transparentBasicBricks));
            break;
        }
    }

    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    int opaqueSpecialBricks = uIntBuf;

    for(int a = 0; a<opaqueSpecialBricks; a++)
    {
        unsigned char r,g,b,alpha;
        bls.read((char*)&r,sizeof(unsigned char));
        bls.read((char*)&g,sizeof(unsigned char));
        bls.read((char*)&b,sizeof(unsigned char));
        bls.read((char*)&alpha,sizeof(unsigned char));

        float x,y,z;
        bls.read((char*)&x,sizeof(float));
        bls.read((char*)&y,sizeof(float));
        bls.read((char*)&z,sizeof(float));

        unsigned int saveTypeID;
        bls.read((char*)&saveTypeID,sizeof(unsigned int));

        unsigned char angleID,material;
        bls.read((char*)&angleID,sizeof(unsigned char));
        bls.read((char*)&material,sizeof(unsigned char));

        bool isColliding = true;

        bool hasMusic = false;
        float musicPitch = 1.0;
        unsigned int musicTrack = 0;

        bool hasLight = false;
        float lightR,lightG,lightB;

        bool hasPrint = false;
        int printMask = 0;
        std::string printName = "";

        int ownerID = -1;
        std::string brickName = "";

        if(isNextVersion)
        {
            bls.read((char*)&ownerID,sizeof(int));

            bls.read((char*)&charBuf,sizeof(unsigned char));
            if(charBuf > 0)
            {
                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                brickName = std::string(tempName);
                delete tempName;
            }

            unsigned char flags;
            bls.read((char*)&flags,sizeof(unsigned char));
            isColliding = flags & 1;
            hasMusic = flags & 2;
            hasLight = flags & 4;
            hasPrint = flags & 8;

            if(hasMusic)
            {
                bls.read((char*)&musicTrack,sizeof(unsigned int));
                bls.read((char*)&musicPitch,sizeof(float));
            }

            if(hasLight)
            {
                bls.read((char*)&lightR,sizeof(float));
                bls.read((char*)&lightG,sizeof(float));
                bls.read((char*)&lightB,sizeof(float));
            }

            if(hasPrint)
            {
                bls.read((char*)&printMask,sizeof(unsigned char));
                bls.read((char*)&charBuf,sizeof(unsigned char));

                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                printName = std::string(tempName);
                delete tempName;
            }
        }

        if(saveTypeID >= saveToServerBrickTypeIDs.size())
        {
            error("os Brick had save type index of " + std::to_string(saveTypeID)+", error in save file? Brick: " + std::to_string(brickCount));
            break;
        }

        int typeID = saveToServerBrickTypeIDs[saveTypeID];

        //We already logged an error for this while filling out saveToServerBrickTypeIDs
        if(typeID < 0)
            continue;

        if(angleID > 3)
        {
            error("os Brick had angleID of " + std::to_string((int)angleID) + " error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        if(x < -16000 || x > 16000 || y < - 16000 || y > 16000 || z < -16000 || z > 16000)
        {
            error("os Brick had position of " + std::to_string(x)+","+std::to_string(y)+","+std::to_string(z)+" this is out of bounds! Brick: " + std::to_string(brickCount));
            continue;
        }

        brick *tmp = new brick;

        tmp->posX = floor(x);
        tmp->xHalfPosition = fabs(floor(x) - x) > 0.25;

        float platesHigh = (y/1.2)*3.0;
        tmp->uPosY = floor(platesHigh);
        tmp->yHalfPosition = fabs(floor(platesHigh) - platesHigh) > 0.25;
        tmp->uPosY += 1;

        tmp->posZ = floor(z);
        tmp->zHalfPosition = fabs(floor(z) - z) > 0.25;

        tmp->posX += xOffset;
        tmp->uPosY += yOffset;
        tmp->posZ += zOffset;

        tmp->r = r;
        tmp->g = g;
        tmp->b = b;
        tmp->a = alpha;

        tmp->r /= 255.0;
        tmp->g /= 255.0;
        tmp->b /= 255.0;
        tmp->a /= 255.0;

        tmp->angleID = angleID;
        tmp->material = material;

        tmp->typeID = typeID;
        tmp->isSpecial = true;
        tmp->width = brickTypes->brickTypes[typeID]->width;
        tmp->height = brickTypes->brickTypes[typeID]->height;
        tmp->length = brickTypes->brickTypes[typeID]->length;

        if(hasPrint)
        {
            for(unsigned int a = 0; a<brickTypes->printNames.size(); a++)
                if(brickTypes->printNames[a] == printName)
                    tmp->printID = a;
            tmp->printMask = printMask;
            tmp->printName = printName;
        }

        loadedBricks.push_back(tmp);
        addBrick(tmp,false,isColliding,false);
        brickCount++;

        if(brickName.length() > 0)
            setBrickName(tmp,brickName);
        tmp->builtBy = ownerID;
        if(hasLight)
            addLight(btVector3(lightR,lightG,lightB),tmp);

        if(hasMusic)
        {
            tmp->musicPitch = musicPitch;
            tmp->music = musicTrack;
            tmp->musicLoopId = startSoundLoop(musicTrack,tmp->getX(),tmp->getY(),tmp->getZ(),musicPitch);
        }

        if(brickCount > toLoad)
        {
            error("Loaded too many bricks? OS val: " + std::to_string(opaqueSpecialBricks));
            break;
        }
    }

    bls.read((char*)&uIntBuf,sizeof(unsigned int));
    int transparentSpecialBricks = uIntBuf;

    for(int a = 0; a<transparentSpecialBricks; a++)
    {
        unsigned char r,g,b,alpha;
        bls.read((char*)&r,sizeof(unsigned char));
        bls.read((char*)&g,sizeof(unsigned char));
        bls.read((char*)&b,sizeof(unsigned char));
        bls.read((char*)&alpha,sizeof(unsigned char));

        float x,y,z;
        bls.read((char*)&x,sizeof(float));
        bls.read((char*)&y,sizeof(float));
        bls.read((char*)&z,sizeof(float));

        unsigned int saveTypeID;
        bls.read((char*)&saveTypeID,sizeof(unsigned int));

        unsigned char angleID,material;
        bls.read((char*)&angleID,sizeof(unsigned char));
        bls.read((char*)&material,sizeof(unsigned char));

        bool isColliding = true;

        bool hasMusic = false;
        float musicPitch = 1.0;
        unsigned int musicTrack = 0;

        bool hasLight = false;
        float lightR,lightG,lightB;

        bool hasPrint = false;
        int printMask = 0;
        std::string printName = "";

        int ownerID = -1;
        std::string brickName = "";

        if(isNextVersion)
        {
            bls.read((char*)&ownerID,sizeof(int));

            bls.read((char*)&charBuf,sizeof(unsigned char));
            if(charBuf > 0)
            {
                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                brickName = std::string(tempName);
                delete tempName;
            }

            unsigned char flags;
            bls.read((char*)&flags,sizeof(unsigned char));
            isColliding = flags & 1;
            hasMusic = flags & 2;
            hasLight = flags & 4;
            hasPrint = flags & 8;

            if(hasMusic)
            {
                bls.read((char*)&musicTrack,sizeof(unsigned int));
                bls.read((char*)&musicPitch,sizeof(float));
            }

            if(hasLight)
            {
                bls.read((char*)&lightR,sizeof(float));
                bls.read((char*)&lightG,sizeof(float));
                bls.read((char*)&lightB,sizeof(float));
            }

            if(hasPrint)
            {
                bls.read((char*)&printMask,sizeof(unsigned char));
                bls.read((char*)&charBuf,sizeof(unsigned char));

                char *tempName = new char[charBuf+1];
                tempName[charBuf] = 0;
                bls.read(tempName,charBuf);
                printName = std::string(tempName);
                delete tempName;
            }
        }

        if(saveTypeID >= saveToServerBrickTypeIDs.size())
        {
            error("ts Brick had save type index of " + std::to_string(saveTypeID)+", error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        int typeID = saveToServerBrickTypeIDs[saveTypeID];

        //We already logged an error for this while filling out saveToServerBrickTypeIDs
        if(typeID < 0)
            continue;

        if(angleID > 3)
        {
            error("ts Brick had angleID of " + std::to_string((int)angleID) + " error in save file? Brick: " + std::to_string(brickCount));
            continue;
        }

        if(x < -16000 || x > 16000 || y < - 16000 || y > 16000 || z < -16000 || z > 16000)
        {
            error("ts Brick had position of " + std::to_string(x)+","+std::to_string(y)+","+std::to_string(z)+" this is out of bounds! Brick: " + std::to_string(brickCount));
            continue;
        }

        brick *tmp = new brick;

        tmp->posX = floor(x);
        tmp->xHalfPosition = fabs(floor(x) - x) > 0.25;

        float platesHigh = (y/1.2)*3.0;
        tmp->uPosY = floor(platesHigh);
        tmp->yHalfPosition = fabs(floor(platesHigh) - platesHigh) > 0.25;
        tmp->uPosY += 1;

        tmp->posZ = floor(z);
        tmp->zHalfPosition = fabs(floor(z) - z) > 0.25;

        tmp->posX += xOffset;
        tmp->uPosY += yOffset;
        tmp->posZ += zOffset;

        tmp->r = r;
        tmp->g = g;
        tmp->b = b;
        tmp->a = alpha;

        tmp->r /= 255.0;
        tmp->g /= 255.0;
        tmp->b /= 255.0;
        tmp->a /= 255.0;

        tmp->angleID = angleID;
        tmp->material = material;

        tmp->typeID = typeID;
        tmp->isSpecial = true;
        tmp->width = brickTypes->brickTypes[typeID]->width;
        tmp->height = brickTypes->brickTypes[typeID]->height;
        tmp->length = brickTypes->brickTypes[typeID]->length;

        if(hasPrint)
        {
            for(unsigned int a = 0; a<brickTypes->printNames.size(); a++)
                if(brickTypes->printNames[a] == printName)
                    tmp->printID = a;
            tmp->printMask = printMask;
            tmp->printName = printName;
        }

        loadedBricks.push_back(tmp);
        addBrick(tmp,false,isColliding,false);
        brickCount++;

        if(brickName.length() > 0)
            setBrickName(tmp,brickName);
        tmp->builtBy = ownerID;
        if(hasLight)
            addLight(btVector3(lightR,lightG,lightB),tmp);

        if(hasMusic)
        {
            tmp->musicPitch = musicPitch;
            tmp->music = musicTrack;
            tmp->musicLoopId = startSoundLoop(musicTrack,tmp->getX(),tmp->getY(),tmp->getZ(),musicPitch);
        }

        if(brickCount > toLoad)
        {
            error("Loaded too many bricks? TS val: " + std::to_string(transparentSpecialBricks));
            break;
        }
    }

    bls.close();
    info("Loaded " + std::to_string(brickCount) + " bricks in " + std::to_string(SDL_GetTicks()-start) + "ms.");
}

void unifiedWorld::loadBlocklandSave(std::string filePath,std::vector<brick*> &loadedBricks)
{
    int start = SDL_GetTicks();
    std::ifstream bls(filePath.c_str());

    if(!bls.is_open())
    {
        error("Could not open save file " + filePath);
        return;
    }
    else
        debug("Opened save file!");

    std::string line = "";
    std::vector<std::string> words;

    getline(bls,line); //don't screw it up message
    getline(bls,line); //1

    int lines = atoi(line.c_str());
    for(int a = 0; a<lines; a++)
        getline(bls,line);

    for(int a = 0; a<64; a++)
        getline(bls,line);

    getline(bls,line); //linecount

    int brickCount = 0;

    brick *tmp = 0;

    while(!bls.eof())
    {
        getline(bls,line);
        if(line.substr(0,2) == "+-")
        {
            split(line,words);
            if(words[0] == "+-NTOBJECTNAME")
            {
                if(words.size() < 2)
                {
                    error("+-NTOBJECTNAME no name specified");
                    continue;
                }
                if(tmp)
                    setBrickName(tmp,words[1]);
                else
                    error("+-NTOBJECTNAME line before brick loaded!");
            }
            continue;
        }

        std::size_t quote = line.find('\"');
        if(quote == std::string::npos)
            continue;

        std::string name = lowercase(line.substr(0,quote));
        line = line.substr(quote+2);

        if(line.find("  ") != std::string::npos)
            line.insert(line.find("  ")+1,"!");

        split(line,words);

        float x = atof(words[0].c_str());
        float y = atof(words[1].c_str());
        float z = atof(words[2].c_str());

        //std::cout<<line<<"\n";
        int colorFx = atoi(words[7].c_str());
        int shapeFx = atoi(words[8].c_str());
        int brickMat = brickMaterial::none;
        if(shapeFx == 1)
            brickMat = brickMaterial::undulo;
        switch(colorFx)
        {
            case 1: brickMat += brickMaterial::peral; break;
            case 2: brickMat += brickMaterial::chrome; break;
            case 3: brickMat += brickMaterial::glow; break;
            case 4: brickMat += brickMaterial::blink; break;
            case 5: brickMat += brickMaterial::swirl; break;
            case 6: brickMat += brickMaterial::rainbow; break;
        }

        std::swap(y,z);
        x *= 2.0;
        y *= 2.0;
        z *= 2.0;
        //y -= 10;//make sure this is a multiple of 0.4
        /*x += 100;
        z += 100;*/
        vec4 color = colorSet[atoi(words[5].c_str())];

        int foundPrint = -1;
        for(unsigned int a = 0; a<brickTypes->printTypes.size(); a++)
        {
            if(brickTypes->printTypes[a]->uiName == name)
            {
                foundPrint = a;
                break;
            }
        }

        int foundNonPrint = -1;
        if(foundPrint == -1)
        {
            for(unsigned int a = 0; a<brickTypes->brickTypes.size(); a++)
            {
                if(brickTypes->brickTypes[a]->uiname == name)
                {
                    foundNonPrint = a;
                    break;
                }
                else if(brickTypes->brickTypes[a]->fileName == name)
                {
                    foundNonPrint = a;
                    break;
                }
            }
        }

        if(foundPrint == -1 && foundNonPrint == -1)
        {
            error("Could not find brick with type " + name);
            continue;
        }

        if(y < 0)
            continue;

        brickCount++;

        if(brickCount % 1000 == 0)
            std::cout<<brickCount <<" bricks loaded.\n";

        tmp = new brick;
        tmp->r = color.r;
        tmp->g = color.g;
        tmp->b = color.b;
        tmp->a = color.a;

        tmp->posX = floor(x);
        tmp->xHalfPosition = fabs(floor(x) - x) > 0.25;

        float platesHigh = (y/1.2)*3.0;
        tmp->uPosY = floor(platesHigh);
        tmp->yHalfPosition = fabs(floor(platesHigh) - platesHigh) > 0.25;
        tmp->uPosY += 1;

        tmp->posZ = floor(z);
        tmp->zHalfPosition = fabs(floor(z) - z) > 0.25;

        tmp->angleID = atoi(words[3].c_str());
        tmp->material = brickMat;
        //TODO: Add stuff for loading prints
        if(foundPrint != -1)
        {
            tmp->width = brickTypes->printTypes[foundPrint]->width;
            tmp->height = brickTypes->printTypes[foundPrint]->height;
            tmp->length = brickTypes->printTypes[foundPrint]->length;

            tmp->printMask = brickTypes->printTypes[foundPrint]->faceMask;

            //TODO: Remove this and just set ID
            tmp->printName = words[6];

            for(unsigned int a = 0; a<brickTypes->printNames.size(); a++)
                if(brickTypes->printNames[a] == words[6])
                    tmp->printID = a;

            if(tmp->printID == -1)
                error("Could not find print " + words[6]);
        }
        else
        {
            tmp->width = brickTypes->brickTypes[foundNonPrint]->width;
            tmp->height = brickTypes->brickTypes[foundNonPrint]->height;
            tmp->length = brickTypes->brickTypes[foundNonPrint]->length;
            if(brickTypes->brickTypes[foundNonPrint]->special)
            {
                tmp->isSpecial = true;
                tmp->typeID = foundNonPrint;
            }
        }

        int colliding = atoi(words[10].c_str());
        loadedBricks.push_back(tmp);
        addBrick(tmp,false,colliding,false);
    }

    bls.close();

    info("Loaded " + std::to_string(brickCount) + " bricks in " + std::to_string(SDL_GetTicks()-start) + "ms.");
}

dynamic* unifiedWorld::addDynamic(int typeID,float red,float green,float blue)
{
    if(typeID < 0 || typeID >= dynamicTypes.size())
    {
        error("Dynamic type ID out of range.");
        return 0;
    }
    dynamic *tmp = new dynamic(dynamicTypes[typeID],physicsWorld,lastDynamicID,typeID);
    tmp->redTint = red;
    tmp->greenTint = green;
    tmp->blueTint = blue;
    lastDynamicID++;
    dynamics.push_back(tmp);

    for(unsigned int a = 0; a<users.size(); a++)
        tmp->sendToClient(users[a]->netRef);

    return tmp;
}

void unifiedWorld::setBrickName(brick *theBrick,std::string name)
{
    if(name == theBrick->name)
        return;

    for(unsigned int a = 0; a<namedBricks.size(); a++)
    {
        if(namedBricks[a].name == theBrick->name)
        {
            for(unsigned int b = 0; b<namedBricks[a].bricks.size(); b++)
            {
                if(namedBricks[a].bricks[b] == theBrick)
                {
                    namedBricks[a].bricks.erase(namedBricks[a].bricks.begin() + b);
                    break;
                }
            }

            if(namedBricks[a].bricks.size() == 0)
                namedBricks.erase(namedBricks.begin() + a);

            break;
        }
    }

    theBrick->name = name;

    if(name == "")
        return;

    for(unsigned int a = 0; a<namedBricks.size(); a++)
    {
        if(namedBricks[a].name == name)
        {
            namedBricks[a].bricks.push_back(theBrick);
            return;
        }
    }

    brickNameVector tmp;
    tmp.name = name;
    tmp.bricks.push_back(theBrick);
    namedBricks.push_back(tmp);
}

item *unifiedWorld::addItem(itemType *type)
{
    item *tmp = new item(type,physicsWorld,lastDynamicID,type->itemTypeID);
    //tmp->setActivationState(DISABLE_DEACTIVATION);
    lastDynamicID++;
    dynamics.push_back(tmp);
    items.push_back(tmp);
    for(unsigned int a = 0; a<users.size(); a++)
        tmp->sendToClient(users[a]->netRef);
    return tmp;
}

item *unifiedWorld::addItem(itemType *type,float x,float y,float z)
{
    item *tmp = new item(type,physicsWorld,lastDynamicID,type->itemTypeID);
    lastDynamicID++;
    dynamics.push_back(tmp);
    items.push_back(tmp);

    btTransform t = btTransform::getIdentity();
    t.setOrigin(btVector3(x,y,z));
    tmp->setWorldTransform(t);

    for(unsigned int a = 0; a<users.size(); a++)
        tmp->sendToClient(users[a]->netRef);
    return tmp;
}



