#ifndef MISCFUNCTIONS_H_INCLUDED
#define MISCFUNCTIONS_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/crc.h"

extern unifiedWorld *common_lua;

bool okayFilePath(std::string path)
{
    //Greater than 2 characters
    //Not start with a . or /
    //Can only contain a-z A-Z 0-9 _ . /
    //Can not have a . or / as the first character
    //Can only have one .
    //Can not be longer than 48 characters

    if(path.length() < 2)
        return false;

    if(path.length() > 48)
        return false;

    if(path[0] == '.')
        return false;

    if(path[0] == '/')
        return false;

    if(path[0] == '\\')
        return false;

    bool onePeriod = false;

    for(int i = 0; path[i]; i++)
    {
        if(path[i] >= 'a' && path[i] <= 'z')
            continue;
        if(path[i] >= 'A' && path[i] <= 'Z')
            continue;
        if(path[i] >= '0' && path[i] <= '9')
            continue;
        if(path[i] == '/' || path[i] == '_')
            continue;
        if(path[i] == '.')
        {
            if(onePeriod)
                return false;
            else
            {
                onePeriod = true;
                continue;
            }
        }

        return false;
    }

    return true;
}

unsigned int getFileChecksum(const char *filePath)
{
    std::ifstream file(filePath,std::ios::in | std::ios::binary);
    if(!file.is_open())
        return 0;

    const unsigned int bufSize = 2048;
    char buffer[bufSize];
    unsigned int ret = 0;
    bool firstRun = true;
    while(!file.eof())
    {
        file.read(buffer,bufSize);
        if(firstRun)
        {
            ret = CRC::Calculate(buffer,file.gcount(),CRC::CRC_32());
            firstRun = false;
        }
        else
            ret = CRC::Calculate(buffer,file.gcount(),CRC::CRC_32(),ret);
    }

    file.close();
    return ret;
}

static int addCustomFile(lua_State *L)
{
    scope("addCustomFile");

    const char *path = lua_tostring(L,-1);
    lua_pop(L,1);
    const char *name = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!path)
    {
        error("Invalid path string!");
        return 0;
    }

    if(!name)
    {
        error("Invalid name string!");
        return 0;
    }

    std::string pathStr = path;
    std::string nameStr = name;

    if(!okayFilePath(pathStr))
    {
        error("File path does not meet file path requirements!");
        return 0;
    }

    if(nameStr.length() < 1 || nameStr.length() > 48)
    {
        error("Name must be between 1 and 48 characters!");
        return 0;
    }

    if(!std::filesystem::exists("add-ons/"+pathStr))
    {
        error("File add-ons/"+pathStr+" does not exist!");
        return 0;
    }

    unsigned int size = file_size("add-ons/"+pathStr);
    if(size < 1 || size > 2000000)
    {
        error("File must be between 1 and 2 million bytes.");
        return 0;
    }

    std::filesystem::path p(pathStr);
    std::string ext = std::string(p.extension().u8string());
    fileType type = discernExtension(ext);

    if(type == unknownFile)
    {
        error("File type extension unknown: " + ext);
        error("At the moment only .blb and .wav files are supported as I test this system.");
        return 0;
    }

    std::string debugStr = "Added file ";
    debugStr += path;
    debugStr += " with name ";
    debugStr += name;
    debugStr += " size: ";
    debugStr += std::to_string(size);
    debugStr += " bytes.";
    debug(debugStr);

    fileDescriptor tmp;
    tmp.id = common_lua->nextFileID;
    common_lua->nextFileID++;
    tmp.name = nameStr;
    tmp.path = pathStr;
    tmp.type = type;
    tmp.sizeBytes = size;
    tmp.checksum = getFileChecksum(std::string("add-ons/"+pathStr).c_str());
    common_lua->customFiles.push_back(tmp);

    lua_pushinteger(L,tmp.id);
    return 1;
}

static int setDNCTime(lua_State *L)
{
    float num = lua_tonumber(L,-1);
    lua_pop(L,1);

    packet dncTimePacket;
    dncTimePacket.writeUInt(packetType_serverCommand,packetTypeBits);
    dncTimePacket.writeString("dncTime");
    dncTimePacket.writeFloat(num);

    common_lua->theServer->send(&dncTimePacket,true);

    return 0;
}

