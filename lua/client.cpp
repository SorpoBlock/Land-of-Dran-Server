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

    lua_getfield(L, -1, "id");
    int id1 = lua_tointeger(L,-1);
    lua_pop(L,2);

    lua_getfield(L, -1, "id");
    int id2 = lua_tointeger(L,-1);
    lua_pop(L,2);

    lua_pushboolean(L,id1 == id2);
    return 1;
}

static int kick(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            common_lua->theServer->kick(common_lua->users[a]->netRef);
            break;
        }
    }

    return 0;
}

static int LUAclientToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    std::string ret = "(Client " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

static int getPlayer(lua_State *L)
{
    scope("getPlayer");

    lua_getfield(L, -1, "id");
    unsigned int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            if(common_lua->users[a]->controlling)
            {
                //Register an instance of dynamic
                lua_newtable(L);
                lua_getglobal(L,"dynamic");
                lua_setmetatable(L,-2);
                lua_pushinteger(L,common_lua->users[a]->controlling->serverID);
                lua_setfield(L,-2,"id");
                return 1;
            }
            else
            {
                lua_pushnil(L);
                return 1;
            }
        }
    }

    error("Client did not exist.");
    lua_pushnil(L);
    return 1;
}

static int getCameraTarget(lua_State *L)
{
    scope("getCameraTarget");

    lua_getfield(L, -1, "id");
    unsigned int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            if(common_lua->users[a]->cameraTarget && common_lua->users[a]->cameraBoundToDynamic)
            {
                //Register an instance of dynamic
                lua_newtable(L);
                lua_getglobal(L,"dynamic");
                lua_setmetatable(L,-2);
                lua_pushinteger(L,common_lua->users[a]->cameraTarget->serverID);
                lua_setfield(L,-2,"id");
                return 1;
            }
            else
            {
                lua_pushnil(L);
                return 1;
            }
        }
    }

    error("Client did not exist.");
    lua_pushnil(L);
    return 1;
}

static int getClientName(lua_State *L)
{
    scope("getClientName");

    lua_getfield(L, -1, "id");
    unsigned int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            lua_pushstring(L,common_lua->users[a]->name.c_str());
            return 1;
        }
    }

    error("Client did not exist.");
    lua_pushnil(L);
    return 1;
}

static int setPlayer(lua_State *L)
{
    int args = lua_gettop(L);
    if(args != 2)
    {
        error("client:setPlayer called with " + std::to_string(args) + " arguments, takes 2");
        return 0;
    }

    scope("setPlayer");

    int dynamicID = -1;
    if(lua_istable(L,-1))
    {
        lua_getfield(L, -1, "id");
        dynamicID = lua_tointeger(L,-1);
        lua_pop(L,2);
    }
    else
        lua_pop(L,1);

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            for(unsigned int b = 0; b<common_lua->dynamics.size(); b++)
            {
                if(common_lua->dynamics[b]->serverID == dynamicID)
                {
                    common_lua->users[a]->controlling = common_lua->dynamics[b];

                    for(unsigned int slot = 0; slot<inventorySize; slot++)
                    {
                        packet data;
                        data.writeUInt(packetType_setInventory,packetTypeBits);
                        data.writeUInt(slot,3);

                        if(common_lua->users[a]->controlling->holding[slot])
                        {
                            data.writeBit(true);
                            data.writeUInt(common_lua->users[a]->controlling->holding[slot]->heldItemType->itemTypeID,10);
                        }
                        else
                            data.writeBit(false);

                        common_lua->users[a]->netRef->send(&data,true);
                    }
                    return 0;
                }
            }

            common_lua->users[a]->controlling = 0;
            return 0;
        }
    }

    error("Invalid client");
    return 0;
}

static int bindCamera(lua_State *L)
{
    scope("bindCamera");

    lua_getfield(L, -1, "id");
    int dynamicID = lua_tointeger(L,-1);
    lua_pop(L,2);

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            for(unsigned int b = 0; b<common_lua->dynamics.size(); b++)
            {
                if(common_lua->dynamics[b]->serverID == dynamicID)
                {
                    common_lua->users[a]->cameraBoundToDynamic = true;
                    common_lua->users[a]->cameraTarget = common_lua->dynamics[b];
                    common_lua->users[a]->needsCameraUpdate = true;
                    return 0;
                }
            }

            common_lua->users[a]->cameraBoundToDynamic = false;
            common_lua->users[a]->cameraTarget = 0;
            common_lua->users[a]->needsCameraUpdate = true;
            return 0;
        }
    }

    error("Invalid client");
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

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->cameraBoundToDynamic = false;
            common_lua->users[a]->cameraTarget = 0;
            common_lua->users[a]->camX = x;
            common_lua->users[a]->camY = y;
            common_lua->users[a]->camZ = z;
            common_lua->users[a]->needsCameraUpdate = true;
            return 0;
        }
    }

    error("Invalid client");
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

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->cameraBoundToDynamic = false;
            common_lua->users[a]->cameraTarget = 0;
            common_lua->users[a]->camDirX = x;
            common_lua->users[a]->camDirY = y;
            common_lua->users[a]->camDirZ = z;
            common_lua->users[a]->needsCameraUpdate = true;
            return 0;
        }
    }

    error("Invalid client");
    return 0;
}

static int setCameraFreelook(lua_State *L)
{
    scope("setCameraFreelook");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->cameraBoundToDynamic = false;
            common_lua->users[a]->cameraTarget = 0;
            common_lua->users[a]->cameraFreelookEnabled = toggle;
            common_lua->users[a]->needsCameraUpdate = true;
            return 0;
        }
    }

    error("Invalid client");
    return 0;
}

