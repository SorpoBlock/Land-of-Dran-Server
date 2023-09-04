#include "code/lua/itemLua.h"

static int LUAitemEqual(lua_State *L)
{
    int args = lua_gettop(L);
    if(args != 2)
    {
        error("LUAitemEqual args were not 2");
        lua_pushboolean(L,false);
        return 1;
    }

    item *a = popItem(L);
    item *b = popItem(L);

    lua_pushboolean(L,a == b);
    return 1;
}

static int LUAitemToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);

    lua_pop(L,2);

    std::string ret = "(Item " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

static int LUAremoveItem(lua_State *L)
{
    scope("LUAremoveItem");

    item *object = popItem(L);

    if(!object)
        return 0;

    common_lua->removeItem(object);
    return 0;
}

static int getNumItems(lua_State *L)
{
    lua_pushnumber(L,common_lua->items.size());
    return 1;
}

static int getItemIdx(lua_State *L)
{
    int idx = lua_tointeger(L,1);
    lua_pop(L,1);

    if(idx < 0 || (int)idx >= (int)common_lua->items.size())
    {
        error("getItemIdx invalid index " + std::to_string(idx));
        lua_pushnil(L);
        return 1;
    }

    pushItem(L,common_lua->items[idx]);

    return 1;
}

static int addItemLua(lua_State *L)
{
    scope("addItemLua");

    int args = lua_gettop(L);
    if(args != 4)
    {
        error("Too many arguments, expected 4");
        lua_pushnil(L);
        return 1;
    }

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    const char *name = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!name)
    {
        error("Invalid string for name of item!");
        lua_pushnil(L);
        return 1;
    }

    std::string uiName = std::string(name);
    for(int a = 0; a<common_lua->itemTypes.size(); a++)
    {
        if(common_lua->itemTypes[a]->uiName == uiName)
        {
            item *ret = common_lua->addItem(common_lua->itemTypes[a],x,y,z);
            if(!ret)
            {
                error("Error allocating item!");
                lua_pushnil(L);
            }
            else
                pushItem(L,ret);
            return 1;
        }
    }

    error("Item not found with ui name " + uiName);
    lua_pushnil(L);
    return 1;
}

static int getHolder(lua_State *L)
{
    scope("getHolder");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Expected one argument!");
        lua_pushnil(L);
        return 1;
    }

    item *i = popItem(L);
    if(!i)
    {
        error("No item!");
        lua_pushnil(L);
        return 1;
    }

    if(i->heldBy)
        pushDynamic(L,i->heldBy);
    else
        lua_pushnil(L);
    return 1;
}

static int setFireAnim(lua_State *L)
{
    scope("setFireAnim");

    int args = lua_gettop(L);
    if(args < 2 || args > 3)
    {
        error("Function expects 2 or 3 arguments!");
        return 0;
    }

    float animSpeed = 1.0;
    if(args == 3)
    {
        animSpeed = lua_tonumber(L,-1);
        lua_pop(L,1);
        if(animSpeed <= 0 || animSpeed > 10)
        {
            error("animation speed should be >0 and <= 10");
            animSpeed = 1.0;
        }
    }

    const char *animName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!animName)
    {
        error("Invalid string param");
        return 0;
    }

    item *i = popItem(L);

    if(!i)
    {
        error("Invalid item!");
        return 0;
    }

    std::string nameStr = std::string(animName);

    if(nameStr.length() > 0)
    {
        for(int a = 0; a<i->type->animations.size(); a++)
        {
            if(i->type->animations[a].name == nameStr)
            {
                i->nextFireAnim = a;
                i->nextFireAnimSpeed = animSpeed;

                packet updateAnimPacket;
                updateAnimPacket.writeUInt(packetType_serverCommand,packetTypeBits);
                updateAnimPacket.writeString("setItemFireAnim");
                updateAnimPacket.writeUInt(i->serverID,dynamicObjectIDBits);
                updateAnimPacket.writeBit(true);
                updateAnimPacket.writeUInt(a,10);
                updateAnimPacket.writeFloat(animSpeed);
                common_lua->theServer->send(&updateAnimPacket,true);

                return 0;
            }
        }
    }

    i->nextFireAnim = -1;
    packet updateAnimPacket;
    updateAnimPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    updateAnimPacket.writeString("setItemFireAnim");
    updateAnimPacket.writeUInt(i->serverID,dynamicObjectIDBits);
    updateAnimPacket.writeBit(false);
    common_lua->theServer->send(&updateAnimPacket,true);

    return 0;
}

