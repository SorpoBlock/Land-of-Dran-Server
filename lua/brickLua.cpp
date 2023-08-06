#include "code/lua/brickLua.h"

static int LUAbrickToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    std::string ret = "(Brick " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

static int getNumBricks(lua_State *L)
{
    lua_pushnumber(L,common_lua->bricks.size());
    return 1;
}

static int getBrickIdx(lua_State *L)
{
    scope("getBrickIdx");

    int idx = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(idx < 0 || (int)idx >= (int)common_lua->bricks.size())
    {
        error("invalid index " + std::to_string(idx));
        lua_pushnil(L);
        return 1;
    }

    //Register an instance of brick
    lua_newtable(L);
    lua_getglobal(L,"brickMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,common_lua->bricks[idx]->serverID);
    lua_setfield(L,-2,"id");
    lua_pushlightuserdata(L,common_lua->bricks[idx]);
    lua_setfield(L,-2,"pointer");
    lua_pushstring(L,"brick");
    lua_setfield(L,-2,"type");

    return 1;
}

static int getNumNamedBricks(lua_State *L)
{
    scope("getNumNamedBricks");

    const char* name = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!name)
    {
        error("No name specified.");
        lua_pushnumber(L,0);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->namedBricks.size(); a++)
    {
        if(common_lua->namedBricks[a].name == std::string(name))
        {
            lua_pushnumber(L,common_lua->namedBricks[a].bricks.size());
            return 1;
        }
    }

    lua_pushnumber(L,0);
    return 1;
}

static int getNamedBrickIdx(lua_State *L)
{
    scope("getNamedBrickIdx");

    int idx = lua_tointeger(L,-1);
    lua_pop(L,1);

    const char* name = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!name)
    {
        error("No named specified.");
        lua_pushnil(L);
        return 1;
    }

    if(idx < 0)
    {
        error("negative index " + std::to_string(idx));
        lua_pushnil(L);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->namedBricks.size(); a++)
    {
        if(common_lua->namedBricks[a].name == std::string(name))
        {
            if((unsigned int)idx >= common_lua->namedBricks[a].bricks.size())
            {
                error("Index too large " + std::to_string(idx) + " for name " + std::string(name));
                lua_pushnil(L);
                return 1;
            }

            lua_newtable(L);
            lua_getglobal(L,"brickMETATABLE");
            lua_setmetatable(L,-2);
            lua_pushinteger(L,common_lua->namedBricks[a].bricks[idx]->serverID);
            lua_setfield(L,-2,"id");
            lua_pushlightuserdata(L,common_lua->namedBricks[a].bricks[idx]);
            lua_setfield(L,-2,"pointer");
            lua_pushstring(L,"brick");
            lua_setfield(L,-2,"type");

            return 1;
        }
    }

    error("No bricks with given name " + std::to_string(idx));
    lua_pushnil(L);
    return 1;
}

/*static int getBrickAt(lua_State *L)
{
    scope("getBrickAt");

    int z = lua_tointeger(L,-1);
    lua_pop(L,1);
    int y = lua_tointeger(L,-1);
    lua_pop(L,1);
    int x = lua_tointeger(L,-1);
    lua_pop(L,1);

    x += brickTreeSize;
    z += brickTreeSize;

    brick *result = common_lua->tree->at(x,y,z);
    if(!result)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    lua_getglobal(L,"brickMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,result->serverID);
    lua_setfield(L,-2,"id");
    lua_pushlightuserdata(L,result);
    lua_setfield(L,-2,"pointer");
    lua_pushstring(L,"brick");
    lua_setfield(L,-2,"type");

    return 1;
}*/

static int getBrickName(lua_State *L)
{
    scope("getBrickName");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L,passedBrick->name.c_str());
    return 1;
}

static int getBrickPosition(lua_State *L)
{
    scope("getBrickPosition");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L,passedBrick->getX());
    lua_pushnumber(L,passedBrick->getY());
    lua_pushnumber(L,passedBrick->getZ());
    return 3;
}

static int getBrickColor(lua_State *L)
{
    scope("getBrickColor");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L,passedBrick->r);
    lua_pushnumber(L,passedBrick->g);
    lua_pushnumber(L,passedBrick->b);
    lua_pushnumber(L,passedBrick->a);
    return 4;
}

