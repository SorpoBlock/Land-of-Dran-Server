#include "emitter.h"

void pushEmitter(lua_State *L,emitter *e)
{
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

static int LUAemitterToString(lua_State *L)
{
    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    std::string ret = "(Emitter " + std::to_string(id) + ")";

    lua_pushstring(L,ret.c_str());
    return 1;
}

btVector4 strToVec4(std::string in)
{
    std::vector<std::string> words;
    split(in,words);
    if(words.size() != 4)
    {
        error("Expected 4 numbers in string, got " + std::to_string(words.size()));
        return btVector4(1,1,1,1);
    }
    else
        return btVector4(atof(words[0].c_str()),
                         atof(words[1].c_str()),
                         atof(words[2].c_str()),
                         atof(words[3].c_str()));

}

btVector3 strToVec3(std::string in)
{
    std::vector<std::string> words;
    split(in,words);
    if(words.size() != 3)
    {
        error("Expected 3 numbers in string, got " + std::to_string(words.size()));
        return btVector3(1,1,1);
    }
    else
        return btVector3(atof(words[0].c_str()),
                         atof(words[1].c_str()),
                         atof(words[2].c_str()));

}

static int addParticleType(lua_State *L)
{
    scope("addParticleType");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Correct syntax: addParticleType(name,table)");
        lua_pop(L,args);
        return 0;
    }

    const char *dbName = lua_tostring(L,-2);
    if(!dbName)
    {
        error("dbName string invalid!");
        return 0;
    }

    bool foundParticleType = false;
    particleType *madeType = 0;

    for(unsigned int a = 0; a<common_lua->particleTypes.size(); a++)
    {
        if(common_lua->particleTypes[a]->dbname == std::string(dbName))
        {
            foundParticleType = true;
            madeType = common_lua->particleTypes[a];
            break;
        }
    }

    if(!madeType)
    {
        madeType = new particleType;
        madeType->dbname = dbName;
    }

    lua_getfield(L, -1, "texture");
    const char *texturePath = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!texturePath)
    {
        error("Texture path string for particle was invalid or not set! A texture is required!");
        lua_pop(L,2);
        return 0;
    }

    madeType->filePath = texturePath;

    for(int a = 0; a<4; a++)
    {
        std::string memberName = "color" + std::to_string(a);
        lua_getfield(L, -1, memberName.c_str());
        const char *colorChar = lua_tostring(L,-1);
        lua_pop(L,1);
        if(colorChar)
            madeType->colors[a] = strToVec4(colorChar);
        else
            madeType->colors[a] = btVector4(1,1,1,1);

        memberName = "size" + std::to_string(a);
        lua_getfield(L, -1, memberName.c_str());
        float size = lua_tonumber(L,-1);
        lua_pop(L,1);
        madeType->sizes[a] = size;

        memberName = "time" + std::to_string(a);
        lua_getfield(L, -1, memberName.c_str());
        float time = lua_tonumber(L,-1);
        lua_pop(L,1);
        madeType->times[a] = time;
    }

    lua_getfield(L, -1, "drag");
    const char *drag = lua_tostring(L,-1);
    if(drag)
        madeType->drag = strToVec3(drag);
    lua_pop(L,1);

    lua_getfield(L, -1, "gravity");
    const char *gravity = lua_tostring(L,-1);
    if(gravity)
        madeType->gravity = strToVec3(gravity);
    lua_pop(L,1);

    lua_getfield(L,-1,"inheritedVelFactor");
    if(!lua_isnil(L,-1))
    {
        float velFactor = lua_tonumber(L,-1);
        madeType->inheritedVelFactor = velFactor;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"lifetimeMS");
    if(!lua_isnil(L,-1))
    {
        float lifetimeMS = lua_tonumber(L,-1);
        madeType->lifetimeMS = lifetimeMS;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"lifetimeVarianceMS");
    if(!lua_isnil(L,-1))
    {
        float lifetimeVarianceMS = lua_tonumber(L,-1);
        madeType->lifetimeVarianceMS = lifetimeVarianceMS;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"spinSpeed");
    if(!lua_isnil(L,-1))
    {
        float spinSpeed = lua_tonumber(L,-1);
        madeType->spinSpeed = spinSpeed;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"useInvAlpha");
    if(!lua_isnil(L,-1))
    {
        bool useInvAlpha = lua_toboolean(L,-1);
        madeType->useInvAlpha = useInvAlpha;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"needsSorting");
    if(!lua_isnil(L,-1))
    {
        bool needsSorting = lua_toboolean(L,-1);
        madeType->needsSorting = needsSorting;
    }
    lua_pop(L,1);

    lua_pop(L,1); //Popping the table itself

    lua_pop(L,1);

    if(!foundParticleType)
    {
        madeType->serverID = common_lua->particleTypes.size();
        common_lua->particleTypes.push_back(madeType);
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
        madeType->sendToClient(common_lua->users[a]->netRef);

    return 0;
}