static int setFireSound(lua_State *L)
{
    scope("setFireSound");

    int args = lua_gettop(L);
    if(args < 2 || args > 4)
    {
        error("Function expects 2 to 4 arguments!");
        return 0;
    }

    float pitch = 1.0;
    float gain = 1.0;
    if(args == 4)
    {
        gain = lua_tonumber(L,-1);
        lua_pop(L,1);
        if(gain < 0 || gain > 1)
        {
            error("Gain must be between 0 and 1");
            gain = 1.0;
        }
    }

    if(args == 3 || args == 4)
    {
        pitch = lua_tonumber(L,-1);
        lua_pop(L,1);
        if(pitch <= 0 || pitch > 10)
        {
            error("Pitch must be between 0 and 10");
            pitch = 10.0;
        }
    }

    const char *soundName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!soundName)
    {
        error("Invalid string param");
        return 0;
    }

    item *i = popItem(L);

    if(!i)
    {
        error("Invalid item!");
        return 0;
    }

    std::string nameStr = std::string(soundName);

    if(nameStr.length() > 0)
    {
        for(int a = 0; a<common_lua->soundScriptNames.size(); a++)
        {
            if(common_lua->soundScriptNames[a] == nameStr)
            {
                i->nextFireSound = a;
                i->nextFireSoundGain = gain;
                i->nextFireSoundPitch = pitch;

                packet updateSoundPacket;
                updateSoundPacket.writeUInt(packetType_serverCommand,packetTypeBits);
                updateSoundPacket.writeString("setItemFireSound");
                updateSoundPacket.writeUInt(i->serverID,dynamicObjectIDBits);
                updateSoundPacket.writeBit(true);
                updateSoundPacket.writeUInt(a,10);
                updateSoundPacket.writeFloat(pitch);
                updateSoundPacket.writeFloat(gain);
                common_lua->theServer->send(&updateSoundPacket,true);

                return 0;
            }
        }
    }

    i->nextFireSound = -1;
    packet updateSoundPacket;
    updateSoundPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    updateSoundPacket.writeString("setItemFireSound");
    updateSoundPacket.writeUInt(i->serverID,dynamicObjectIDBits);
    updateSoundPacket.writeBit(false);
    common_lua->theServer->send(&updateSoundPacket,true);

    return 0;
}

static int setItemCooldown(lua_State *L)
{
    scope("setItemCooldown");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Function expects 2 arguments!");
        return 0;
    }

    int ms = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(ms < 0 || ms > 64000)
    {
        error("Milliseconds needs to be 0 to 64000ms!");
        return 0;
    }

    item *i = popItem(L);

    if(!i)
    {
        error("Invalid item!");
        return 0;
    }

    i->fireCooldownMS = ms;
    packet updateSoundPacket;
    updateSoundPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    updateSoundPacket.writeString("setItemCooldown");
    updateSoundPacket.writeUInt(i->serverID,dynamicObjectIDBits);
    updateSoundPacket.writeUInt(i->fireCooldownMS,16);
    common_lua->theServer->send(&updateSoundPacket,true);

    return 0;
}

static int getItemTypeName(lua_State *L)
{
    scope("getItemTypeName");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Expected one argument!");
        lua_pushnil(L);
        return 1;
    }

    item *i = popItem(L);
    if(!i)
    {
        error("No item!");
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L,i->heldItemType->uiName.c_str());
    return 1;
}

