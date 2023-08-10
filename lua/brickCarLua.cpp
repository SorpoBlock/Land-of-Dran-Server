#include "brickCarLua.h"

static int LUAbrickCarToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    std::string ret = "(Brick Car " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

static int getNumBrickCars(lua_State *L)
{
    lua_pushnumber(L,common_lua->brickCars.size());
    return 1;
}

static int getBrickCarIdx(lua_State *L)
{
    scope("getBrickCarIdx");

    unsigned int id = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(id >= common_lua->brickCars.size())
    {
        error("Brick car index out of range!");
        lua_pushnil(L);
        return 1;
    }

    luaPushBrickCar(L,common_lua->brickCars[id]);
    return 1;
}

static int removeBrickCar(lua_State *L)
{
    scope("removeBrickCar");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    common_lua->removeBrickCar(passedBrickCar);

    return 0;
}

static int brickCarGetNumBricks(lua_State *L)
{
    scope("brickCarGetNumBricks");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    lua_pushnumber(L,passedBrickCar->bricks.size());
    return 1;
}

static int brickCarGetOwner(lua_State *L)
{
    scope("brickCarGetOwner");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    int ownerID = passedBrickCar->ownerID;
    if(ownerID < 0)
    {
        error("Car never had an owner? (Made by script?");
        lua_pushnil(L);
        return 1;
    }

    for(int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->accountID == ownerID)
        {
            pushClient(L,common_lua->users[a]);
            return 1;
        }
    }

    error("Car owner not found, maybe they left the game?");
    lua_pushnil(L);
    return 1;
}

static int brickCarGetPosition(lua_State *L)
{
    scope("brickCarGetPosition");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    btVector3 pos = passedBrickCar->body->getWorldTransform().getOrigin();

    lua_pushnumber(L,pos.x());
    lua_pushnumber(L,pos.y());
    lua_pushnumber(L,pos.z());
    return 3;
}

static int brickCarGetVelocity(lua_State *L)
{
    scope("brickCarGetVelocity");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    btVector3 pos = passedBrickCar->body->getLinearVelocity();

    lua_pushnumber(L,pos.x());
    lua_pushnumber(L,pos.y());
    lua_pushnumber(L,pos.z());
    return 3;
}

static int brickCarGetAngularVelocity(lua_State *L)
{
    scope("brickCarGetAngularVelocity");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    btVector3 pos = passedBrickCar->body->getAngularVelocity();

    lua_pushnumber(L,pos.x());
    lua_pushnumber(L,pos.y());
    lua_pushnumber(L,pos.z());
    return 3;
}

static int brickCarGetRotation(lua_State *L)
{
    scope("brickCarGetRotation");

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    btQuaternion rot = passedBrickCar->body->getWorldTransform().getRotation();

    lua_pushnumber(L,rot.w());
    lua_pushnumber(L,rot.x());
    lua_pushnumber(L,rot.y());
    lua_pushnumber(L,rot.z());
    return 4;
}

static int brickCarSetPosition(lua_State *L)
{
    scope("brickCarSetPosition");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    btTransform t = passedBrickCar->body->getWorldTransform();
    t.setOrigin(btVector3(x,y,z));
    passedBrickCar->body->setWorldTransform(t);

    return 0;
}

static int brickCarSetAngularVelocity(lua_State *L)
{
    scope("brickCarSetAngularVelocity");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    passedBrickCar->body->setAngularVelocity(btVector3(x,y,z));

    return 0;
}

static int brickCarSetVelocity(lua_State *L)
{
    scope("brickCarSetVelocity");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    passedBrickCar->body->setLinearVelocity(btVector3(x,y,z));

    return 0;
}

static int brickCarSetGravity(lua_State *L)
{
    scope("brickCarSetGravity");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    passedBrickCar->body->setGravity(btVector3(x,y,z));

    return 0;
}

static int brickCarSetRotation(lua_State *L)
{
    scope("brickCarSetRotation");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);
    float w = lua_tonumber(L,-1);
    lua_pop(L,1);

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Brick car with empty pointer passed!");
        return 0;
    }

    btTransform t = passedBrickCar->body->getWorldTransform();
    t.setRotation(btQuaternion(x,y,z,w));
    passedBrickCar->body->setWorldTransform(t);

    return 0;
}

static int brickCarGetDriver(lua_State *L)
{
    scope("brickCarGetDriver");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Function passed with too many args, expected 1");
        lua_pushnil(L);
        return 1;
    }

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Invalid pointer to brick car!");
        lua_pushnil(L);
        return 1;
    }

    if(!passedBrickCar->occupied)
    {
        lua_pushnil(L);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->driving == passedBrickCar)
        {
            pushClient(L,common_lua->users[a]);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int getBrickcarOwnerID(lua_State *L)
{
    scope("getBrickcarOwnerID");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Wrong amount of arguments!");
        lua_pushnumber(L,0);
        return 1;
    }

    brickCar *passedBrickCar = popBrickCar(L);

    if(!passedBrickCar)
    {
        error("Invalid brick!");
        lua_pushnumber(L,0);
        return 1;
    }

    lua_pushnumber(L,passedBrickCar->ownerID);
    return 1;
}

void registerBrickCarFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAbrickCarToString},
        {"remove",removeBrickCar},
        {"getNumBricks",brickCarGetNumBricks},
        {"getPosition",brickCarGetPosition},
        {"getVelocity",brickCarGetVelocity},
        {"setVelocity",brickCarSetVelocity},
        {"setPosition",brickCarSetPosition},
        {"getBuilder",brickCarGetOwner},
        {"getDriver",brickCarGetDriver},
        {"getOwnerID",getBrickcarOwnerID},
        {"setRotation",brickCarSetRotation},
        {"setGravity",brickCarSetGravity},
        {"setAngularVelocity",brickCarSetAngularVelocity},
        {"getAngularVelocity",brickCarGetAngularVelocity},
        {"getRotation",brickCarGetRotation},
        {NULL,NULL}};
    luaL_newmetatable(L,"brickCarMETATABLE");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"brickCarMETATABLE");

    //Register functions
    lua_register(L,"getNumBrickCars",getNumBrickCars);
    lua_register(L,"getBrickCarIdx",getBrickCarIdx);
}