static int addEmitterType(lua_State *L)
{
    scope("addEmitterType");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("Correct syntax: addEmitterType(name,table)");
        lua_pop(L,args);
        return 0;
    }

    const char *dbName = lua_tostring(L,-2);

    if(!dbName)
    {
        error("dbName string invalid!");
        return 0;
    }

    lua_getfield(L, -1, "particles");
    const char *particleTypes = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!particleTypes)
    {
        error("Particle types string for particle was invalid or not set! At least one particle type is required.");
        lua_pop(L,2);
        return 0;
    }

    std::vector<std::string> particleNames;
    split(particleTypes,particleNames);

    if(particleNames.size() < 1)
    {
        error("Particle types string didn't contain any particle type names!");
        lua_pop(L,2);
        return 0;
    }

    bool foundEmitterType = false;
    emitterType *madeType = 0;

    for(unsigned int a = 0; a<common_lua->emitterTypes.size(); a++)
    {
        if(common_lua->emitterTypes[a]->dbname == std::string(dbName))
        {
            madeType = common_lua->emitterTypes[a];
            foundEmitterType = true;
            break;
        }
    }

    if(!madeType)
        madeType = new emitterType;

    for(unsigned int w = 0; w<particleNames.size(); w++)
    {
        bool foundType = false;
        for(unsigned int a = 0; a<common_lua->particleTypes.size(); a++)
        {
            if(common_lua->particleTypes[a]->dbname == particleNames[w])
            {
                madeType->particleTypes.push_back(common_lua->particleTypes[a]);
                foundType = true;
                break;
            }
        }
        if(!foundType)
            error("Could not find particle type " + particleNames[w]);
    }

    if(madeType->particleTypes.size() < 1)
    {
        error("No valid particle types were found for this emitter!");
        delete madeType;
        return 0;
    }

    lua_getfield(L,-1,"ejectionOffset");
    if(!lua_isnil(L,-1))
    {
        float ejectionOffset = lua_tonumber(L,-1);
        madeType->ejectionOffset = ejectionOffset;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"ejectionPeriodMS");
    if(!lua_isnil(L,-1))
    {
        float ejectionPeriodMS = lua_tonumber(L,-1);
        madeType->ejectionPeriodMS = ejectionPeriodMS;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"ejectionVelocity");
    if(!lua_isnil(L,-1))
    {
        float ejectionVelocity = lua_tonumber(L,-1);
        madeType->ejectionVelocity = ejectionVelocity;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"periodVarianceMS");
    if(!lua_isnil(L,-1))
    {
        float periodVarianceMS = lua_tonumber(L,-1);
        madeType->periodVarianceMS = periodVarianceMS;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"phiReferenceVel");
    if(!lua_isnil(L,-1))
    {
        float phiReferenceVel = lua_tonumber(L,-1);
        madeType->phiReferenceVel = phiReferenceVel;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"phiVariance");
    if(!lua_isnil(L,-1))
    {
        float phiVariance = lua_tonumber(L,-1);
        madeType->phiVariance = phiVariance;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"thetaMax");
    if(!lua_isnil(L,-1))
    {
        float thetaMax = lua_tonumber(L,-1);
        madeType->thetaMax = thetaMax;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"thetaMin");
    if(!lua_isnil(L,-1))
    {
        float thetaMin = lua_tonumber(L,-1);
        madeType->thetaMin = thetaMin;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"velocityVariance");
    if(!lua_isnil(L,-1))
    {
        float velocityVariance = lua_tonumber(L,-1);
        madeType->velocityVariance = velocityVariance;
    }
    lua_pop(L,1);

    lua_getfield(L,-1,"lifetimeMS");
    if(!lua_isnil(L,-1))
    {
        float lifetimeMS = lua_tonumber(L,-1);
        madeType->lifetimeMS = lifetimeMS;
    }
    lua_pop(L,1);

    lua_getfield(L, -1, "uiName");
    const char *uiName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(uiName)
        madeType->uiName = uiName;

    lua_pop(L,1); //popping the table itself

    //const char *dbName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!foundEmitterType)
    {
        madeType->dbname = dbName;
        madeType->serverID = common_lua->emitterTypes.size();
        common_lua->emitterTypes.push_back(madeType);
    }

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
        madeType->sendToClient(common_lua->users[a]->netRef);

    return 0;
}

