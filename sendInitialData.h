#ifndef SENDINITIALDATA_H_INCLUDED
#define SENDINITIALDATA_H_INCLUDED

#include "code/unifiedWorld.h"

//Sending brick types, dynamic types, environment data...
void sendInitalDataFirstHalf(unifiedWorld *common,clientData *source,serverClientHandle *client)
{
    for(unsigned int a = 0; a<common->users.size(); a++)
    {
        packet data;
        data.writeUInt(packetType_addOrRemovePlayer,packetTypeBits);
        data.writeBit(true); // for player's list, not typing players list
        data.writeBit(true); //add
        data.writeString(common->users[a]->name);
        data.writeUInt(common->users[a]->playerID,16);
        client->send(&data,true);
    }

    for(int a = 0; a<common->particleTypes.size(); a++)
        common->particleTypes[a]->sendToClient(client);

    common->sendDecals(client);

    common->brickTypes->sendTypesToClient(client);

    for(unsigned int a = 0; a<common->dynamicTypes.size(); a++)
        common->dynamicTypes[a]->sendTypeToClient(client,a);

    for(unsigned int a = 0; a<common->itemTypes.size(); a++)
        common->itemTypes[a]->sendToClient(client);

    packet data;
    data.writeUInt(packetType_setColorPalette,packetTypeBits);
    data.writeUInt(64,8);
    for(unsigned int a = 0; a<common->colorSet.size(); a++)
    {
        int r = common->colorSet[a].r*255;
        int g = common->colorSet[a].g*255;
        int b = common->colorSet[a].b*255;
        int al = common->colorSet[a].a*255;
        data.writeUInt(a,6);
        data.writeUInt(r,8);
        data.writeUInt(g,8);
        data.writeUInt(b,8);
        data.writeUInt(al,8);
    }
    client->send(&data,true);

    for(unsigned int a = 0; a<common->soundFilePaths.size(); a++)
    {
        packet data;
        data.writeUInt(packetType_addSoundType,packetTypeBits);
        data.writeString(common->soundFilePaths[a]);
        data.writeString(common->soundScriptNames[a]);
        data.writeUInt(a,10);
        data.writeBit(common->soundIsMusic[a]);
        client->send(&data,true);
    }

    //Water level:
    packet waterLevel;
    waterLevel.writeUInt(packetType_serverCommand,packetTypeBits);
    waterLevel.writeString("setWaterLevel");
    waterLevel.writeFloat(common->waterLevel);
    client->send(&waterLevel,true);
}


//Sending bricks, dynamics, etc...
void sendInitialDataSecondHalf(unifiedWorld *common,clientData *source,serverClientHandle *client)
{
    for(int a = 0; a<common->emitterTypes.size(); a++)
        common->emitterTypes[a]->sendToClient(client);

    packet skipCompileData;
    skipCompileData.writeUInt(packetType_serverCommand,packetTypeBits);
    skipCompileData.writeString("skipBricksCompile");
    skipCompileData.writeUInt(common->bricks.size(),24);
    client->send(&skipCompileData,true);

    std::vector<packet*> resultPackets;
    addBrickPacketsFromVector(common->bricks,resultPackets);
    for(int a = 0; a<resultPackets.size(); a++)
    {
        client->send(resultPackets[a],true);
        delete resultPackets[a];
    }

    for(unsigned int a = 0; a<common->dynamics.size(); a++)
        common->dynamics[a]->sendToClient(client);

    for(unsigned int a = 0; a<common->brickCars.size(); a++)
        common->brickCars[a]->sendBricksToClient(client);

    for(unsigned int a = 0; a<common->dynamics.size(); a++)
    {
        dynamic *toSet = common->dynamics[a];
        if(common->dynamics[a]->shapeName.length() < 1)
            continue;

        packet data;
        data.writeUInt(packetType_setShapeName,packetTypeBits);
        data.writeUInt(toSet->serverID,dynamicObjectIDBits);
        data.writeString(toSet->shapeName);
        data.writeFloat(toSet->shapeNameR);
        data.writeFloat(toSet->shapeNameG);
        data.writeFloat(toSet->shapeNameB);
        client->send(&data,true);
    }

    for(unsigned int a = 0; a<common->carMusicLoops.size(); a++)
    {
        int loopID = std::get<0>(common->carMusicLoops[a]);
        int soundID = std::get<1>(common->carMusicLoops[a]);
        int carID = std::get<2>(common->carMusicLoops[a]);
        float pitch = std::get<3>(common->carMusicLoops[a]);

        packet data;
        data.writeUInt(packetType_playSound,packetTypeBits);
        data.writeUInt(soundID,10);
        data.writeBit(true);
        data.writeUInt(loopID,24);
        data.writeFloat(pitch);
        data.writeBit(true);
        data.writeUInt(carID,10);
        client->send(&data,true);
    }

    for(unsigned int a = 0; a<common->worldMusicLoops.size(); a++)
    {
        int loopID = std::get<0>(common->worldMusicLoops[a]);
        int soundID = std::get<1>(common->worldMusicLoops[a]);
        float x = std::get<2>(common->worldMusicLoops[a]);
        float y = std::get<3>(common->worldMusicLoops[a]);
        float z = std::get<4>(common->worldMusicLoops[a]);
        float pitch = std::get<5>(common->worldMusicLoops[a]);

        packet data;
        data.writeUInt(packetType_playSound,packetTypeBits);
        data.writeUInt(soundID,10);
        data.writeBit(true);
        data.writeUInt(loopID,24);
        data.writeFloat(pitch);
        data.writeBit(false);
        data.writeFloat(x);
        data.writeFloat(y);
        data.writeFloat(z);
        client->send(&data,true);
    }

    /*for(unsigned int a = 0; a<maxMusicBricks; a++)
    {
        brick *musicBrick = common->musicBricks[a];
        if(!musicBrick)
            continue;

        if(musicBrick->car)
        {
            packet data;
            data.writeUInt(packetType_playSound,packetTypeBits);
            data.writeUInt(musicBrick->music,10);
            data.writeBit(true);
            data.writeUInt(a,5);
            data.writeFloat(musicBrick->musicPitch);
            data.writeBit(true);
            data.writeUInt(((brickCar*)musicBrick->car)->serverID,10);
            client->send(&data,true);
        }
        else //music loop on brick
        {
            packet data;
            data.writeUInt(packetType_playSound,packetTypeBits);
            data.writeUInt(musicBrick->music,10);
            data.writeBit(true);
            data.writeUInt(a,5);
            data.writeFloat(musicBrick->musicPitch);
            data.writeBit(false);
            data.writeFloat(musicBrick->getX());
            data.writeFloat(musicBrick->getY());
            data.writeFloat(musicBrick->getZ());
            client->send(&data,true);
        }
    }*/

    for(unsigned int a = 0; a<common->dynamics.size(); a++)
    {
        if(!common->dynamics[a])
            continue;

        if(common->dynamics[a]->nodeNames.size() < 1)
            continue;

        packet nodeColorPacket;
        common->dynamics[a]->nodeColorsPacket(&nodeColorPacket);
        client->send(&nodeColorPacket,true);
    }

    for(unsigned int a = 0; a<common->ropes.size(); a++)
        common->ropes[a]->sendToClient(client);

    for(unsigned int a = 0; a<common->lights.size(); a++)
        common->lights[a]->sendToClient(client);
}

#endif // SENDINITIALDATA_H_INCLUDED
