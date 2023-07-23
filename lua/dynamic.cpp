#include "code/lua/dynamic.h"

static int LUAdynamicEqual(lua_State *L)
{
    int args = lua_gettop(L);
    if(args != 2)
    {
        error("LUAdynamicEqual args were not 2");
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

static int LUAdynamicToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);

    lua_pop(L,2);

    std::string ret = "(Dynamic " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

static int getDynamicIdx(lua_State *L)
{
    int idx = lua_tointeger(L,1);
    lua_pop(L,1);

    if(idx < 0 || (int)idx >= (int)common_lua->dynamics.size())
    {
        error("getdynamicIdx invalid index " + std::to_string(idx));
        lua_pushnil(L);
        return 1;
    }

    //Register an instance of dynamic
    lua_newtable(L);
    lua_getglobal(L,"dynamic");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,common_lua->dynamics[idx]->serverID);
    lua_setfield(L,-2,"id");

    return 1;
}

static int getDynamicOfClient(lua_State *L)
{
    const char* name = lua_tostring(L,1);
    lua_pop(L,1);

    if(!name)
    {
        error("getDynamicOfClient invalid argument");
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

                if(name[i] != c->name[i])
                {
                    match = false;
                    break;
                }
            }

            if(match)
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
    }

    lua_pushnil(L);
    return 1;
}

static int dynamicSetGravity(lua_State *L)
{
    std::cout<<"Stack pointer: "<<lua_gettop(L)<<"\n";
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);
    std::cout<<"Stack pointer: "<<lua_gettop(L)<<"\n";

    if(x > 100 || y > 100 || z > 100 || x < -100 || y < -100 || z < -100)
    {
        error("dynamicSetGravity bad args - x,y,z must be between -100 and 100");
        return 0;
    }

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);
    std::cout<<"Stack pointer: "<<lua_gettop(L)<<"\n";

    std::cout<<"args: "<<id<<","<<x<<","<<y<<","<<z<<"\n";

    if(id < 0)
    {
        error("dynamicSetGravity bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                d->setGravity(btVector3(x,y,z));
                return 0;
            }
        }
    }

    error("dynamicSetGravity bad id " + std::to_string(id));
    return 0;
}

static int setAngularFactor(lua_State *L)
{
    scope("setAngularFactor");
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    if(x > 1 || y > 1 || z > 1 || x < 0 || y < 0 || z < 0)
    {
        error("setAngularFactor bad args - x,y,z must be between 0 and 1");
        return 0;
    }

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                d->setAngularFactor(btVector3(x,y,z));
                return 0;
            }
        }
    }

    error("bad id " + std::to_string(id));
    return 0;
}

static int dynamicSetVelocity(lua_State *L)
{
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("dynamicSetVelocity bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                d->activate();
                d->setLinearVelocity(btVector3(x,y,z));

                if(d->isPlayer)
                {
                    for(int b = 0; b<common_lua->users.size(); b++)
                    {
                        if(common_lua->users[b]->controlling == d)
                        {
                            common_lua->users[b]->forceTransformUpdate();
                        }
                    }
                }

                return 0;
            }
        }
    }

    error("dynamicSetVelocity bad id " + std::to_string(id));
    return 0;
}

static int setPosition(lua_State *L)
{
    scope("setPosition");
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                btTransform t = d->getWorldTransform();
                t.setOrigin(btVector3(x,y,z));
                d->setWorldTransform(t);

                if(d->isPlayer)
                {
                    for(int b = 0; b<common_lua->users.size(); b++)
                    {
                        if(common_lua->users[b]->controlling == d)
                        {
                            common_lua->users[b]->forceTransformUpdate();
                        }
                    }
                }

                return 0;
            }
        }
    }

    error("bad id " + std::to_string(id));
    return 0;
}

static int setAngularVelocity(lua_State *L)
{
    scope("setAngularVelocity");
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                d->setAngularVelocity(btVector3(x,y,z));
                return 0;
            }
        }
    }

    error("bad id " + std::to_string(id));
    return 0;
}