static int setDNCSpeed(lua_State *L)
{
    float num = lua_tonumber(L,-1);
    lua_pop(L,1);

    packet dncTimePacket;
    dncTimePacket.writeUInt(packetType_serverCommand,packetTypeBits);
    dncTimePacket.writeString("dncSpeed");
    dncTimePacket.writeFloat(num);

    common_lua->theServer->send(&dncTimePacket,true);

    return 0;
}

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

    return 0;
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
    else if(args == 4 || args == 5 || args == 6)
    {
        float vol = 1.0;
        if(args == 6)
        {
            vol = lua_tonumber(L,-1);
            if(vol < 0.0)
                vol = 0.0;
            if(vol > 1.0)
                vol = 1.0;
            lua_pop(L,1);
        }

        float pitch = 1.0;
        if(args == 5 || args == 6)
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
        common_lua->playSound(s,x,y,z,pitch,vol);

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

static int clearAllCars(lua_State *L)
{
    while(common_lua->brickCars.size() > 0)
        common_lua->removeBrickCar(common_lua->brickCars[0]);

    return 0;
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
    info("Emitter ID: " + std::to_string(common_lua->lastEmitterID));
    info("Light ID: " + std::to_string(common_lua->lastLightID));
    info("Packet ID: " + std::to_string(common_lua->theServer->highestPacketID));

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

        if(theBrick->musicLoopId != -1)
        {
            common_lua->stopSoundLoop(theBrick->musicLoopId);
            theBrick->musicLoopId = -1;
        }
        //common_lua->setMusic(theBrick,0);

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

    delete common_lua->overlapTree;
    common_lua->overlapTree = new brickPointerTree;

    return 0;
}

static int loadLodSave(lua_State *L)
{
    scope("loadLodSave");

    int args = lua_gettop(L);

    if(!(args == 1 || args == 4))
    {
        error("This function takes 1 or 4 arguments!");
        return 0;
    }

    int z=0,y=0,x=0;

    if(args == 4)
    {
        z = lua_tointeger(L,-1);
        lua_pop(L,1);
        y = lua_tointeger(L,-1);
        lua_pop(L,1);
        x = lua_tointeger(L,-1);
        lua_pop(L,1);
    }

    const char *fileName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!fileName)
    {
        error("Invalid string argument!");
        return 0;
    }

    std::vector<brick*> loadedBricks;
    common_lua->loadLodSave(fileName,loadedBricks,x,y,z);

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

    bool omitOwnership = false;

    int args = lua_gettop(L);

    if(args < 1 || args > 2)
    {
        error("This function must be called with 1 or 2 arguments.");
        return 0;
    }

    if(args == 2)
    {
        omitOwnership = lua_toboolean(L,-1);
        lua_pop(L,1);
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
    unsigned int uIntBuf = landOfDranBuildMagic+1;//next version
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

        if(omitOwnership)
        {
            int intBuf = -1;
            saveFile.write((char*)&intBuf,sizeof(int));
        }
        else
        {
            int intBuf = common_lua->bricks[a]->builtBy;
            saveFile.write((char*)&intBuf,sizeof(int));
        }

        charBuf = common_lua->bricks[a]->name.length();
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        if(charBuf > 0)
        {
            std::string brickName = common_lua->bricks[a]->name;
            if(brickName.length() > 255)
                brickName = brickName.substr(0,255);
            saveFile.write(brickName.c_str(),brickName.length());
        }

        //Flags
        charBuf = 0;
        if(common_lua->bricks[a]->isColliding())
            charBuf += 1;
        if(common_lua->bricks[a]->musicLoopId != -1)
            charBuf += 2;
        if(common_lua->bricks[a]->attachedLight)
            charBuf += 4;
        if(common_lua->bricks[a]->printID != -1)
            charBuf += 8;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));

        if(common_lua->bricks[a]->musicLoopId != -1)
        {
            uIntBuf = common_lua->bricks[a]->music;
            saveFile.write((char*)&uIntBuf,sizeof(unsigned int));
            floatBuf = common_lua->bricks[a]->musicPitch;
            saveFile.write((char*)&floatBuf,sizeof(float));
        }

        if(common_lua->bricks[a]->attachedLight)
        {
            floatBuf = common_lua->bricks[a]->attachedLight->color.x();
            saveFile.write((char*)&floatBuf,sizeof(float));
            floatBuf = common_lua->bricks[a]->attachedLight->color.y();
            saveFile.write((char*)&floatBuf,sizeof(float));
            floatBuf = common_lua->bricks[a]->attachedLight->color.z();
            saveFile.write((char*)&floatBuf,sizeof(float));
        }

        if(common_lua->bricks[a]->printID != -1)
        {
            charBuf = common_lua->bricks[a]->printMask;
            saveFile.write((char*)&charBuf,sizeof(unsigned char));
            charBuf = common_lua->bricks[a]->printName.length();
            saveFile.write((char*)&charBuf,sizeof(unsigned char));
            saveFile.write(common_lua->bricks[a]->printName.c_str(),common_lua->bricks[a]->printName.length());
        }
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

        if(omitOwnership)
        {
            int intBuf = -1;
            saveFile.write((char*)&intBuf,sizeof(int));
        }
        else
        {
            int intBuf = common_lua->bricks[a]->builtBy;
            saveFile.write((char*)&intBuf,sizeof(int));
        }

        charBuf = common_lua->bricks[a]->name.length();
        saveFile.write((char*)&charBuf,sizeof(unsigned char));
        if(charBuf > 0)
        {
            std::string brickName = common_lua->bricks[a]->name;
            if(brickName.length() > 255)
                brickName = brickName.substr(0,255);
            saveFile.write(brickName.c_str(),brickName.length());
        }

        //Flags
        charBuf = 0;
        if(common_lua->bricks[a]->isColliding())
            charBuf += 1;
        if(common_lua->bricks[a]->musicLoopId != -1)
            charBuf += 2;
        if(common_lua->bricks[a]->attachedLight)
            charBuf += 4;
        if(common_lua->bricks[a]->printID != -1)
            charBuf += 8;
        saveFile.write((char*)&charBuf,sizeof(unsigned char));

        if(common_lua->bricks[a]->musicLoopId != -1)
        {
            uIntBuf = common_lua->bricks[a]->music;
            saveFile.write((char*)&uIntBuf,sizeof(unsigned int));
            floatBuf = common_lua->bricks[a]->musicPitch;
            saveFile.write((char*)&floatBuf,sizeof(float));
        }

        if(common_lua->bricks[a]->attachedLight)
        {
            floatBuf = common_lua->bricks[a]->attachedLight->color.x();
            saveFile.write((char*)&floatBuf,sizeof(float));
            floatBuf = common_lua->bricks[a]->attachedLight->color.y();
            saveFile.write((char*)&floatBuf,sizeof(float));
            floatBuf = common_lua->bricks[a]->attachedLight->color.z();
            saveFile.write((char*)&floatBuf,sizeof(float));
        }

        if(common_lua->bricks[a]->printID != -1)
        {
            charBuf = common_lua->bricks[a]->printMask;
            saveFile.write((char*)&charBuf,sizeof(unsigned char));
            charBuf = common_lua->bricks[a]->printName.length();
            saveFile.write((char*)&charBuf,sizeof(unsigned char));
            saveFile.write(common_lua->bricks[a]->printName.c_str(),common_lua->bricks[a]->printName.length());
        }
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

    return 0;
}

