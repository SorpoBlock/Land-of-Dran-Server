#ifndef MISCFUNCTIONS_H_INCLUDED
#define MISCFUNCTIONS_H_INCLUDED

#include "code/unifiedWorld.h"

extern unifiedWorld *common_lua;

static int LUAecho(lua_State *L)
{
    const char *text = lua_tostring(L,1);
    lua_pop(L,1);

    if(!text)
    {
        error("Could not convert argument of echo to string");
        return 0;
    }

    info(text);
    return 0;
}

static int relWalkSpeed(lua_State *L)
{
    bool toggle = lua_toboolean(L,1);
    lua_pop(L,1);

    common_lua->useRelativeWalkingSpeed = toggle;

    return 0;
}

static int setColorPalette(lua_State *L)
{
    scope("setColorPalette");

    int args = lua_gettop(L);
    if(args != 5)
    {
        error("Not enough arguments, expected setColorPallete(idx,r,g,b,a)");
        return 0;
    }

    float a = lua_tonumber(L,-1);
    lua_pop(L,1);
    float b = lua_tonumber(L,-1);
    lua_pop(L,1);
    float g = lua_tonumber(L,-1);
    lua_pop(L,1);
    float r = lua_tonumber(L,-1);
    lua_pop(L,1);

    int idx = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(idx < 0 || idx >= 64)
    {
        error("Invalid palette index must be between 0 and 63");
        return 0;
    }

    common_lua->colorSet[idx].r = r;
    common_lua->colorSet[idx].g = g;
    common_lua->colorSet[idx].b = b;
    common_lua->colorSet[idx].a = a;

    packet data;
    data.writeUInt(packetType_setColorPalette,packetTypeBits);
    data.writeUInt(1,8);

    int cr = common_lua->colorSet[idx].r*255;
    int cg = common_lua->colorSet[idx].g*255;
    int cb = common_lua->colorSet[idx].b*255;
    int cal = common_lua->colorSet[idx].a*255;
    data.writeUInt(idx,6);
    data.writeUInt(cr,8);
    data.writeUInt(cg,8);
    data.writeUInt(cb,8);
    data.writeUInt(cal,8);

    common_lua->theServer->send(&data,true);
}

static int playSound(lua_State *L)
{
    scope("playSound");

    int args = lua_gettop(L);
    if(args == 1 || args == 2 || args == 3)
    {
        float vol = 1.0;
        if(args == 3)
        {
            vol = lua_tonumber(L,-1);
            if(vol < 0.0)
                vol = 0.0;
            if(vol > 1.0)
                vol = 1.0;

            lua_pop(L,1);
        }

        float pitch = 1.0;
        if(args == 2 || args == 3)
        {
            pitch = lua_tonumber(L,-1);
            if(pitch < 0)
                pitch = 0;
            if(pitch > 10.0)
                pitch = 10.0;
            lua_pop(L,1);
        }

        const char *sound = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!sound)
        {
            error("Invalid argument for sound name!");
            return 0;
        }

        std::string s = std::string(sound);
        common_lua->playSound(s,pitch,vol);

        return 0;
    }
    else if(args == 5 || args == 6 || args == 7)
    {
        bool loop = lua_toboolean(L,-1);
        lua_pop(L,1);

        float vol = 1.0;
        if(args == 7)
        {
            vol = lua_tonumber(L,-1);
            if(vol < 0.0)
                vol = 0.0;
            if(vol > 1.0)
                vol = 1.0;
            lua_pop(L,1);
        }

        float pitch = 1.0;
        if(args == 6 || args == 7)
        {
            pitch = lua_tonumber(L,-1);
            if(pitch < 0)
                pitch = 0;
            if(pitch > 10.0)
                pitch = 10.0;
            lua_pop(L,1);
        }

        float z = lua_tonumber(L,-1);
        lua_pop(L,1);
        float y = lua_tonumber(L,-1);
        lua_pop(L,1);
        float x = lua_tonumber(L,-1);
        lua_pop(L,1);

        const char *sound = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!sound)
        {
            error("Invalid argument for sound name!");
            return 0;
        }

        std::string s = std::string(sound);
        common_lua->playSound(s,x,y,z,false,-1,pitch,vol);

        return 0;
    }
    else
    {
        lua_pop(L,args);
        error(std::to_string(args) + " args passed to playSound (lua) which is bad");
        return 0;
    }
}



