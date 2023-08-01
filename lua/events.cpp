#include "events.h"

void registerEvent(std::string eventName,std::string functionName)
{
    scope("registerEvent");

    if(functionName.length() < 1)
    {
        error("Invalid function name!");
        return;
    }

    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == eventName)
        {
            common_lua->events[a].functionNames.push_back(functionName);
            return;
        }
    }

    error("Unable to find event " + eventName);
}

static int registerEventListener(lua_State *L)
{
    scope("registerEventListener");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("requires 2 arguments " + std::to_string(args) + " provided!");
        lua_pop(L,args);
        return 0;
    }

    const char *funcName = lua_tostring(L,-1);
    lua_pop(L,1);
    const char *eventName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!funcName)
    {
        error("funcName invalid");
        return 0;
    }

    if(!eventName)
    {
        error("eventName invalid!");
        return 0;
    }

    registerEvent(eventName,funcName);

    return 0;
}

static int unregisterEventListener(lua_State *L)
{
    scope("unregisterEventListener");

    int args = lua_gettop(L);
    if(args != 2)
    {
        error("requires 2 arguments " + std::to_string(args) + " provided!");
        lua_pop(L,args);
        return 0;
    }

    const char *funcName = lua_tostring(L,-1);
    lua_pop(L,1);
    const char *eventName = lua_tostring(L,-1);
    lua_pop(L,1);

    if(!funcName)
    {
        error("funcName invalid");
        return 0;
    }

    if(!eventName)
    {
        error("eventName invalid!");
        return 0;
    }

    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == eventName)
        {
            for(unsigned int b = 0; b<common_lua->events[a].functionNames.size(); b++)
            {
                if(common_lua->events[a].functionNames[b] == funcName)
                {
                    common_lua->events[a].functionNames.erase(common_lua->events[a].functionNames.begin() + b);
                    return 0;
                }
            }

            error("Function " + std::string(funcName) + " was not registered to " + std::string(eventName));

            return 0;
        }
    }

    error("Could not find event " + std::string(eventName));

    return 0;
}

void addEventNames(lua_State *L)
{
    common_lua->events.push_back(eventListener("spawnPlayer"));
    common_lua->events.push_back(eventListener("clientClickBrick"));
    common_lua->events.push_back(eventListener("clientPlantBrick"));
    common_lua->events.push_back(eventListener("clientLeave"));
    common_lua->events.push_back(eventListener("clientTryLoadCar"));
    common_lua->events.push_back(eventListener("clientChat"));
    common_lua->events.push_back(eventListener("clientAdminOrb"));

    lua_register(L,"registerEventListener",registerEventListener);
    lua_register(L,"unregisterEventListener",unregisterEventListener);
}

bool clientAdminOrb(clientData* source,bool startOrStop,float &x,float &y,float &z)
{

    bool cancelled = false;

    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientAdminOrb")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientAdminOrb event dissapeared!");
        return false;
    }

    int top = lua_gettop(common_lua->luaState);

    if(top > 0)
        lua_pop(common_lua->luaState,top);

    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,source->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            lua_pushboolean(common_lua->luaState,startOrStop);

            lua_pushboolean(common_lua->luaState,cancelled);

            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);

            if(lua_pcall(common_lua->luaState,6,4,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 4)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return 4 arguments, only returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
            else
            {
                z = lua_tonumber(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
                y = lua_tonumber(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
                x = lua_tonumber(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
                cancelled = lua_toboolean(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
            }
        }
    }

    return cancelled;
}

bool clientChat(clientData *source,std::string &message)
{
    bool cancelled = false;

    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientChat")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientChat event dissapeared!");
        return false;
    }

    int top = lua_gettop(common_lua->luaState);

    if(top > 0)
        lua_pop(common_lua->luaState,top);

    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,source->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            lua_pushstring(common_lua->luaState,message.c_str());

            if(lua_pcall(common_lua->luaState,2,2,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 2)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return 2 arguments, only returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
            else
            {
                cancelled = lua_toboolean(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);

                const char *msgbuf = lua_tostring(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);

                if(msgbuf)
                    message = msgbuf;
                else
                    error("Could not resolve string return value for clientChat event.");
            }
        }
    }

    return cancelled;
}

bool clientTryLoadCarEvent(clientData *source,brickCar *theCar,bool loadCarAsCar)
{
    bool cancelled = false;

    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientTryLoadCar")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientTryLoadCar event dissapeared!");
        return false;
    }

    int top = lua_gettop(common_lua->luaState);

    if(top > 0)
        lua_pop(common_lua->luaState,top);

    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,source->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            luaPushBrickCar(common_lua->luaState,theCar);

            lua_pushboolean(common_lua->luaState,loadCarAsCar);

            if(lua_pcall(common_lua->luaState,3,1,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 1)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return 1 argument, only returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
            else
            {
                cancelled = lua_toboolean(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
            }
        }
    }

    return cancelled;
}

void clientLeaveEvent(clientData *source)
{
    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientLeave")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientLeave event dissapeared!");
        return;
    }

    int top = lua_gettop(common_lua->luaState);


    if(top > 0)
        lua_pop(common_lua->luaState,top);

    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,source->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            if(lua_pcall(common_lua->luaState,1,0,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 0)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return no arguments, returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
        }
    }
}

