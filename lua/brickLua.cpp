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

    pushBrick(L,common_lua->bricks[idx]);

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

            pushBrick(L,common_lua->namedBricks[a].bricks[idx]);

            return 1;
        }
    }

    error("No bricks with given name " + std::to_string(idx));
    lua_pushnil(L);
    return 1;
}

brick *lastBrickAtHit = 0;
bool getBrickAtSearch(brick *val)
{
    lastBrickAtHit = val;
    return false;
}

static int getBrickAt(lua_State *L)
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

    double fxd = x;
    double fyd = y;
    double fzd = z;

    double searchMin[3];
    searchMin[0] = fxd + 0.01;
    searchMin[1] = fyd + 0.01;
    searchMin[2] = fzd + 0.01;

    double searchMax[3];
    searchMax[0] = fxd + 0.99;
    searchMax[1] = fyd + 0.99;
    searchMax[2] = fzd + 0.99;

    lastBrickAtHit = 0;

    int hits = common_lua->overlapTree->Search(searchMin,searchMax,getBrickAtSearch);

    if(hits < 1)
    {
        lua_pushnil(L);
        return 1;
    }

    if(!lastBrickAtHit)
    {
        lua_pushnil(L);
        return 1;
    }

    pushBrick(L,lastBrickAtHit);

    return 1;
}

static int getBrickName(lua_State *L)
{
    scope("getBrickName");

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        lua_pushnil(L);
        return 1;
    }

    if(passedBrick->builtBy == -1 || passedBrick->builtBy == 0)
    {
        lua_pushnil(L);
        return 1;
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->accountID == (unsigned int)passedBrick->builtBy)
        {
            pushClient(L,common_lua->users[a]);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

static int removeBrick(lua_State *L)
{
    scope("removeBrick");

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

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

    brick *passedBrick = popBrick(L);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->setBrickPosition(passedBrick,x,y,z);

    return 0;
}

static int setBrickName(lua_State *L)
{
    scope("setBrickName");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Function requires 2 arguments!");
        lua_pop(L,args);
        return 0;
    }

    const char *str = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!str)
    {
        error("Invalid string argument!");
        return 0;
    }

    brick *passedBrick = popBrick(L);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->setBrickName(passedBrick,std::string(str));

    return 0;
}

static int setBrickAngleID(lua_State *L)
{
    scope("setBrickAngleID");

    int angleID = lua_tointeger(L,-1);
    lua_pop(L,1);

    brick *passedBrick = popBrick(L);

    if(!passedBrick)
    {
        error("Brick with empty pointer passed!");
        return 0;
    }

    common_lua->setBrickAngleID(passedBrick,angleID);

    return 0;
}

static int addBasicBrickLua(lua_State *L)
{
    scope("addBasicBrick");

    int args = lua_gettop(L);

    if(args != 12)
    {
        error("This function requires 12 arguments.");
        error("addBasicBrick(x,y,z,r,g,b,a,width,length,height,angleID,material");
        lua_pushnil(L);
        return 1;
    }

    int material = lua_tointeger(L,-1);
    lua_pop(L,1);

    int angleID = lua_tointeger(L,-1);
    lua_pop(L,1);

    int height = lua_tointeger(L,-1);
    lua_pop(L,1);
    int length = lua_tointeger(L,-1);
    lua_pop(L,1);
    int width = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(length < 1 || width < 1 || height < 1)
    {
        error("A dimension was zero or negative!");
        lua_pushnil(L);
        return 1;
    }
    if(length > 255 || width > 255 || height > 255)
    {
        error("A dimension was greater than 255!");
        lua_pushnil(L);
        return 1;
    }
    if(angleID < 0 || angleID > 3)
    {
        error("AngleID was not between 0 and 3!");
        lua_pushnil(L);
        return 1;
    }

    float al = lua_tonumber(L,-1);
    lua_pop(L,1);
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

    brick *tmp = new brick;
    tmp->isSpecial = false;

    tmp->posX = x;
    tmp->uPosY = y;
    tmp->posZ = z;

    tmp->yHalfPosition = height % 2;
    if(angleID % 2 == 0)
    {
        tmp->xHalfPosition = width % 2;
        tmp->zHalfPosition = length % 2;
    }
    else
    {
        tmp->zHalfPosition = width % 2;
        tmp->xHalfPosition = length % 2;
    }

    tmp->r = r;
    tmp->g = g;
    tmp->b = b;
    tmp->a = al;

    tmp->angleID = angleID;
    tmp->material = material;

    tmp->width = width;
    tmp->length = length;
    tmp->height = height;

    if(!common_lua->addBrick(tmp,false,true,false))
    {
        delete tmp;
        lua_pushnil(L);
        return 1;
    }
    else
        common_lua->bricksAddedThisFrame.push_back(tmp);

    //Register an instance of brick
    pushBrick(L,tmp);

    return 1;
}