static int playSoundClient(lua_State *L)
{
    scope("playSoundClient");

    int args = lua_gettop(L);
    if(args == 2 || args == 3 || args == 4)
    {
        float vol = 1.0;
        if(args == 4)
        {
            vol = lua_tonumber(L,-1);
            if(vol < 0.0)
                vol = 0.0;
            if(vol > 1.0)
                vol = 1.0;

            lua_pop(L,1);
        }

        float pitch = 1.0;
        if(args == 3 || args == 4)
        {
            pitch = lua_tonumber(L,-1);
            if(pitch < 0)
                pitch = 0;
            if(pitch > 10.0)
                pitch = 10.0;
            lua_pop(L,1);
        }

        const char *sound = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!sound)
        {
            error("Invalid argument for sound name!");
            return 0;
        }

        std::string s = std::string(sound);

        clientData *target = popClient(L);

        if(!target)
        {
            error("Invalid client!");
            return 0;
        }

        common_lua->playSound(s,pitch,vol,target);

        return 0;
    }
    else
    {
        lua_pop(L,args);
        error(std::to_string(args) + " args passed to playSoundClient(lua) which is bad, needs 2 to 4 args");
        return 0;
    }
}

void saveBricks(std::ofstream &save,std::vector<brick*> &theVector)
{
    error("no more saving :(");
    return;

    unsigned int numBricks = theVector.size();
    save.write((const char*)&numBricks,sizeof(unsigned int));

    for(unsigned int a = 0; a<numBricks; a++)
    {
        brick *b = theVector[a];
        if(!b)
            continue;

        unsigned char red = b->r*255;
        save.write((const char*)&red,sizeof(unsigned char));
        unsigned char green = b->g*255;
        save.write((const char*)&green,sizeof(unsigned char));
        unsigned char blue = b->b*255;
        save.write((const char*)&blue,sizeof(unsigned char));
        unsigned char alpha = b->a*255;
        save.write((const char*)&alpha,sizeof(unsigned char));

        /*float x = b->x;
        save.write((const char*)&x,sizeof(float));
        float y = b->y;
        save.write((const char*)&y,sizeof(float));
        float z = b->z;
        save.write((const char*)&z,sizeof(float));*/

        unsigned int material = b->material;
        save.write((const char*)&material,sizeof(unsigned int));

        unsigned char angleID = b->angleID;
        save.write((const char*)&angleID,sizeof(unsigned char));

        unsigned char isSpecial = b->isSpecial ? 1 : 0;
        save.write((const char*)&isSpecial,sizeof(unsigned char));

        if(b->isSpecial)
        {
            unsigned short type = b->typeID;
            save.write((const char*)&type,sizeof(unsigned short));
        }
        else
        {
            unsigned char width = b->width;
            save.write((const char*)&width,sizeof(unsigned char));
            unsigned char height = b->height;
            save.write((const char*)&height,sizeof(unsigned char));
            unsigned char length = b->length;
            save.write((const char*)&length,sizeof(unsigned char));
        }
    }
}

/*static int saveBuildLod(lua_State *L)
{
    scope("saveBuildLod");

    const char *path = lua_tostring(L,1);
    lua_pop(L,1);

    if(!path)
    {
        error("No file path.");
        return 0;
    }

    std::ofstream save(path,std::ios::binary);
    if(!save.is_open())
    {
        error("Could not open " + std::string(path));
        return 0;
    }

    saveBricks(save,common_lua->bricks);
    info(std::to_string(common_lua->bricks.size()) + " bricks saved to " + std::string(path));

    save.close();

    return 0;
}*/

static int clearAllCars(lua_State *L)
{
    while(common_lua->brickCars.size() > 0)
        common_lua->removeBrickCar(common_lua->brickCars[0]);
}

