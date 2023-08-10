#include "pushPopClasses.h"

void pushDynamic(lua_State *L,dynamic *object)
{
    if(!object)
    {
        lua_pushnil(L);
        return;
    }

    lua_newtable(L);
    lua_getglobal(L,"dynamic");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,object->serverID);
    lua_setfield(L,-2,"id");
}

dynamic *popDynamic(lua_State *L,bool supressErrors)
{
    if(!lua_istable(L,-1))
    {
        lua_pop(L,1);
        if(!supressErrors)
            error("No table for dynamic argument!");
        return 0;
    }

    lua_getfield(L, -1, "id");
    if(!lua_isinteger(L,-1))
    {
        if(!supressErrors)
            error("No ID for dynamic argument!");
        lua_pop(L,2);
        return 0;
    }

    int dynamicID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
        if(common_lua->dynamics[a]->serverID == dynamicID)
            return common_lua->dynamics[a];

    error("Dynamic ID no longer valid");
    return 0;
}

void pushClient(lua_State *L,clientData* client)
{
    if(!client)
    {
        lua_pushnil(L);
        return;
    }

    lua_newtable(L);
    lua_getglobal(L,"clientMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,client->playerID);
    lua_setfield(L,-2,"id");
}

clientData* popClient(lua_State *L)
{
    if(!lua_istable(L,-1))
    {
        lua_pop(L,1);
        error("No table for client argument!");
        return 0;
    }

    lua_getfield(L, -1, "id");
    if(!lua_isinteger(L,-1))
    {
        error("No ID for client argument!");
        lua_pop(L,2);
        return 0;
    }

    int clientID = lua_tointeger(L,-1);
    lua_pop(L,2);

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
        if(common_lua->users[a]->playerID == clientID)
            return common_lua->users[a];

    error("Client ID no longer valid");
    return 0;
}

void pushBrick(lua_State *L,brick *toPush)
{
    if(!toPush)
    {
        lua_pushnil(L);
        return;
    }

    //Register an instance of brick
    lua_newtable(L);
    lua_getglobal(L,"brickMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,toPush->serverID);
    lua_setfield(L,-2,"id");
    lua_pushlightuserdata(L,toPush);
    lua_setfield(L,-2,"pointer");
    lua_pushstring(L,"brick");
    lua_setfield(L,-2,"type");
}

brick *popBrick(lua_State *L)
{
    if(!lua_istable(L,-1))
    {
        lua_pop(L,1);
        error("No table for brick argument!");
        return 0;
    }

    lua_getfield(L, -1, "pointer");
    if(!lua_isuserdata(L,-1))
    {
        lua_pop(L,2);
        error("No userdata for brick argument!");
        return 0;
    }

    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    return passedBrick;
}

void luaPushBrickCar(lua_State *L,brickCar *theCar)
{
    if(!theCar)
    {
        lua_pushnil(L);
        return;
    }

    lua_newtable(L);
    lua_getglobal(L,"brickCarMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,theCar->serverID);
    lua_setfield(L,-2,"id");
    lua_pushlightuserdata(L,theCar);
    lua_setfield(L,-2,"pointer");
    lua_pushstring(L,"brickCar");
    lua_setfield(L,-2,"type");
}

brickCar *popBrickCar(lua_State *L)
{
    if(!lua_istable(L,-1))
    {
        lua_pop(L,1);
        error("No table for brickcar argument!");
        return 0;
    }

    lua_getfield(L, -1, "pointer");
    if(!lua_isuserdata(L,-1))
    {
        lua_pop(L,2);
        error("No userdata for brickcar argument!");
        return 0;
    }

    brickCar *passedBrickcar = (brickCar*)lua_touserdata(L,-1);
    lua_pop(L,2);

    return passedBrickcar;
}

void pushEmitter(lua_State *L,emitter *e)
{
    if(!e)
    {
        lua_pushnil(L);
        return;
    }

    lua_newtable(L);
    lua_getglobal(L,"emitterMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,e->serverID);
    lua_setfield(L,-2,"id");
    lua_pushlightuserdata(L,e);
    lua_setfield(L,-2,"pointer");
    lua_pushstring(L,"emitter");
    lua_setfield(L,-2,"type");
}

emitter *popEmitter(lua_State *L)
{
    int args = lua_gettop(L);
    if(args < 1)
        return 0;
    if(lua_istable(L,-1))
    {
        lua_getfield(L, -1, "pointer");
        emitter *ret = (emitter*)lua_touserdata(L,-1);
        lua_pop(L,2);
        return ret;
    }
    else
        return 0;
}

