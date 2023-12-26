#include "client.h"

static int LUAclientEqual(lua_State *L)
{
    int args = lua_gettop(L);
    if(args != 2)
    {
        error("LUAclientEqual args were not 2");
        lua_pushboolean(L,false);
        return 1;
    }

    clientData *a = popClient(L);
    clientData *b = popClient(L);

    lua_pushboolean(L,a == b);
    return 1;
}

static int kick(lua_State *L)
{
    clientData *toKick = popClient(L);
    if(toKick)
        common_lua->theServer->kick(toKick->netRef);
    return 0;
}

static int LUAclientToString(lua_State *L)
{
    clientData *arg = popClient(L);

    std::string ret = "(Nil?)";
    if(arg)
        ret = "(Client " + std::to_string(arg->playerID) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

static int getPlayer(lua_State *L)
{
    scope("getPlayer");

    clientData *client = popClient(L);
    if(!client)
        lua_pushnil(L);
    else if(!client->controlling)
        lua_pushnil(L);
    else
        pushDynamic(L,client->controlling);

    return 1;
}

static int getCameraTarget(lua_State *L)
{
    scope("getCameraTarget");

    clientData *client = popClient(L);
    if(!client)
        lua_pushnil(L);
    else if(!client->cameraTarget || !client->cameraBoundToDynamic)
        lua_pushnil(L);
    else
        pushDynamic(L,client->cameraTarget);
    return 1;
}

static int getClientName(lua_State *L)
{
    scope("getClientName");

    clientData *client = popClient(L);
    if(!client)
        lua_pushnil(L);
    else
        lua_pushstring(L,client->name.c_str());
    return 1;
}

static int setPlayer(lua_State *L)
{
    scope("setPlayer");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("client:setPlayer called with " + std::to_string(args) + " arguments, takes 2");
        return 0;
    }

    dynamic *toSet = popDynamic(L,true);
    clientData *client = popClient(L);

    if(!client)
        return 0;

    if(client->controlling == toSet)
        return 0;

    client->setControlling(toSet);

    if(!toSet)
        return 0;

    for(unsigned int slot = 0; slot<inventorySize; slot++)
    {
        packet data;
        data.writeUInt(packetType_setInventory,packetTypeBits);
        data.writeUInt(slot,3);

        if(toSet->holding[slot])
        {
            data.writeBit(true);
            data.writeUInt(toSet->holding[slot]->heldItemType->itemTypeID,10);
        }
        else
            data.writeBit(false);

        client->netRef->send(&data,true);
    }

    return 0;
}

static int bindCamera(lua_State *L)
{
    scope("bindCamera");

    dynamic *target = popDynamic(L,true);
    clientData *client = popClient(L);

    if(!client)
        return 0;

    if(client->cameraTarget == target && client->cameraBoundToDynamic && target)
        return 0;

    if(!target)
    {
        client->cameraBoundToDynamic = false;
        client->cameraTarget = 0;
        client->needsCameraUpdate = true;
        return 0;
    }

    client->cameraBoundToDynamic = true;
    client->cameraTarget = target;
    client->needsCameraUpdate = true;
    return 0;
}

static int setCameraPosition(lua_State *L)
{
    scope("setCameraPosition");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->cameraBoundToDynamic = false;
    client->cameraTarget = 0;
    client->camX = x;
    client->camY = y;
    client->camZ = z;
    client->needsCameraUpdate = true;

    return 0;
}

static int setCameraDirection(lua_State *L)
{
    scope("setCameraDirection");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->cameraBoundToDynamic = false;
    client->cameraTarget = 0;
    client->camDirX = x;
    client->camDirY = y;
    client->camDirZ = z;
    client->needsCameraUpdate = true;
    return 0;
}

static int setCameraFreelook(lua_State *L)
{
    scope("setCameraFreelook");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->cameraBoundToDynamic = false;
    client->cameraTarget = 0;
    client->cameraFreelookEnabled = toggle;
    client->needsCameraUpdate = true;

    return 0;
}

static int toggleRotationControl(lua_State *L)
{
    scope("toggleRotationControl");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->prohibitTurning = !toggle;
    return 0;
}

static int bottomPrint(lua_State *L)
{
    scope("bottomPrint");

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

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->bottomPrint(name,ms);
    return 0;
}