static int bottomPrintAll(lua_State *L)
{
    scope("bottomPrintAll");

    int ms = lua_tointeger(L,-1);
    lua_pop(L,1);
    const char *name = lua_tostring(L,-1);
    lua_pop(L,1);
    if(!name)
    {
        error("Invalid string argument!");
        return 0;
    }
    if(ms < 0 || ms >= 65535)
    {
        error("Time must be between 0 and 65534ms.");
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
        common_lua->users[a]->bottomPrint(name,ms);

    return 0;
}

static int printNetIDs(lua_State *L)
{
    info("Client ID: " + std::to_string(common_lua->lastPlayerID));
    info("Brick ID: " + std::to_string(common_lua->lastBrickID));
    info("Dynamic ID: " + std::to_string(common_lua->lastDynamicID));
    info("Brick Car ID: " + std::to_string(common_lua->lastBuiltVehicleID));
    info("Snapshot ID: " + std::to_string(common_lua->lastSnapshotID));
    info("Schedule ID: " + std::to_string(common_lua->lastScheduleID));

    return 0;
}

static int messageAll(lua_State *L)
{
    scope("messageAll");

    int args = lua_gettop(L);

    if(!(args == 1 || args == 2))
    {
        error("This function must be called with 1 or 2 arguments.");
        return 0;
    }

    std::string category = "generic";

    if(args == 2)
    {
        const char *cate = lua_tostring(L,-1);
        lua_pop(L,1);

        if(cate)
            category = cate;
        else
            error("Invalid string for category argument!");
    }

    const char *msg = lua_tostring(L,-1);
    lua_pop(L,1);
    if(!msg)
    {
        error("Invalid message string!");
        return 0;
    }
    std::string message = msg;

    common_lua->messageAll(message,category);
    return 0;
}

static int clearAllBricks(lua_State *L)
{
    packet clearAllBricksPacket;
    clearAllBricksPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    clearAllBricksPacket.writeString("clearAllBricks");
    common_lua->theServer->send(&clearAllBricksPacket,true);

    for(int a = 0; a<common_lua->bricks.size(); a++)
    {
       brick *theBrick = common_lua->bricks[a];

        if(!theBrick)
            return 0;

        if(theBrick->attachedLight)
        {
            common_lua->removeLight(theBrick->attachedLight);
            theBrick->attachedLight = 0;
        }

        if(theBrick->music > 0)
            common_lua->setMusic(theBrick,0);

        if(theBrick->body)
        {
            common_lua->physicsWorld->removeRigidBody(theBrick->body);
            delete theBrick->body;
            theBrick->body = 0;
        }

        delete theBrick;
    }

    for(int a = 0; a<common_lua->users.size(); a++)
        common_lua->users[a]->ownedBricks.clear();

    common_lua->bricks.clear();

    for(int a = 0; a<common_lua->namedBricks.size(); a++)
        common_lua->namedBricks[a].bricks.clear();
    common_lua->namedBricks.clear();

    /*delete common_lua->tree;
    common_lua->tree = new Octree<brick*>(brickTreeSize*2,0);
    common_lua->tree->setEmptyValue(0);*/

    delete common_lua->overlapTree;
    common_lua->overlapTree = new brickPointerTree;

    /*int bricksToRemove = common_lua->bricks.size();
    unsigned int bricksRemoved = 0;

    if(bricksToRemove == 0)
        return 0;

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
            brick *theBrick = common_lua->bricks[i];

            data.writeUInt(theBrick->serverID,20);

            if(theBrick->music > 0)
                common_lua->setMusic(theBrick,0);

            common_lua->setBrickName(theBrick,"");

            if(theBrick->body)
            {
                common_lua->physicsWorld->removeRigidBody(theBrick->body);
                delete theBrick->body;
                theBrick->body = 0;
            }

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

            for(int x = 0; x<theBrick->width; x++)
            {
                for(int z = 0; z<theBrick->length; z++)
                {
                    if(theBrick->angleID % 2 == 0)
                    {
                        for(int y = 0; y<theBrick->height; y++)
                        {
                            common_lua->tree->set(fx+x,fy+y,fz+z,0);
                        }
                    }
                    else
                    {
                        for(int y = 0; y<theBrick->height; y++)
                        {
                            common_lua->tree->set(fx+z,fy+y,fz+x,0);
                        }
                    }
                }
            }
        }

        common_lua->theServer->send(&data,true);

        bricksRemoved += bricksInThisPacket;
        bricksToRemove -= bricksInThisPacket;
    }

    for(int a = 0; a<common_lua->bricks.size(); a++)
    {
        if(common_lua->bricks[a])
        {
            delete common_lua->bricks[a];
            common_lua->bricks[a] = 0;
        }
    }

    for(int a = 0; a<common_lua->users.size(); a++)
        common_lua->users[a]->ownedBricks.clear();

    common_lua->bricks.clear();

    for(int a = 0; a<common_lua->namedBricks.size(); a++)
        common_lua->namedBricks[a].bricks.clear();
    common_lua->namedBricks.clear();

    return 0;*/
}

static int loadLodSave(lua_State *L)
{
    scope("loadLodSave");

    const char *fileName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!fileName)
    {
        error("Invalid string argument!");
        return 0;
    }

    std::vector<brick*> loadedBricks;
    common_lua->loadLodSave(fileName,loadedBricks);

    packet skipCompileData;
    skipCompileData.writeUInt(packetType_serverCommand,packetTypeBits);
    skipCompileData.writeString("skipBricksCompile");
    skipCompileData.writeUInt(common_lua->bricks.size(),24);
    common_lua->theServer->send(&skipCompileData,true);

    std::vector<packet*> resultPackets;
    addBrickPacketsFromVector(loadedBricks,resultPackets);
    for(int a = 0; a<resultPackets.size(); a++)
    {
        common_lua->theServer->send(resultPackets[a],true);
        delete resultPackets[a];
    }

    return 0;
}