static int setRotation(lua_State *L)
{
    scope("setRotation");
    float w = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                btTransform t = d->getWorldTransform();
                t.setRotation(btQuaternion(x,y,z,w));
                d->setWorldTransform(t);

                if(d->isPlayer)
                {
                    for(int b = 0; b<common_lua->users.size(); b++)
                    {
                        if(common_lua->users[b]->controlling == d)
                        {
                            common_lua->users[b]->forceTransformUpdate();
                        }
                    }
                }
                return 0;
            }
        }
    }

    error("bad id " + std::to_string(id));
    return 0;
}

static int setRestitution(lua_State *L)
{
    scope("setRestitution");
    float rest = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                d->setRestitution(rest);
                return 0;
            }
        }
    }

    error("bad id " + std::to_string(id));
    return 0;
}

static int getPosition(lua_State *L)
{
    scope("setDynamicPositionLua");

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                btVector3 pos = d->getWorldTransform().getOrigin();
                lua_pushnumber(L,pos.x());
                lua_pushnumber(L,pos.y());
                lua_pushnumber(L,pos.z());
                return 3;
            }
        }
    }

    error("bad id " + std::to_string(id));
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
    return 3;
}

static int getNumDynamics(lua_State *L)
{
    lua_pushnumber(L,common_lua->dynamics.size());
    return 1;
}

static int removeDynamic(lua_State *L)
{
    scope("removeDynamicLua");

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                common_lua->removeDynamic(d);
                return 0;
            }
        }
    }

    return 0;
}

static int setShapeName(lua_State *L)
{
    scope("setShapeName");

    int args = lua_gettop(L);

    if(!(args == 2 || args == 5))
    {
        error("setShapeName can have 2 or 5 arguments, was passed: " + std::to_string(args));
        lua_pop(L,args);
        return 0;
    }

    float r,g,b;

    if(args == 5)
    {
        b = lua_tonumber(L,-1);
        lua_pop(L,1);
        g = lua_tonumber(L,-1);
        lua_pop(L,1);
        r = lua_tonumber(L,-1);
        lua_pop(L,1);
    }

    const char *name = lua_tostring(L,-1);
    lua_pop(L,1);
    if(!name)
    {
        error("Invalid string parameter passed!");
        int nowArgs = lua_gettop(L);
        lua_pop(L,nowArgs);
        return 0;
    }

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    if(id < 0)
    {
        error("bad id " + std::to_string(id));
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                if(args == 2)
                {
                    r = d->shapeNameR;
                    g = d->shapeNameG;
                    b = d->shapeNameB;
                }

                common_lua->setShapeName(d,name,r,g,b);

                return 0;
            }
        }
    }

    error("bad id " + std::to_string(id));
    return 0;
}

static int addDynamic(lua_State *L)
{
    scope("addDynamic");

    int typeID = 0;

    int args = lua_gettop(L);
    if(args > 1)
    {
        error("Too many arguments, expected one.");
        lua_pop(L,args);

        lua_pushnil(L);
        return 1;
    }
    else if(args == 1)
    {
        typeID = lua_tointeger(L,-1);
        lua_pop(L,1);
    }
    else
        typeID = 0;


    dynamic *tmp = common_lua->addDynamic(typeID,0,0,0);
    int idx = tmp->serverID;

    //Register an instance of dynamic
    lua_newtable(L);
    lua_getglobal(L,"dynamic");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,idx);
    lua_setfield(L,-2,"id");

    return 1;
}