static int getClientIdx(lua_State *L)
{
    scope("getClientIdx");

    int idx = lua_tointeger(L,1);
    lua_pop(L,1);

    if(idx < 0 || (int)idx >= (int)common_lua->users.size())
    {
        error("invalid index " + std::to_string(idx));
        lua_pushnil(L);
        return 1;
    }

    int args = lua_gettop(L);
    if(args > 0)
        lua_pop(L,args);

    pushClient(L,common_lua->users[idx]);

    return 1;
}

static int getClientByName(lua_State *L)
{
    scope("getClientByName");

    const char* name = lua_tostring(L,1);
    lua_pop(L,1);

    if(!name)
    {
        error("invalid argument");
        lua_pushnil(L);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        clientData *c = common_lua->users[a];
        if(c)
        {
            bool match = true;
            for(unsigned int i = 0; name[i]; i++)
            {
                if(i >= c->name.length())
                {
                    match = false;
                    break;
                }

                if(tolower(name[i]) != tolower(c->name[i]))
                {
                    match = false;
                    break;
                }
            }

            if(match)
            {
                pushClient(L,common_lua->users[a]);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

static int getNumClients(lua_State *L)
{
    lua_pushnumber(L,common_lua->users.size());
    return 1;
}

static int LUAspawnPlayer(lua_State *L)
{
    scope("LUAspawnPlayer");

    int args = lua_gettop(L);
    if(args != 4)
    {
        error("spawnPlayer expects 4 arguments: " + std::to_string(args) + " provided");
        lua_pop(L,args);
        return 0;
    }

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    common_lua->spawnPlayer(client,x,y,z);
    return 0;
}

static int LUAclearBricks(lua_State *L)
{
    scope("LUAclearBricks");

    clientData *client = popClient(L);

    if(!client)
        return 0;

    common_lua->clearBricks(client);

    return 0;
}

static int getNumClientVehicles(lua_State *L)
{
    scope("getNumClientVehicles");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushnil(L);
        return 1;
    }

    int num = 0;

    for(unsigned int a = 0; a<common_lua->brickCars.size(); a++)
        if(common_lua->brickCars[a]->ownerID == client->accountID)
            num++;

    lua_pushnumber(L,num);
    return 1;
}

static int getNumClientBricks(lua_State *L)
{
    scope("getNumClientBricks");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L,client->ownedBricks.size());
    return 1;
}

static int hasEvalAccess(lua_State *L)
{
    scope("hasEvalAccess");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushboolean(L,false);
        return 1;
    }

    lua_pushboolean(L,client->hasLuaAccess);
    return 1;
}

static int isGuest(lua_State *L)
{
    scope("isGuest");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushboolean(L,true);
        return 1;
    }

    lua_pushboolean(L,client->logInState != 2);
    return 1;
}

static int getID(lua_State *L)
{
    scope("getID");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushnumber(L,-1);
        return 1;
    }

    if(client->logInState != 2)
        lua_pushnumber(L,0);
    else
        lua_pushnumber(L,client->accountID);
    return 1;
}

static int getClientBrickIdx(lua_State *L)
{
    scope("getClientBrickIdx");

    int idx = lua_tointeger(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushnil(L);
        return 1;
    }

    if(idx >= client->ownedBricks.size())
    {
        error("Brick IDX out of range for client.");
        lua_pushnil(L);
        return 1;
    }

    if(idx < 0)
    {
        error("Brick IDX must be positive integer!");
        lua_pushnil(L);
        return 1;
    }

    pushBrick(L,client->ownedBricks[idx]);
    return 1;
}

static int sendMessage(lua_State *L)
{
    scope("sendMessage");

    int args = lua_gettop(L);

    if(!(args == 2 || args == 3))
    {
        error("This function must be called with 2 or 3 arguments.");
        return 0;
    }

    std::string category = "generic";

    if(args == 3)
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

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->message(message,category);
    return 0;
}

static int setJetsEnabled(lua_State *L)
{
    scope("setJetsEnabled");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->canJet = toggle;

    if(client->controlling)
        client->forceTransformUpdate();

    return 0;
}

static int allowBuilding(lua_State *L)
{
    scope("allowBuilding");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->allowedToBuild = toggle;

    return 0;
}