static int addEmitter(lua_State *L)
{
    scope("addEmitter");

    int args = lua_gettop(L);

    if(args == 1)
    {
        const char *dbName = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!dbName)
        {
            error("Invalid emitter type name string provided.");
            lua_pushnil(L);
            return 1;
        }

        emitterType *theType = 0;
        for(unsigned int a = 0; a<common_lua->emitterTypes.size(); a++)
        {
            if(common_lua->emitterTypes[a]->dbname == dbName)
            {
                theType = common_lua->emitterTypes[a];
                break;
            }
        }

        if(!theType)
        {
            error("Could not find emitterType named " + std::string(dbName));
            lua_pushnil(L);
            return 1;
        }

        emitter *tmp = common_lua->addEmitter(theType);

        pushEmitter(L,tmp);
        return 1;
    }
    else if(args == 4)
    {
        float z = lua_tonumber(L,-1);
        lua_pop(L,1);
        float y = lua_tonumber(L,-1);
        lua_pop(L,1);
        float x = lua_tonumber(L,-1);
        lua_pop(L,1);

        const char *dbName = lua_tostring(L,-1);
        lua_pop(L,1);

        if(!dbName)
        {
            error("Invalid emitter type name string provided.");
            lua_pushnil(L);
            return 1;
        }

        emitterType *theType = 0;
        for(unsigned int a = 0; a<common_lua->emitterTypes.size(); a++)
        {
            if(common_lua->emitterTypes[a]->dbname == dbName)
            {
                theType = common_lua->emitterTypes[a];
                break;
            }
        }

        if(!theType)
        {
            error("Could not find emitterType named " + std::string(dbName));
            lua_pushnil(L);
            return 1;
        }

        emitter *tmp = common_lua->addEmitter(theType,x,y,z);

        pushEmitter(L,tmp);
        return 1;
    }
    else
    {
        error("This function requires either 1 argument or 4, provided: " + std::to_string(args));
        lua_pop(L,args);
    }

    lua_pushnil(L);
    return 1;
}

static int attachToBrick(lua_State *L)
{
    scope("attachToBrick");

    lua_getfield(L, -1, "pointer");
    brick *passedBrick = (brick*)lua_touserdata(L,-1);
    lua_pop(L,2);

    if(!passedBrick)
    {
        error("Invalid brick passed!");
        return 0;
    }

    emitter *e = popEmitter(L);

    if(!e)
    {
        error("Invalid emitter passed!");
        return 0;
    }

    e->attachedToModel = 0;
    e->attachedToBrick = passedBrick;

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
        e->sendToClient(common_lua->users[a]->netRef);
}

static int attachToDynamic(lua_State *L)
{
    scope("attachToDyanmic");

    int args = lua_gettop(L);

    if(args < 2 || args > 3)
    {
        error("Function requires 2 or 3 arguments, passed: " + std::to_string(args));
        return 0;
    }

    std::string nodeName = "";
    if(args == 3)
    {
        const char *nameBuffer = lua_tostring(L,-1);
        lua_pop(L,1);
        if(nameBuffer)
            nodeName = nameBuffer;
        else
            error("Invalid string argument passed!");
    }

    lua_getfield(L, -1, "id");
    int id = lua_tointeger(L,-1);
    lua_pop(L,2);

    dynamic *passedDynamic = 0;

    for(unsigned int a = 0; a<common_lua->dynamics.size(); a++)
    {
        if(common_lua->dynamics[a]->serverID == id)
        {
            passedDynamic = common_lua->dynamics[a];
            break;
        }
    }

    if(!passedDynamic)
    {
        error("Invalid dynamic passed!");
        return 0;
    }

    emitter *e = popEmitter(L);

    if(!e)
    {
        error("Invalid emitter passed!");
        return 0;
    }

    e->attachedToBrick = 0;
    e->attachedToModel = passedDynamic;
    e->nodeName = nodeName;

    for(unsigned int a = 0; a<common_lua->users.size(); a++)
        e->sendToClient(common_lua->users[a]->netRef);
}

static int getEmitterTable(lua_State *L)
{
    scope("getEmitterTable");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Function requires 1 argument, was given: " + std::to_string(args));
        lua_pushnil(L);
        return 1;
    }

    const char *emitterName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!emitterName)
    {
        error("Invalid string parameter passed!");
        lua_pushnil(L);
        return 1;
    }

    emitterType *type = 0;
    for(unsigned int a = 0; a<common_lua->emitterTypes.size(); a++)
    {
        if(common_lua->emitterTypes[a]->dbname == std::string(emitterName))
        {
            type = common_lua->emitterTypes[a];
            break;
        }
    }

    if(!type)
    {
        error("Could not find emitter type named " + std::string(emitterName));
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);

    std::string particles = "";
    for(int a = 0; a<type->particleTypes.size(); a++)
        particles = type->particleTypes[a]->dbname + " ";

    lua_pushstring(L,particles.c_str());
    lua_setfield(L,-2,"particles");

    lua_pushnumber(L,type->ejectionOffset);
    lua_setfield(L,-2,"ejectionOffset");

    lua_pushnumber(L,type->ejectionPeriodMS);
    lua_setfield(L,-2,"ejectionPeriodMS");

    lua_pushnumber(L,type->ejectionVelocity);
    lua_setfield(L,-2,"ejectionVelocity");

    lua_pushnumber(L,type->periodVarianceMS);
    lua_setfield(L,-2,"periodVarianceMS");

    lua_pushnumber(L,type->phiReferenceVel);
    lua_setfield(L,-2,"phiReferenceVel");

    lua_pushnumber(L,type->phiVariance);
    lua_setfield(L,-2,"phiVariance");

    lua_pushnumber(L,type->thetaMax);
    lua_setfield(L,-2,"thetaMax");

    lua_pushnumber(L,type->thetaMin);
    lua_setfield(L,-2,"thetaMin");

    lua_pushnumber(L,type->velocityVariance);
    lua_setfield(L,-2,"velocityVariance");

    lua_pushstring(L,type->uiName.c_str());
    lua_setfield(L,-2,"uiName");

    return 1;
}

