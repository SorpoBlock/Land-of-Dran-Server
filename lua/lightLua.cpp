#include "lightLua.h"

static int LUAlightToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    std::string ret = "(Light " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

light *popLight(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("Bad light object passed " + std::to_string(id));
        return 0;
    }

    for(int a = 0; a<common_lua->lights.size(); a++)
        if(common_lua->lights[a]->serverID == id)
            return common_lua->lights[a];

    error("Light index/object not found " + std::to_string(id));
    return 0;
}

void pushLight(lua_State *L,light *otherL)
{
    //Register an instance of brick
    lua_newtable(L);
    lua_getglobal(L,"lightMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,otherL->serverID);
    lua_setfield(L,-2,"id");
    lua_pushstring(L,"light");
    lua_setfield(L,-2,"type");
}

static int luaLightRemove(lua_State *L)
{
    light *l = popLight(L);
    if(l)
        common_lua->removeLight(l);
}

static int addLight(lua_State *L)
{
    scope("addLight");

    int args = lua_gettop(L);
    if(args == 6)
    {
        float b = lua_tonumber(L,-1);
        lua_pop(L,1);
        float g = lua_tonumber(L,-1);
        lua_pop(L,1);
        float r = lua_tonumber(L,-1);
        lua_pop(L,1);
        float z = lua_tonumber(L,-1);
        lua_pop(L,1);
        float y = lua_tonumber(L,-1);
        lua_pop(L,1);
        float x = lua_tonumber(L,-1);
        lua_pop(L,1);

        light *l = common_lua->addLight(btVector3(r,g,b),btVector3(x,y,z));
        pushLight(L,l);
        return 1;
    }
    else if(args == 4)
    {
        float b = lua_tonumber(L,-1);
        lua_pop(L,1);
        float g = lua_tonumber(L,-1);
        lua_pop(L,1);
        float r = lua_tonumber(L,-1);
        lua_pop(L,1);

        lua_getfield(L, -1, "pointer");
        brick *passedBrick = (brick*)lua_touserdata(L,-1);
        lua_pop(L,2);

        if(!passedBrick)
        {
            error("Invalid brick pointer!");
            lua_pushnil(L);
            return 1;
        }

        light *l = common_lua->addLight(btVector3(r,g,b),passedBrick);
        pushLight(L,l);
        return 1;
    }
    else
    {
        error("Function requires 6 or 4 arguments!");
        lua_pushnil(L);
        return 1;
    }
}

static int setPhi(lua_State *L)
{
    scope("setPhi");

    float phi = lua_tonumber(L,-1);
    lua_pop(L,1);
    light *l = popLight(L);

    if(!l)
    {
        error("Bad light provided!");
        return 0;
    }

    phi = fabs(phi);

    if(phi >= 6.2831855)
        l->isSpotlight = false;
    else
    {
        l->isSpotlight = true;
        l->direction.setW(phi);
    }

    for(int a = 0; a<common_lua->users.size(); a++)
        l->sendToClient(common_lua->users[a]->netRef);

    return 0;
}

static int setSpotlightDirection(lua_State *L)
{
    scope("setSpotlightDirection");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    light *l = popLight(L);

    if(!l)
    {
        error("Bad light provided!");
        return 0;
    }

    l->direction.setX(x);
    l->direction.setY(y);
    l->direction.setZ(z);

    for(int a = 0; a<common_lua->users.size(); a++)
        l->sendToClient(common_lua->users[a]->netRef);

    return 0;
}

void registerLightFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAlightToString},
        {"remove",luaLightRemove},
        {"setPhi",setPhi},
        {"setSpotlightDirection",setSpotlightDirection},
        {NULL,NULL}};
    luaL_newmetatable(L,"lightMETATABLE");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"lightMETATABLE");

    //Register functions
    lua_register(L,"addLight",addLight);
}