static int endAdminOrb(lua_State *L)
{
    scope("endAdminOrb");

    clientData *client = popClient(L);

    if(!client)
        return 0;

    client->cameraAdminOrb = false;
    client->sendCameraDetails();

    if(client->adminOrb)
    {
        common_lua->removeEmitter(client->adminOrb);
        client->adminOrb = 0;
    }
    return 0;
}

static int getIP(lua_State *L)
{
    scope("getIP");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L,client->netRef->getIP().c_str());
    return 1;
}

static int getPing(lua_State *L)
{
    scope("getPing");

    clientData *client = popClient(L);

    if(!client)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L,client->lastPingRecvAtMS - client->lastPingSentAtMS);
    return 1;
}

static int setVignette(lua_State *L)
{
    scope("setVignette");

    int args = lua_gettop(L);

    if(!(args == 2 || args == 5))
    {
        error("Function requires 2 or 5 arguments!");
        return 0;
    }

    if(args == 5)
    {
        float b = lua_tonumber(L,-1);
        lua_pop(L,1);
        float g = lua_tonumber(L,-1);
        lua_pop(L,1);
        float r = lua_tonumber(L,-1);
        lua_pop(L,1);
        float a = lua_tonumber(L,-1);
        lua_pop(L,1);

        if(b < -10 || b > 10)
            b = 0;
        if(g < -10 || g > 10)
            g = 0;
        if(r < -10 || r > 10)
            r = 0;
        if(a < -10 || a > 10)
            a = 0;

        clientData *c = popClient(L);

        if(!c)
        {
            error("Invalid client!");
            return 0;
        }

        packet vignetteData;
        vignetteData.writeUInt(packetType_serverCommand,packetTypeBits);
        vignetteData.writeString("vignette");
        vignetteData.writeFloat(a);
        vignetteData.writeBit(true);
        vignetteData.writeFloat(r);
        vignetteData.writeFloat(g);
        vignetteData.writeFloat(b);
        c->netRef->send(&vignetteData,true);

        return 0;
    }

    float a = lua_tonumber(L,-1);
    lua_pop(L,1);

    if(a < -10 || a > 10)
        a = 0;

    clientData *c = popClient(L);

    if(!c)
    {
        error("Invalid client!");
        return 0;
    }

    packet vignetteData;
    vignetteData.writeUInt(packetType_serverCommand,packetTypeBits);
    vignetteData.writeString("vignette");
    vignetteData.writeFloat(a);
    vignetteData.writeBit(false);
    c->netRef->send(&vignetteData,true);

    return 0;
}

void registerClientFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAclientToString},
        {"__eq",LUAclientEqual},
        {"getPlayer",getPlayer},
        {"setPlayer",setPlayer},
        {"toggleRotationControl",toggleRotationControl},
        {"bindCamera",bindCamera},
        {"setCameraPosition",setCameraPosition},
        {"setCameraDirection",setCameraDirection},
        {"setCameraFreelook",setCameraFreelook},
        //{"driveCar",driveCar},
        {"bottomPrint",bottomPrint},
        {"spawnPlayer",LUAspawnPlayer},
        {"clearBricks",LUAclearBricks},
        {"getName",getClientName},
        {"getNumVehicles",getNumClientVehicles},
        {"getNumBricks",getNumClientBricks},
        {"getBrickIdx",getClientBrickIdx},
        {"sendMessage",sendMessage},
        {"setJetsEnabled",setJetsEnabled},
        {"hasEvalAccess",hasEvalAccess},
        {"endAdminOrb",endAdminOrb},
        {"getCameraTarget",getCameraTarget},
        {"getIP",getIP},
        {"getPing",getPing},
        {"kick",kick},
        {"getID",getID},
        {"isGuest",isGuest},
        {"setVignette",setVignette},
        {"allowBuilding",allowBuilding},
        {NULL,NULL}};
    luaL_newmetatable(L,"clientMETATABLE");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"clientMETATABLE");

    //Register functions
    lua_register(L,"getClientIdx",getClientIdx);
    lua_register(L,"getClientByName",getClientByName);
    lua_register(L,"gcbn",getClientByName);
    lua_register(L,"getNumClients",getNumClients);
}