static int setRespawnTime(lua_State *L)
{
    scope("setRespawnTime");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Expected 1 argument, got: " + std::to_string(args));
        return 0;
    }

    float level = lua_tonumber(L,-1);
    lua_pop(L,1);

    if(level < 0)
        level = 0;
    if(level > 60000)
        level = 60000;

    common_lua->respawnTimeMS = level;

    return 0;
}

static int setAudioEffect(lua_State *L)
{
    scope("setAudioEffect");

    int args = lua_gettop(L);
    if(args == 1)
    {
        const char *effectName = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!effectName)
        {
            error("Invalid string argument!");
            return 0;
        }

        packet commonAudioEffect;
        commonAudioEffect.writeUInt(packetType_serverCommand,packetTypeBits);
        commonAudioEffect.writeString("audioEffect");
        commonAudioEffect.writeString(effectName);
        common_lua->theServer->send(&commonAudioEffect,true);
    }
    else if(args == 2)
    {
        clientData *user = popClient(L);
        if(!user)
        {
            error("Invalid client passed as parameter!");
            return 0;
        }

        const char *effectName = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!effectName)
        {
            error("Invalid string argument!");
            return 0;
        }

        packet audioEffect;
        audioEffect.writeUInt(packetType_serverCommand,packetTypeBits);
        audioEffect.writeString("audioEffect");
        audioEffect.writeString(effectName);
        user->netRef->send(&audioEffect,true);
    }
    else
        error("Function needs 1 or 2 args!");

    return 0;
}