static int setFireEmitter(lua_State *L)
{
    scope("setFireEmitter");

    int args = lua_gettop(L);
    if(args < 2 || args > 3)
    {
        error("Function expects 2 or 3 arguments!");
        return 0;
    }

    std::string meshName = "";

    if(args == 3)
    {
        const char *mesh = lua_tostring(L,-1);
        lua_pop(L,1);
        if(!mesh)
        {
            error("Invalid mesh name string!");
            return 0;
        }
        meshName = std::string(mesh);
    }

    const char *emitterName = lua_tostring(L,-1);
    lua_pop(L,1);
    if(!emitterName)
    {
        error("Invalid emitter name string");
        return 0;
    }

    std::string emitterStr = std::string(emitterName);

    item *i = popItem(L);
    if(!i)
    {
        error("Invalid item!");
        return 0;
    }

    if(emitterStr.length() > 0)
    {
        for(int a = 0; a<common_lua->emitterTypes.size(); a++)
        {
            if(common_lua->emitterTypes[a]->dbname == emitterStr)
            {
                i->nextFireEmitter = a;
                i->nextFireEmitterMesh = meshName;

                packet updateFireEmitter;
                updateFireEmitter.writeUInt(packetType_serverCommand,packetTypeBits);
                updateFireEmitter.writeString("setItemFireEmitter");
                updateFireEmitter.writeUInt(i->serverID,dynamicObjectIDBits);
                updateFireEmitter.writeBit(true);
                updateFireEmitter.writeUInt(a,10);
                updateFireEmitter.writeString(meshName);
                common_lua->theServer->send(&updateFireEmitter,true);

                return 0;
            }
        }
    }

    i->nextFireEmitter = -1;

    packet updateFireEmitter;
    updateFireEmitter.writeUInt(packetType_serverCommand,packetTypeBits);
    updateFireEmitter.writeString("setItemFireEmitter");
    updateFireEmitter.writeUInt(i->serverID,dynamicObjectIDBits);
    updateFireEmitter.writeBit(false);
    common_lua->theServer->send(&updateFireEmitter,true);

    return 0;
}

static int setPerformRaycast(lua_State *L)
{
    scope("setPerformRaycast");

    bool useBulletTrail = false;
    float r=1.0,g=0.5,b=0.0,speed=1.0;

    int args = lua_gettop(L);
    if(!(args == 2 || args == 7))
    {
        error("Function expects 2 or 7 arguments!");
        return 0;
    }

    if(args == 7)
    {
        speed = lua_tonumber(L,-1);
        lua_pop(L,1);
        b = lua_tonumber(L,-1);
        lua_pop(L,1);
        g = lua_tonumber(L,-1);
        lua_pop(L,1);
        r = lua_tonumber(L,-1);
        lua_pop(L,1);
        useBulletTrail = lua_toboolean(L,-1);
        lua_pop(L,1);
    }

    bool raycast = lua_toboolean(L,-1);
    lua_pop(L,1);

    item *i = popItem(L);

    if(args == 7)
    {
        i->useBulletTrail = useBulletTrail;
        i->bulletTrailColor = btVector3(r,g,b);
        i->bulletTrailSpeed = speed;
    }

    if(!i)
    {
        error("Invalid item!");
        return 0;
    }

    packet updatePerformRaycast;
    updatePerformRaycast.writeUInt(packetType_serverCommand,packetTypeBits);
    updatePerformRaycast.writeString("setItemBulletTrail");
    updatePerformRaycast.writeUInt(i->serverID,dynamicObjectIDBits);
    updatePerformRaycast.writeBit(i->useBulletTrail);
    updatePerformRaycast.writeFloat(i->bulletTrailColor.x());
    updatePerformRaycast.writeFloat(i->bulletTrailColor.y());
    updatePerformRaycast.writeFloat(i->bulletTrailColor.z());
    updatePerformRaycast.writeFloat(i->bulletTrailSpeed);
    common_lua->theServer->send(&updatePerformRaycast,true);

    i->performRaycast = raycast;

    return 0;
}

void registerItemFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAitemToString},
        {"__eq",LUAitemEqual},
        {"remove",LUAremoveItem},
        {"getHolder",getHolder},
        {"setFireAnim",setFireAnim},
        {"setFireSound",setFireSound},
        {"getTypeName",getItemTypeName},
        {"setItemCooldown",setItemCooldown},
        {"setFireEmitter",setFireEmitter},
        {"setPerformRaycast",setPerformRaycast},
        {NULL,NULL}};
    luaL_newmetatable(L,"itemMETATABLE");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"itemMETATABLE");

    //Register functions
    lua_register(L,"getItemIdx",getItemIdx);
    lua_register(L,"getNumItems",getNumItems);
    lua_register(L,"addItem",addItemLua);
}