void clientPlantBrickEvent(clientData *source,brick *theBrick)
{
    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientPlantBrick")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientPlantBrick event dissapeared!");
        return;
    }

    int top = lua_gettop(common_lua->luaState);


    if(top > 0)
        lua_pop(common_lua->luaState,top);

    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,source->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"brickMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,theBrick->serverID);
            lua_setfield(common_lua->luaState,-2,"id");
            lua_pushlightuserdata(common_lua->luaState,theBrick);
            lua_setfield(common_lua->luaState,-2,"pointer");
            lua_pushstring(common_lua->luaState,"brick");
            lua_setfield(common_lua->luaState,-2,"type");

            if(lua_pcall(common_lua->luaState,2,0,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 0)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return no arguments, returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
        }
    }
}

void clientClickBrickEvent(clientData *source,brick *theBrick,float x,float y,float z)
{
    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientClickBrick")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientClickBrick event dissapeared!");
        return;
    }

    int top = lua_gettop(common_lua->luaState);
    if(top > 0)
        lua_pop(common_lua->luaState,top);


    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,source->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"brickMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,theBrick->serverID);
            lua_setfield(common_lua->luaState,-2,"id");
            lua_pushlightuserdata(common_lua->luaState,theBrick);
            lua_setfield(common_lua->luaState,-2,"pointer");
            lua_pushstring(common_lua->luaState,"brick");
            lua_setfield(common_lua->luaState,-2,"type");

            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);

            if(lua_pcall(common_lua->luaState,5,0,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 0)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return no arguments, returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
        }
    }
}

bool spawnPlayerEvent(clientData *client,float &x,float &y,float &z,bool cancelled)
{
    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "spawnPlayer")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("spawnPlayer event dissapeared!");
        return false;
    }

    int top = lua_gettop(common_lua->luaState);
    if(top > 0)
        lua_pop(common_lua->luaState,top);

    for(unsigned int a = 0; a<event->functionNames.size(); a++)
    {
        lua_getglobal(common_lua->luaState,event->functionNames[a].c_str());
        if(!lua_isfunction(common_lua->luaState,1))
        {
            error("Function " + event->functionNames[a] + " was assigned to " + event->eventName + " but doesn't appear to be a valid function!");
            lua_pop(common_lua->luaState,1);
        }
        else
        {
            lua_newtable(common_lua->luaState);
            lua_getglobal(common_lua->luaState,"clientMETATABLE");
            lua_setmetatable(common_lua->luaState,-2);
            lua_pushinteger(common_lua->luaState,client->playerID);
            lua_setfield(common_lua->luaState,-2,"id");

            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);

            lua_pushboolean(common_lua->luaState,cancelled);

            if(lua_pcall(common_lua->luaState,5,5,0))
            {
                error("Error in lua call to event listener function " + event->functionNames[a] + " for event " + event->eventName);
                if(lua_gettop(common_lua->luaState) > 0)
                {
                    const char *err = lua_tostring(common_lua->luaState,-1);
                    if(!err)
                        continue;
                    std::string errorstr = err;

                    int args = lua_gettop(common_lua->luaState);
                    if(args > 0)
                        lua_pop(common_lua->luaState,args);

                    replaceAll(errorstr,"[","\\[");

                    error("[colour='FFFF0000']" + errorstr);
                }
            }

            int rets = lua_gettop(common_lua->luaState);
            if(rets != 5)
            {
                error(event->eventName + " listener " + event->functionNames[a] + " was meant to return 5 arguments, only returned " + std::to_string(rets) + ". Will ignore return values!");
                lua_pop(common_lua->luaState,rets);
            }
            else
            {
                cancelled = lua_toboolean(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
                z = lua_tonumber(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
                y = lua_tonumber(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,1);
                x = lua_tonumber(common_lua->luaState,-1);
                lua_pop(common_lua->luaState,2);
            }
        }
    }

    return cancelled;
}

void unifiedWorld::spawnPlayer(clientData *source,float x,float y,float z)
{
    bool cancelled = spawnPlayerEvent(source,x,y,z);

    if(!cancelled)
    {
        if(source->controlling)
        {
            removeDynamic(source->controlling);
            source->setControlling(0);
        }

        dynamic *theirPlayer = addDynamic(0,source->preferredRed,source->preferredGreen,source->preferredBlue);

        btTransform t;
        t.setIdentity();
        t.setOrigin(btVector3(x,y,z));
        theirPlayer->setWorldTransform(t);
        theirPlayer->makePlayer();

        setShapeName(theirPlayer,source->name,source->preferredRed,source->preferredGreen,source->preferredBlue);

        //source->controlling = theirPlayer;
        source->setControlling(theirPlayer);
        source->cameraBoundToDynamic = true;
        source->cameraTarget = theirPlayer;

        source->sendCameraDetails();

        item *wrench = common_lua->addItem(common_lua->getItemType("Wrench"));
        common_lua->pickUpItem(source->controlling,wrench,0,source);
        item *hammer = common_lua->addItem(common_lua->getItemType("Hammer"));
        common_lua->pickUpItem(source->controlling,hammer,1,source);

        theirPlayer->setLinearVelocity(btVector3(0,0,0));
        theirPlayer->setAngularVelocity(btVector3(0,0,0));

        common_lua->applyAppearancePrefs(source,theirPlayer);
    }
}