static int setNodeColor(lua_State *L)
{
    scope("setNodeColor");

    int args = lua_gettop(L);
    if(args != 5)
    {
        error("Function requires 5 arguments.");
        return 0;
    }

    float r,g,b;

    b = lua_tonumber(L,-1);
    lua_pop(L,1);
    g = lua_tonumber(L,-1);
    lua_pop(L,1);
    r = lua_tonumber(L,-1);
    lua_pop(L,1);

    const char *nodename = lua_tostring(L,-1);
    lua_pop(L,1);
    if(!nodename)
    {
        error("Invalid string parameter passed!");
        int nowArgs = lua_gettop(L);
        lua_pop(L,nowArgs);
        return 0;
    }

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);


    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        dynamic *d = common_lua->dynamics[a];
        if(d)
        {
            if(d->serverID == id)
            {
                d->setNodeColor(nodename,btVector3(r,g,b),common_lua->theServer);
                return 0;
            }
        }
    }

    error("Invalid dynamic object!");
    return 0;
}

static int attachByRope(lua_State *L)
{
    scope("attachByRope");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Function expected 2 arguments!");
        return 0;
    }

    lua_getfield(L, -1, "id");
    int ida = lua_tointeger(L,-1);
    lua_pop(L,2);

    lua_getfield(L, -1, "id");
    int idb = lua_tointeger(L,-1);
    lua_pop(L,2);

    dynamic *a=0,*b=0;

    for(int i = 0; i<common_lua->dynamics.size(); i++)
    {
        if(common_lua->dynamics[i]->serverID == ida)
            a = common_lua->dynamics[i];
        else if(common_lua->dynamics[i]->serverID == idb)
            b = common_lua->dynamics[i];

        if(a && b)
            break;
    }

    if(!a || !b)
    {
        error("Invalid dynamic pass for one of the arguments!");
        return 0;
    }

    a->activate();
    b->activate();

    common_lua->addRope(a,b);
}

static int attachByHinge(lua_State *L)
{
    scope("attachByHinge");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Function expected 2 arguments!");
        return 0;
    }

    lua_getfield(L, -1, "id");
    int ida = lua_tointeger(L,-1);
    lua_pop(L,2);

    lua_getfield(L, -1, "id");
    int idb = lua_tointeger(L,-1);
    lua_pop(L,2);

    dynamic *a=0,*b=0;

    for(int i = 0; i<common_lua->dynamics.size(); i++)
    {
        if(common_lua->dynamics[i]->serverID == ida)
            a = common_lua->dynamics[i];
        else if(common_lua->dynamics[i]->serverID == idb)
            b = common_lua->dynamics[i];

        if(a && b)
            break;
    }

    if(!a || !b)
    {
        error("Invalid dynamic pass for one of the arguments!");
        return 0;
    }

    a->activate();
    b->activate();

    btHingeConstraint *h = new btHingeConstraint(*a,*b,btVector3(0,0,0),btVector3(0,0,0),btVector3(0,1,0),btVector3(0,1,0));
    h->setLimit(-SIMD_PI * 0.25,SIMD_PI * 0.25);
    common_lua->physicsWorld->addConstraint(h);
}

void registerDynamicFunctions(lua_State *L)
{
    //Register metatable for dynamic
    luaL_Reg dynamicRegs[] = {
        {"__tostring",LUAdynamicToString},
        {"__eq",LUAdynamicEqual},
        {"setGravity",dynamicSetGravity},
        {"setVelocity",dynamicSetVelocity},
        {"setAngularFactor",setAngularFactor},
        {"setPosition",setPosition},
        {"setRotation",setRotation},
        {"setRestitution",setRestitution},
        {"setAngularVelocity",setAngularVelocity},
        {"getPosition",getPosition},
        {"remove",removeDynamic},
        {"setShapeName",setShapeName},
        {"setNodeColor",setNodeColor},
        {"attachByRope",attachByRope},
        {"attachByHinge",attachByHinge},
        {NULL,NULL}};
    luaL_newmetatable(L,"dynamic");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"dynamic");

    //Register functions
    lua_register(L,"getDynamicIdx",getDynamicIdx);
    lua_register(L,"getPlayerByName",getDynamicOfClient);
    lua_register(L,"getNumDynamics",getNumDynamics);
    lua_register(L,"addDynamic",addDynamic);
}