std::string vec4ToString(btVector4 in)
{
    return std::to_string(in.x()) + " " + std::to_string(in.y()) + " " + std::to_string(in.z()) + " " + std::to_string(in.w());
}

std::string vec3ToString(btVector3 in)
{
    return std::to_string(in.x()) + " " + std::to_string(in.y()) + " " + std::to_string(in.z());
}

static int getParticleTable(lua_State *L)
{
    scope("getParticleTable");

    int args = lua_gettop(L);
    if(args != 1)
    {
        error("Function requires 1 argument, was given: " + std::to_string(args));
        lua_pushnil(L);
        return 1;
    }

    const char *particleName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!particleName)
    {
        error("Invalid string parameter passed!");
        lua_pushnil(L);
        return 1;
    }

    particleType *type = 0;
    for(unsigned int a = 0; a<common_lua->particleTypes.size(); a++)
    {
        if(common_lua->particleTypes[a]->dbname == std::string(particleName))
        {
            type = common_lua->particleTypes[a];
            break;
        }
    }

    if(!type)
    {
        error("Could not find particle type named " + std::string(particleName));
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);

    lua_pushstring(L,type->filePath.c_str());
    lua_setfield(L,-2,"texture");

    for(int a = 0; a<4; a++)
    {
        std::string color = vec4ToString(type->colors[a]);
        lua_pushstring(L,color.c_str());
        lua_setfield(L,-2,("color" + std::to_string(a)).c_str());

        lua_pushnumber(L,type->times[a]);
        lua_setfield(L,-2,("time" + std::to_string(a)).c_str());

        lua_pushnumber(L,type->sizes[a]);
        lua_setfield(L,-2,("size" + std::to_string(a)).c_str());
    }

    std::string drag = vec3ToString(type->drag);
    lua_pushstring(L,drag.c_str());
    lua_setfield(L,-2,"drag");

    std::string gravity = vec3ToString(type->gravity);
    lua_pushstring(L,gravity.c_str());
    lua_setfield(L,-2,"gravity");

    lua_pushnumber(L,type->inheritedVelFactor);
    lua_setfield(L,-2,"inheritedVelFactor");

    lua_pushnumber(L,type->lifetimeMS);
    lua_setfield(L,-2,"lifetimeMS");

    lua_pushnumber(L,type->lifetimeVarianceMS);
    lua_setfield(L,-2,"lifetimeVarianceMS");

    lua_pushnumber(L,type->spinSpeed);
    lua_setfield(L,-2,"spinSpeed");

    lua_pushboolean(L,type->useInvAlpha);
    lua_setfield(L,-2,"useInvAlpha");

    lua_pushboolean(L,type->needsSorting);
    lua_setfield(L,-2,"needsSorting");

    return 1;
}

static int removeEmitter(lua_State *L)
{
    scope("removeEmitter");

    emitter *e = popEmitter(L);

    if(!e)
        return 0;

    common_lua->removeEmitter(e);

    return 0;
}

void registerEmitterFunctions(lua_State *L)
{
    luaL_Reg emitterRegs[] = {
        {"__tostring",LUAemitterToString},
        {"attachToDynamic",attachToDynamic},
        {"attachToBrick",attachToBrick},
        {"remove",removeEmitter},
        {NULL,NULL}};
    luaL_newmetatable(L,"emitterMETATABLE");
    luaL_setfuncs(L,emitterRegs,0);
    lua_pushvalue(L,-1);
    lua_setfield(L,-1,"__index");
    lua_setglobal(L,"emitterMETATABLE");

    lua_register(L,"addParticleType",addParticleType);
    lua_register(L,"addEmitterType",addEmitterType);
    lua_register(L,"addEmitter",addEmitter);
    lua_register(L,"getEmitterTable",getEmitterTable);
    lua_register(L,"getParticleTable",getParticleTable);
}





