static int toggleRotationControl(lua_State *L)
{
    scope("toggleRotationControl");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->prohibitTurning = !toggle;
            return 0;
        }
    }

    error("Invalid client.");
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

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->bottomPrint(name,ms);
            return 0;
        }
    }

    error("Invalid client.");
    return 0;
}

static int driveCar(lua_State *L)
{
    scope("driveCar");

    int carID = lua_tointeger(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            if(carID == -1)
            {
                common_lua->users[a]->driving = 0;
                common_lua->users[a]->controlling->sittingOn = 0;
                return 0;
            }

            for(unsigned int b = 0; b<common_lua->brickCars.size(); b++)
            {
                if(common_lua->brickCars[b]->serverID == carID)
                {
                    common_lua->users[a]->driving = common_lua->brickCars[b];
                    if(common_lua->users[a]->controlling)
                    {
                        common_lua->users[a]->controlling->sittingOn = common_lua->brickCars[b]->body;
                    }
                    return 0;
                }
            }

            error("Brick car not found!");
            return 0;
        }
    }

    error("Invalid client.");
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

    //Register an instance of client
    lua_newtable(L);
    lua_getglobal(L,"clientMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,common_lua->users[idx]->playerID);
    lua_setfield(L,-2,"id");

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
                //Register an instance of client
                lua_newtable(L);
                lua_getglobal(L,"clientMETATABLE");
                lua_setmetatable(L,-2);
                lua_pushinteger(L,common_lua->users[a]->playerID);
                lua_setfield(L,-2,"id");
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

    lua_getfield(L, -1, "id");
    unsigned int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("Invalid client object passed.");
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            common_lua->spawnPlayer(common_lua->users[a],x,y,z);
            return 0;
        }
    }

    error("Client not found!");
    return 0;
}

static int LUAclearBricks(lua_State *L)
{
    scope("LUAclearBricks");

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);


    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->clearBricks(common_lua->users[a]);
            return 0;
        }
    }

    error("Invalid client!");
    return 0;
}

static int getNumClientVehicles(lua_State *L)
{
    scope("getNumClientVehicles");

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("ID passed was under 0");
        lua_pushnumber(L,0);
        return 1;
    }

    int num = 0;

    for(unsigned int a = 0; a<common_lua->brickCars.size(); a++)
    {
        if(common_lua->brickCars[a]->ownerID == id)
        {
            num++;
        }
    }

    lua_pushnumber(L,num);
    return 1;
}

static int getNumClientBricks(lua_State *L)
{
    scope("getNumClientBricks");

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("ID passed was under 0");
        lua_pushnumber(L,0);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            lua_pushnumber(L,common_lua->users[a]->ownedBricks.size());
            return 1;
        }
    }

    error("Could not find client ID " + std::to_string(id));
    lua_pushnumber(L,0);
    return 1;
}

static int hasEvalAccess(lua_State *L)
{
    scope("hasEvalAccess");

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("ID passed was under 0");
        lua_pushnumber(L,0);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            lua_pushboolean(L,common_lua->users[a]->hasLuaAccess);
            return 1;
        }
    }

    error("Could not find client ID " + std::to_string(id));
    lua_pushboolean(L,false);
    return 1;
}

static int getClientBrickIdx(lua_State *L)
{
    scope("getClientBrickIdx");

    int idx = lua_tointeger(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("ID passed was under 0");
        lua_pushnil(L);
        return 1;
    }

    if(idx < 0)
    {
        error("Idx passed was under 0");
        lua_pushnil(L);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            if(idx >= common_lua->users[a]->ownedBricks.size())
            {
                error("Brick IDX out of range for client.");
                lua_pushnil(L);
                return 1;
            }

            lua_newtable(L);
            lua_getglobal(L,"brickMETATABLE");
            lua_setmetatable(L,-2);
            lua_pushinteger(L,common_lua->users[a]->ownedBricks[idx]->serverID);
            lua_setfield(L,-2,"id");
            lua_pushlightuserdata(L,common_lua->users[a]->ownedBricks[idx]);
            lua_setfield(L,-2,"pointer");
            lua_pushstring(L,"brick");
            lua_setfield(L,-2,"type");
            return 1;
        }
    }

    error("Could not find client ID " + std::to_string(id));
    lua_pushnil(L);
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

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == id)
        {
            common_lua->users[a]->message(message,category);
            return 0;
        }
    }

    error("Could not find client ID " + std::to_string(id));
    return 0;
}

static int setJetsEnabled(lua_State *L)
{
    scope("setJetsEnabled");

    bool toggle = lua_toboolean(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->canJet = toggle;
            return 0;
        }
    }

    error("Invalid client.");
    return 0;
}

static int endAdminOrb(lua_State *L)
{
    scope("endAdminOrb");

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            common_lua->users[a]->cameraAdminOrb = false;
            common_lua->users[a]->sendCameraDetails();

            if(common_lua->users[a]->adminOrb)
            {
                common_lua->removeEmitter(common_lua->users[a]->adminOrb);
                common_lua->users[a]->adminOrb = 0;
            }
            return 0;
        }
    }

    error("Invalid client.");
    return 0;
}

static int getIP(lua_State *L)
{
    scope("getIP");

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            lua_pushstring(L,common_lua->users[a]->netRef->getIP().c_str());
            return 1;
        }
    }

    error("Invalid client.");
    lua_pushnil(L);
    return 1;
}

static int getPing(lua_State *L)
{
    scope("getPing");

    lua_getfield(L, -1, "id");
    unsigned int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == clientID)
        {
            lua_pushnumber(L,common_lua->users[a]->lastPingRecvAtMS - common_lua->users[a]->lastPingSentAtMS);
            return 1;
        }
    }

    error("Invalid client.");
    lua_pushnil(L);
    return 1;
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
        {"driveCar",driveCar},
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
