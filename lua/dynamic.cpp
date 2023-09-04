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

    dynamic *a = popDynamic(L);
    dynamic *b = popDynamic(L);

    lua_pushboolean(L,a == b);
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

    pushDynamic(L,common_lua->dynamics[idx]);

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
                    pushDynamic(L,common_lua->users[a]->controlling);
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
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    if(x > 100 || y > 100 || z > 100 || x < -100 || y < -100 || z < -100)
    {
        error("dynamicSetGravity bad args - x,y,z must be between -100 and 100");
        return 0;
    }

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->setGravity(btVector3(x,y,z));
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

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->setAngularFactor(btVector3(x,y,z));
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

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->activate();
    object->setLinearVelocity(btVector3(x,y,z));

    if(object->isPlayer)
    {
        for(int b = 0; b<common_lua->users.size(); b++)
            if(common_lua->users[b]->controlling == object)
                common_lua->users[b]->forceTransformUpdate();
    }

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

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    btTransform t = object->getWorldTransform();
    t.setOrigin(btVector3(x,y,z));
    object->setWorldTransform(t);

    if(object->isPlayer)
    {
        for(int b = 0; b<common_lua->users.size(); b++)
            if(common_lua->users[b]->controlling == object)
                common_lua->users[b]->forceTransformUpdate();
    }

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

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->setAngularVelocity(btVector3(x,y,z));
    return 0;
}