static int addSpecialBrickLua(lua_State *L)
{
    scope("addSpecialBrick");

    int args = lua_gettop(L);

    if(args != 10)
    {
        error("This function requires 10 arguments.");
        error("addBasicBrick(x,y,z,r,g,b,a,typeID,angleID,material");
        lua_pushnil(L);
        return 1;
    }

    int material = lua_tointeger(L,-1);
    lua_pop(L,1);
    int angleID = lua_tointeger(L,-1);
    lua_pop(L,1);

    int typeID = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(typeID < 0)
    {
        error("TypeID < 0");
        lua_pushnil(L);
        return 1;
    }
    if(typeID >= common_lua->brickTypes->brickTypes.size())
    {
        error("TypeID out of range!");
        lua_pushnil(L);
        return 1;
    }
    if(angleID < 0 || angleID > 3)
    {
        error("AngleID was not between 0 and 3!");
        lua_pushnil(L);
        return 1;
    }

    float al = lua_tonumber(L,-1);
    lua_pop(L,1);
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

    brick *tmp = new brick;
    tmp->isSpecial = true;
    tmp->typeID = typeID;

    tmp->posX = x;
    tmp->uPosY = y;
    tmp->posZ = z;

    tmp->width = common_lua->brickTypes->brickTypes[tmp->typeID]->width;
    tmp->height = common_lua->brickTypes->brickTypes[tmp->typeID]->height;
    tmp->length = common_lua->brickTypes->brickTypes[tmp->typeID]->length;

    tmp->yHalfPosition = tmp->height % 2;
    if(angleID % 2 == 0)
    {
        tmp->xHalfPosition = tmp->width % 2;
        tmp->zHalfPosition = tmp->length % 2;
    }
    else
    {
        tmp->zHalfPosition = tmp->width % 2;
        tmp->xHalfPosition = tmp->length % 2;
    }

    tmp->r = r;
    tmp->g = g;
    tmp->b = b;
    tmp->a = al;

    tmp->angleID = angleID;
    tmp->material = material;

    if(!common_lua->addBrick(tmp,false,true,false))
    {
        delete tmp;
        lua_pushnil(L);
        return 1;
    }
    else
        common_lua->bricksAddedThisFrame.push_back(tmp);

    //Register an instance of brick
    pushBrick(L,tmp);

    return 1;
}

static int getSpecialType(lua_State *L)
{
    scope("getSpecialType");
    int args = lua_gettop(L);
    if(args != 1)
    {
        error("This function requires one argument!");
        lua_pushinteger(L,0);
        return 1;
    }
    const char *str = lua_tostring(L,-1);
    lua_pop(L,1);
    if(!str)
    {
        error("Error getting string argument!");
        lua_pushinteger(L,0);
        return 1;
    }
    std::string dbStr = lowercase(str);

    for(int a = 0; a<common_lua->brickTypes->brickTypes.size(); a++)
    {
        if(lowercase(common_lua->brickTypes->brickTypes[a]->uiname) == dbStr)
        {
            lua_pushinteger(L,a);
            return 1;
        }
        std::string checkedDB = lowercase(getFileFromPath(common_lua->brickTypes->brickTypes[a]->fileName));
        if(checkedDB.length() > 4)
        {
            if(checkedDB.substr(checkedDB.length()-4,4) == ".blb")
                checkedDB = checkedDB.substr(0,checkedDB.length()-4);
            else if(checkedDB.substr(checkedDB.length()-3,3) == "blb")
                checkedDB = checkedDB.substr(0,checkedDB.length()-3);
        }

        if(checkedDB == dbStr)
        {
            lua_pushinteger(L,a);
            return 1;
        }
    }

    lua_pushinteger(L,0);
    return 1;
}

static int getOwnerID(lua_State *L)
{
    scope("getOwnerID");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Wrong amount of arguments!");
        lua_pushnumber(L,0);
        return 1;
    }

    brick *passedBrick = popBrick(L);

    if(!passedBrick)
    {
        error("Invalid brick!");
        lua_pushnumber(L,0);
        return 1;
    }

    lua_pushnumber(L,passedBrick->builtBy);
    return 1;
}

void registerBrickFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAbrickToString},
        {"getName",getBrickName},
        {"setName",setBrickName},
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
        {"getOwnerID",getOwnerID},
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
    lua_register(L,"getBrickAt",getBrickAt);
    lua_register(L,"addBasicBrick",addBasicBrickLua);
    lua_register(L,"addSpecialBrick",addSpecialBrickLua);
    lua_register(L,"getSpecialType",getSpecialType);
}


