static int getBrickDimensions(lua_State *L)
{
    scope("getBrickDimensions");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L,passedBrick->width);
    lua_pushnumber(L,passedBrick->height);
    lua_pushnumber(L,passedBrick->length);
    return 3;
}

static int isBrickSpecial(lua_State *L)
{
    scope("isBrickSpecial");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    lua_pushboolean(L,passedBrick->isSpecial);
    return 1;
}

static int getBrickTypeID(lua_State *L)
{
    scope("getBrickTypeID");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    if(passedBrick->isSpecial)
    {
        lua_pushnumber(L,passedBrick->typeID);
        return 1;
    }
    else
    {
        lua_pushnil(L);
        return 1;
    }
}

static int getBrickOwner(lua_State *L)
{
    scope("getBrickOwner");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    if(passedBrick->builtBy == -1)
    {
        lua_pushnil(L);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->playerID == (unsigned int)passedBrick->builtBy)
        {
            lua_newtable(L);
            lua_getglobal(L,"client");
            lua_setmetatable(L,-2);
            lua_pushinteger(L,common_lua->users[a]->playerID);
            lua_setfield(L,-2,"id");
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int removeBrick(lua_State *L)
{
    scope("removeBrick");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->removeBrick(passedBrick);

    return 0;
}

static int setBrickColliding(lua_State *L)
{
    scope("setBrickColliding");

    bool colliding = lua_toboolean(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    if(!passedBrick->body)
    {
        error("Brick missing body!");
        return 0;
    }

    //std::cout<<"Setting brick collision..."<<passedBrick<<" "<<passedBrick->serverID<<"\n";

    /*if(passedBrick->body && !colliding)
    {
        //std::cout<<"Removing body...\n";
        common_lua->physicsWorld->removeRigidBody(passedBrick->body);
        delete passedBrick->body;
        passedBrick->body = 0;
        packet update;
        passedBrick->createUpdatePacket(&update);
        common_lua->theServer->send(&update,true);
    }
    else if(passedBrick->body == 0 && colliding)
    {
        //std::cout<<"Adding body...\n";
        common_lua->brickTypes->addPhysicsToBrick(passedBrick,common_lua->physicsWorld);
        packet update;
        passedBrick->createUpdatePacket(&update);
        common_lua->theServer->send(&update,true);
    }*/

    passedBrick->setColliding(common_lua->physicsWorld,colliding);

    packet update;
    passedBrick->createUpdatePacket(&update);
    common_lua->theServer->send(&update,true);

    return 0;
}

static int setBrickColor(lua_State *L)
{
    scope("setBrickColor");

    float a = lua_tonumber(L,-1);
    lua_pop(L,1);
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
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->setBrickColor(passedBrick,r,g,b,a);

    return 0;
}

static int setBrickPosition(lua_State *L)
{
    scope("setBrickPosition");

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->setBrickPosition(passedBrick,x,y,z);

    return 0;
}

static int setBrickAngleID(lua_State *L)
{
    scope("setBrickAngleID");

    int angleID = lua_tointeger(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->setBrickAngleID(passedBrick,angleID);

    return 0;
}

void registerBrickFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAbrickToString},
        {"getName",getBrickName},
        {"getPosition",getBrickPosition},
        {"getColor",getBrickColor},
        {"getDimensions",getBrickDimensions},
        {"isSpecial",isBrickSpecial},
        {"getTypeID",getBrickTypeID},
        {"getOwner",getBrickOwner},
        {"remove",removeBrick},
        {"setColliding",setBrickColliding},
        {"setColor",setBrickColor},
        {"setPosition",setBrickPosition},
        {"setAngleID",setBrickAngleID},
        {NULL,NULL}};
    luaL_newmetatable(L,"brickMETATABLE");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"brickMETATABLE");

    //Register functions
    lua_register(L,"getNumBricks",getNumBricks);
    lua_register(L,"getBrickIdx",getBrickIdx);
    lua_register(L,"getNumNamedBricks",getNumNamedBricks);
    lua_register(L,"getNamedBrickIdx",getNamedBrickIdx);
    //lua_register(L,"getBrickAt",getBrickAt);
}