static int radiusImpulse(lua_State *L)
{
    scope("radiusImpulse");

    int args = lua_gettop(L);
    if(args < 5 || args > 6)
    {
        error("This function takes 5 or 6 arguments");
        return 0;
    }

    bool doDamage = false;
    if(args == 6)
    {
        doDamage = lua_toboolean(L,-1);
        lua_pop(L,1);
    }

    float power = lua_tonumber(L,-1);
    lua_pop(L,1);
    float radius = lua_tonumber(L,-1);
    lua_pop(L,1);
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    if(power < -1000 || power > 1000)
    {
        error("Power must be between -1000 and 1000");
        return 0;
    }

    if(radius <= 0 || radius >= 100)
    {
        error("Radius must be between 0 and 100");
        return 0;
    }

    common_lua->radiusImpulse(btVector3(x,y,z),radius,power,doDamage);
    return 0;
}

static int addSoundType(lua_State *L)
{
    scope("addSoundType");

    int numArgs = lua_gettop(L);
    if(numArgs != 2)
    {
        lua_pop(L,numArgs);
        error("Wrong # args. addSoundFile(fileID,scriptName)");
        return 0;
    }

    const char *scriptName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!scriptName)
    {
        error("Invalid name string for sound file.");
        return 0;
    }

    int fileID = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(fileID < 0 || fileID >= common_lua->customFiles.size())
    {
        error("Invalid file index: " + std::to_string(fileID));
        return 0;
    }

    common_lua->addSoundType(std::string(scriptName),"add-ons/" + common_lua->customFiles[fileID].path);

    return 0;
}

static int addMusicType(lua_State *L)
{
    scope("addMusicType");

    int numArgs = lua_gettop(L);
    if(numArgs != 2)
    {
        lua_pop(L,numArgs);
        error("Wrong # args. addMusicFile(fileID,scriptName)");
        return 0;
    }

    const char *scriptName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!scriptName)
    {
        error("Invalid name string for sound file.");
        return 0;
    }

    int fileID = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(fileID < 0 || fileID >= common_lua->customFiles.size())
    {
        error("Invalid file index: " + std::to_string(fileID));
        return 0;
    }

    common_lua->addMusicType(std::string(scriptName),"add-ons/" + common_lua->customFiles[fileID].path);

    return 0;
}

void bindMiscFuncs(lua_State *L)
{
    lua_register(L,"echo",LUAecho);
    lua_register(L,"relWalkSpeed",relWalkSpeed);
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
    lua_register(L,"setAudioEffect",setAudioEffect);
    lua_register(L,"radiusImpulse",radiusImpulse);
    lua_register(L,"setRespawnTime",setRespawnTime);
    lua_register(L,"setDNCTime",setDNCTime);
    lua_register(L,"setDNCSpeed",setDNCSpeed);
    lua_register(L,"addCustomFile",addCustomFile);
    lua_register(L,"addSoundType",addSoundType);
    lua_register(L,"addMusicType",addMusicType);
}

#endif // MISCFUNCTIONS_H_INCLUDED




