static int saveBuild(lua_State *L)
{
    scope("saveBuild");

    int args = lua_gettop(L);

    if(args != 1)
    {
        error("This function must be called with 1 argument.");
        return 0;
    }

    const char *filePath = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!filePath)
    {
        error("Invalid string argument!");
        return 0;
    }

    std::ofstream saveFile(filePath,std::ios::binary);

    if(!saveFile.is_open())
    {
        error("Could not open save file for write!");
        return 0;
    }

    float floatBuf = 0;
    unsigned char charBuf = 0;
    unsigned int uIntBuf = landOfDranBuildMagic;
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    uIntBuf = common_lua->bricks.size();
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    uIntBuf = common_lua->brickTypes->brickTypes.size();
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<common_lua->brickTypes->brickTypes.size(); a++)
    {
        std::string dbName = lowercase(getFileFromPath(common_lua->brickTypes->brickTypes[a]->fileName));
        charBuf = dbName.length();
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        saveFile.write(dbName.c_str(),dbName.length());
    }

    int basic = 0;
    int special = 0;
    for(int a = 0; a<common_lua->bricks.size(); a++)
    {
        if(common_lua->bricks[a]->isSpecial)
            special++;
        else
            basic++;
    }

    uIntBuf = basic;
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<common_lua->bricks.size(); a++)
    {
        if(common_lua->bricks[a]->isSpecial)
            continue;

        charBuf = common_lua->bricks[a]->r * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->g * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->b * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->a * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));

        floatBuf = common_lua->bricks[a]->posX + (common_lua->bricks[a]->xHalfPosition ? 0.5 : 0.0);
        saveFile.write((char*)&floatBuf,sizeof(float));
        floatBuf = common_lua->bricks[a]->uPosY + (common_lua->bricks[a]->yHalfPosition ? 0.5 : 0.0);
        floatBuf /= 2.5;
        saveFile.write((char*)&floatBuf,sizeof(float));
        floatBuf = common_lua->bricks[a]->posZ + (common_lua->bricks[a]->zHalfPosition ? 0.5 : 0.0);
        saveFile.write((char*)&floatBuf,sizeof(float));

        charBuf = common_lua->bricks[a]->width;
        //std::cout<<"Dims: "<<(int)charBuf<<" ";
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->height;
        //std::cout<<(int)charBuf<<" ";
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->length;
        //std::cout<<(int)charBuf<<"\n";
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->printMask;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));

        charBuf = common_lua->bricks[a]->angleID;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->material;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
    }

    //Only client saving uses this:
    uIntBuf = 0;
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    uIntBuf = special;
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    for(int a = 0; a<common_lua->bricks.size(); a++)
    {
        if(!common_lua->bricks[a]->isSpecial)
            continue;

        charBuf = common_lua->bricks[a]->r * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->g * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->b * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->a * 255;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));

        floatBuf = common_lua->bricks[a]->posX + (common_lua->bricks[a]->xHalfPosition ? 0.5 : 0.0);
        saveFile.write((char*)&floatBuf,sizeof(float));
        floatBuf = common_lua->bricks[a]->uPosY + (common_lua->bricks[a]->yHalfPosition ? 0.5 : 0.0);
        floatBuf /= 2.5;
        saveFile.write((char*)&floatBuf,sizeof(float));
        floatBuf = common_lua->bricks[a]->posZ + (common_lua->bricks[a]->zHalfPosition ? 0.5 : 0.0);
        saveFile.write((char*)&floatBuf,sizeof(float));

        uIntBuf = common_lua->bricks[a]->typeID;
        saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

        charBuf = common_lua->bricks[a]->angleID;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        charBuf = common_lua->bricks[a]->material;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
    }

    //Only client saving uses this:
    uIntBuf = 0;
    saveFile.write((char*)&uIntBuf,sizeof(unsigned int));

    saveFile.close();

    return 0;
}