static int setRotation(lua_State *L)
{
    scope("setRotation");

    int args = lua_gettop(L);

    if(args == 4)
    {
        float roll = lua_tonumber(L,-1);
        lua_pop(L,1);
        float pitch = lua_tonumber(L,-1);
        lua_pop(L,1);
        float yaw = lua_tonumber(L,-1);
        lua_pop(L,1);

        dynamic *object = popDynamic(L);

        if(!object)
            return 0;

        btTransform t = object->getWorldTransform();
        t.setRotation(btQuaternion(yaw,pitch,roll));
        object->setWorldTransform(t);

        btVector3 lookDir = t * btVector3(0,0,-1);
        object->lastCamX = lookDir.x();
        object->lastCamZ = lookDir.z();

        if(object->isPlayer)
        {
            for(int b = 0; b<common_lua->users.size(); b++)
                if(common_lua->users[b]->controlling == object)
                    common_lua->users[b]->forceTransformUpdate();
        }

        return 0;
    }
    else if(args != 5)
    {
        error("Function takes 4 or 5 arguments!");
        return 0;
    }

    float w = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float z = lua_tonumber(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    btTransform t = object->getWorldTransform();
    t.setRotation(btQuaternion(x,y,z,w));
    object->setWorldTransform(t);

    btVector3 lookDir = t * btVector3(0,0,-1);
    object->lastCamX = lookDir.x();
    object->lastCamZ = lookDir.z();

    if(object->isPlayer)
    {
        for(int b = 0; b<common_lua->users.size(); b++)
            if(common_lua->users[b]->controlling == object)
                common_lua->users[b]->forceTransformUpdate();
    }
    return 0;
}

static int setRestitution(lua_State *L)
{
    scope("setRestitution");
    float rest = lua_tonumber(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->setRestitution(rest);
    return 0;
}

static int applyDamage(lua_State *L)
{
    scope("applyDamage");

    int args = lua_gettop(L);

    if(args < 2 || args > 3)
    {
        error("This function takes 2 or 3 arguments!");
        return 0;
    }

    std::string cause = "";
    if(args == 3)
    {
        const char *causeBuf = lua_tostring(L,-1);
        lua_pop(L,1);
        if(!causeBuf)
        {
            error("Invalid cause string!");
            return 0;
        }
        cause = std::string(causeBuf);
    }

    float dam = lua_tonumber(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    common_lua->applyDamage(object,dam,cause);

    return 0;
}

static int setCanTakeDamage(lua_State *L)
{
    scope("setCanTakeDamage");
    bool canTake = lua_toboolean(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->canTakeDamage = canTake;

    return 0;
}

static int getPosition(lua_State *L)
{
    scope("getDynamicPositionLua");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    btVector3 pos = object->getWorldTransform().getOrigin();
    lua_pushnumber(L,pos.x());
    lua_pushnumber(L,pos.y());
    lua_pushnumber(L,pos.z());
    return 3;
}

static int getVelocity(lua_State *L)
{
    scope("getDynamicVelocityLua");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    btVector3 vel = object->getLinearVelocity();
    lua_pushnumber(L,vel.x());
    lua_pushnumber(L,vel.y());
    lua_pushnumber(L,vel.z());
    return 3;
}

static int getCamDir(lua_State *L)
{
    scope("getCamDir");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    lua_pushnumber(L,object->lastCamX);
    lua_pushnumber(L,object->lastCamY);
    lua_pushnumber(L,object->lastCamZ);
    return 3;
}

static int getAngularVelocity(lua_State *L)
{
    scope("getDynamicAngularVelocityLua");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    btVector3 vel = object->getAngularVelocity();
    lua_pushnumber(L,vel.x());
    lua_pushnumber(L,vel.y());
    lua_pushnumber(L,vel.z());
    return 3;
}

static int getRotation(lua_State *L)
{
    scope("getDynamicRotation");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 4;
    }

    btQuaternion quat = object->getWorldTransform().getRotation();
    lua_pushnumber(L,quat.w());
    lua_pushnumber(L,quat.x());
    lua_pushnumber(L,quat.y());
    lua_pushnumber(L,quat.z());
    return 4;
}

static int getNumDynamics(lua_State *L)
{
    lua_pushnumber(L,common_lua->dynamics.size());
    return 1;
}

static int removeDynamic(lua_State *L)
{
    scope("removeDynamicLua");

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    common_lua->removeDynamic(object);
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

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    if(args == 2)
    {
        r = object->shapeNameR;
        g = object->shapeNameG;
        b = object->shapeNameB;
    }

    common_lua->setShapeName(object,name,r,g,b);

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

    pushDynamic(L,tmp);
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

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->setNodeColor(nodename,btVector3(r,g,b),common_lua->theServer);
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

    dynamic *a = popDynamic(L);
    dynamic *b = popDynamic(L);

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

    dynamic *a = popDynamic(L);
    dynamic *b = popDynamic(L);

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

static int getHeldTool(lua_State *L)
{
    scope("getHeldTool");

    int args = lua_gettop(L);
    if(args != 1)
    {
        lua_pop(L,args);
        error("Function expected 1 argument!");
        lua_pushnil(L);
        return 1;
    }

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushnil(L);
        return 1;
    }

    if(object->lastHeldSlot == -1 || object->lastHeldSlot < 0 || object->lastHeldSlot >= inventorySize)
        lua_pushnil(L);
    else
    {
        if(!object->holding[object->lastHeldSlot])
            lua_pushnil(L);
        else
            pushItem(L,object->holding[object->lastHeldSlot]);
            //lua_pushstring(L,object->holding[object->lastHeldSlot]->heldItemType->uiName.c_str());
    }
    return 1;
}

static int addProjectile(lua_State *L)
{
    scope("addProjectile");

    //dyanmicTypeID,x,y,z,velX,velY,velZ,tag
    int args = lua_gettop(L);

    if(args < 7 || args > 8)
    {
        error("Function requires 7 or 8 arguments!");
        lua_pushnil(L);
        return 1;
    }

    std::string tag = "";

    if(args == 8)
    {
        const char *tagBuf = lua_tostring(L,-1);
        lua_pop(L,1);
        if(!tagBuf)
        {
            error("Invalid tag string argument!");
            lua_pushnil(L);
            return 1;
        }
        tag = std::string(tagBuf);
    }

    float velZ = lua_tonumber(L,-1);
    lua_pop(L,1);
    float velY = lua_tonumber(L,-1);
    lua_pop(L,1);
    float velX = lua_tonumber(L,-1);
    lua_pop(L,1);

    float z = lua_tonumber(L,-1);
    lua_pop(L,1);
    float y = lua_tonumber(L,-1);
    lua_pop(L,1);
    float x = lua_tonumber(L,-1);
    lua_pop(L,1);

    int typeID = lua_tointeger(L,-1);
    lua_pop(L,1);

    if(typeID < 0 || typeID >= common_lua->dynamicTypes.size())
    {
        error("Type ID out of range!");
        lua_pushnil(L);
        return 1;
    }

    dynamic *projectile = common_lua->addDynamic(typeID,0,0,0);
    projectile->isProjectile = true;
    projectile->setLinearVelocity(btVector3(velX,velY,velZ));
    btTransform t = btTransform::getIdentity();
    t.setOrigin(btVector3(x,y,z));
    projectile->setWorldTransform(t);
    projectile->setActivationState(DISABLE_DEACTIVATION);
    projectile->projectileTag = tag;

    common_lua->projectiles.push_back(projectile);

    pushDynamic(L,projectile);
    return 1;
}

static int giveItem(lua_State *L)
{
    scope("giveItem");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Function expects 2 arguments!");
        lua_pushboolean(L,false);
        return 1;
    }

    item *i = popItem(L);

    if(!i)
    {
        error("Invalid item!");
        lua_pushboolean(L,false);
        return 1;
    }

    dynamic *dyn = popDynamic(L);

    if(!dyn)
    {
        error("Invalid dynamic!");
        lua_pushboolean(L,false);
        return 1;
    }

    int freeSlot = -1;
    for(int b = 0; b<inventorySize; b++)
    {
        if(!dyn->holding[b])
        {
            freeSlot = b;
            break;
        }
    }

    if(freeSlot == -1)
    {
        lua_pushboolean(L,false);
        return 1;
    }

    clientData *source = 0;
    for(int b = 0; b<common_lua->users.size(); b++)
    {
        if(!common_lua->users[b])
            continue;
        if(!common_lua->users[b]->controlling)
            continue;

        if(common_lua->users[b]->controlling == dyn)
        {
            source = common_lua->users[b];
            break;
        }
    }

    common_lua->pickUpItem(dyn,i,freeSlot,source);

    lua_pushboolean(L,true);
    return 1;
}

static int isProjectile(lua_State *L)
{
    scope("isProjectile");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushboolean(L,false);
        return 1;
    }

    lua_pushboolean(L,object->isProjectile);
    return 1;
}

static int isPlayer(lua_State *L)
{
    scope("isPlayer");

    dynamic *object = popDynamic(L);

    if(!object)
    {
        lua_pushboolean(L,false);
        return 1;
    }

    lua_pushboolean(L,object->isPlayer);
    return 1;
}

static int getProjectileIdx(lua_State *L)
{
    int idx = lua_tointeger(L,1);
    lua_pop(L,1);

    if(idx < 0 || (int)idx >= (int)common_lua->projectiles.size())
    {
        error("getProjectileIdx invalid index " + std::to_string(idx));
        lua_pushnil(L);
        return 1;
    }

    pushDynamic(L,common_lua->projectiles[idx]);

    return 1;
}

static int getNumProjectiles(lua_State *L)
{
    lua_pushnumber(L,common_lua->projectiles.size());
    return 1;
}

static int switchHeldSlot(lua_State *L)
{
    scope("switchHeldSlot");
    int slot = lua_tointeger(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    if(slot >= inventorySize)
        slot = -1;
    if(slot < -1)
        slot = -1;

    object->lastHeldSlot = slot;

    for(unsigned int a = 0; a<inventorySize; a++)
    {
        if(object->holding[a])
        {
            object->holding[a]->setHidden(true);
            object->holding[a]->setSwinging(false);
        }
    }

    if(slot != -1)
    {
        if(object->holding[slot])
            object->holding[slot]->setHidden(false);
    }

    return 0;
}

static int setWalking(lua_State *L)
{
    scope("setWalking");
    bool walking = lua_toboolean(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->isPlayer = true;
    object->walking = walking;

    return 0;
}

static int setHeadPitch(lua_State *L)
{
    scope("setHeadPitch");
    float headPitch = lua_tonumber(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    if(headPitch < -1)
        headPitch = -1;
    if(headPitch > 1)
        headPitch = 1;

    object->isPlayer = true;
    object->lastCamY = headPitch;

    return 0;
}

static int fireHeldGun(lua_State *L)
{
    scope("fireHeldGun");

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    if(object->lastHeldSlot == -1)
        return 0;

    btVector3 firePos = object->getWorldTransform().getOrigin() + btVector3(object->type->eyeOffsetX,object->type->eyeOffsetY,object->type->eyeOffsetZ);
    weaponFire(object,object->holding[object->lastHeldSlot],firePos.x(),firePos.y(),firePos.z(),object->lastCamX,object->lastCamY,object->lastCamZ);

    return 0;
}

static int setHealth(lua_State *L)
{
    scope("setHealth");
    float health = lua_tonumber(L,-1);
    lua_pop(L,1);

    dynamic *object = popDynamic(L);

    if(!object)
        return 0;

    object->health = health;

    //Keep death code in one place with this one weird trick

    if(object->health <= 0)
        common_lua->applyDamage(object,1);

    return 0;
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
        {"getVelocity",getVelocity},
        {"getAngularVelocity",getAngularVelocity},
        {"getRotation",getRotation},
        {"getHeldTool",getHeldTool},
        {"giveItem",giveItem},
        {"isProjectile",isProjectile},
        {"isPlayer",isPlayer},
        {"applyDamage",applyDamage},
        {"setCanTakeDamage",setCanTakeDamage},
        {"switchHeldSlot",switchHeldSlot},
        {"setWalking",setWalking},
        {"setHeadPitch",setHeadPitch},
        {"fireHeldGun",fireHeldGun},
        {"getCamDir",getCamDir},
        {"setHealth",setHealth},
        {NULL,NULL}};
    luaL_newmetatable(L,"dynamic");
    luaL_setfuncs(L,dynamicRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"dynamic");

    //Register functions
    lua_register(L,"getDynamicIdx",getDynamicIdx);
    lua_register(L,"getProjectileIdx",getProjectileIdx);
    lua_register(L,"getPlayerByName",getDynamicOfClient);
    lua_register(L,"getNumDynamics",getNumDynamics);
    lua_register(L,"addDynamic",addDynamic);
    lua_register(L,"addProjectile",addProjectile);
    lua_register(L,"getNumProjectiles",getNumProjectiles);
}