static int loadBlocklandSave(lua_State *L)
{
    scope("loadBlocklandSave");

    const char *fileName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!fileName)
    {
        error("Invalid string argument!");
        return 0;
    }

    std::vector<brick*> loadedBricks;
    common_lua->loadBlocklandSave(fileName,loadedBricks);

    packet skipCompileData;
    skipCompileData.writeUInt(packetType_serverCommand,packetTypeBits);
    skipCompileData.writeString("skipBricksCompile");
    skipCompileData.writeUInt(common_lua->bricks.size(),24);
    common_lua->theServer->send(&skipCompileData,true);

    std::vector<packet*> resultPackets;
    addBrickPacketsFromVector(loadedBricks,resultPackets);
    for(int a = 0; a<resultPackets.size(); a++)
    {
        common_lua->theServer->send(resultPackets[a],true);
        delete resultPackets[a];
    }

    return 0;
}

static int setWaterLevel(lua_State *L)
{
    scope("setWaterLevel");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Expected 1 argument, got: " + std::to_string(args));
        return 0;
    }

    float level = lua_tonumber(L,-1);
    lua_pop(L,1);

    common_lua->waterLevel = level;

    packet waterLevel;
    waterLevel.writeUInt(packetType_serverCommand,packetTypeBits);
    waterLevel.writeString("setWaterLevel");
    waterLevel.writeFloat(level);
    common_lua->theServer->send(&waterLevel,true);
}

void bindMiscFuncs(lua_State *L)
{
    lua_register(L,"echo",LUAecho);
    lua_register(L,"relWalkSpeed",relWalkSpeed);
    //lua_register(L,"saveBuildLod",saveBuildLod);
    lua_register(L,"playSound",playSound);
    lua_register(L,"playSoundClient",playSoundClient);
    lua_register(L,"clearAllCars",clearAllCars);
    lua_register(L,"bottomPrintAll",bottomPrintAll);
    lua_register(L,"printNetIDs",printNetIDs);
    lua_register(L,"messageAll",messageAll);
    lua_register(L,"clearAllBricks",clearAllBricks);
    lua_register(L,"loadBlocklandSave",loadBlocklandSave);
    lua_register(L,"loadLodSave",loadLodSave);
    lua_register(L,"saveBuild",saveBuild);
    lua_register(L,"setColorPalette",setColorPalette);
    lua_register(L,"setWaterLevel",setWaterLevel);
}

#endif // MISCFUNCTIONS_H_INCLUDED




















