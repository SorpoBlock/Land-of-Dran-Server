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
    common_lua->events.push_back(eventListener("clientJoin"));
    common_lua->events.push_back(eventListener("projectileHit"));
    common_lua->events.push_back(eventListener("weaponFire"));
    common_lua->events.push_back(eventListener("dynamicDeath"));
    common_lua->events.push_back(eventListener("raycastHit"));

    lua_register(L,"registerEventListener",registerEventListener);
    lua_register(L,"unregisterEventListener",unregisterEventListener);
}

void clientJoin(clientData *source)
{
    //Maybe I should just have the event as a global variable...
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "clientJoin")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("clientJoin event dissapeared!");
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
            pushClient(common_lua->luaState,source);

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
            pushClient(common_lua->luaState,source);

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
            pushClient(common_lua->luaState,source);

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
            pushClient(common_lua->luaState,source);

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
            pushClient(common_lua->luaState,source);

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
            pushClient(common_lua->luaState,source);

            pushBrick(common_lua->luaState,theBrick);

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

void clientClickBrickEvent(clientData *source,brick *theBrick,float x,float y,float z,bool isLeft)
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
            pushClient(common_lua->luaState,source);

            pushBrick(common_lua->luaState,theBrick);

            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);

            lua_pushboolean(common_lua->luaState,isLeft);

            if(lua_pcall(common_lua->luaState,6,0,0))
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
            pushClient(common_lua->luaState,client);

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

        if(common_lua->getItemType("Wrench"))
        {
            item *wrench = common_lua->addItem(common_lua->getItemType("Wrench"));
            common_lua->pickUpItem(source->controlling,wrench,0,source);
        }

        if(common_lua->getItemType("Hammer"))
        {
            item *hammer = common_lua->addItem(common_lua->getItemType("Hammer"));
            common_lua->pickUpItem(source->controlling,hammer,1,source);
        }
        //item *gun = common_lua->addItem(common_lua->getItemType("Gun"));
        //common_lua->pickUpItem(source->controlling,gun,2,source);

        theirPlayer->setLinearVelocity(btVector3(0,0,0));
        theirPlayer->setAngularVelocity(btVector3(0,0,0));

        common_lua->applyAppearancePrefs(source,theirPlayer);
    }
}

void projectileHit(brick *hitBrick,float x,float y,float z,std::string tag)
{
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "projectileHit")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("projectileHit event dissapeared!");
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
            pushBrick(common_lua->luaState,hitBrick);
            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);
            lua_pushstring(common_lua->luaState,tag.c_str());

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

void projectileHit(dynamic *hitDynamic,float x,float y,float z,std::string tag)
{
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "projectileHit")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("projectileHit event dissapeared!");
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
            pushDynamic(common_lua->luaState,hitDynamic);
            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);
            lua_pushstring(common_lua->luaState,tag.c_str());

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

void raycastHit(dynamic *player,item *gun,dynamic *hit,float x,float y,float z)
{
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "raycastHit")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("raycastHit event dissapeared!");
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
            pushDynamic(common_lua->luaState,player);
            pushItem(common_lua->luaState,gun);
            pushDynamic(common_lua->luaState,hit);
            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);

            if(lua_pcall(common_lua->luaState,6,0,0))
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

void raycastHit(dynamic *player,item *gun,brick *hit,float x,float y,float z)
{
    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "raycastHit")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("raycastHit event dissapeared!");
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
            pushDynamic(common_lua->luaState,player);
            pushItem(common_lua->luaState,gun);
            pushBrick(common_lua->luaState,hit);
            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);

            if(lua_pcall(common_lua->luaState,6,0,0))
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

void weaponFire(dynamic *player,item *gun,float x,float y,float z,float dirX,float dirY,float dirZ)
{
    if(gun->lastFireEvent + gun->fireCooldownMS > SDL_GetTicks())
        return;

    gun->lastFireEvent = SDL_GetTicks();

    btVector3 raystart = btVector3(x,y,z);
    btVector3 rayend = raystart + btVector3(dirX,dirY,dirZ) * 50.0;

    if(gun->performRaycast)
    {
        btCollisionWorld::AllHitsRayResultCallback res(raystart,rayend);
        common_lua->physicsWorld->rayTest(raystart,rayend,res);

        btRigidBody *closest = 0;
        float closestDist = 999999;
        btVector3 clickPos;

        for(int a = 0; a<res.m_collisionObjects.size(); a++)
        {
            if(res.m_collisionObjects[a] == player)
                continue;
            if(res.m_collisionObjects[a] == gun)
                continue;

            if(closest == 0 || ((res.m_hitPointWorld[a] - raystart).length() < closestDist))
            {
                closestDist = (res.m_hitPointWorld[a] - raystart).length();
                closest = (btRigidBody*)res.m_collisionObjects[a];
                clickPos = res.m_hitPointWorld[a];
            }
        }

        if(closest)
        {
            rayend = clickPos;
            if(closest->getUserIndex() == bodyUserIndex_dynamic)
                raycastHit(player,gun,(dynamic*)closest,clickPos.x(),clickPos.y(),clickPos.z());
            else if(closest->getUserIndex() == bodyUserIndex_brick)
                raycastHit(player,gun,(brick*)closest->getUserPointer(),clickPos.x(),clickPos.y(),clickPos.z());
        }
    }

    packet fireAnimPacket;
    fireAnimPacket.writeUInt(packetType_serverCommand,packetTypeBits);
    fireAnimPacket.writeString("itemFired");
    fireAnimPacket.writeUInt(gun->serverID,dynamicObjectIDBits);
    if(gun->useBulletTrail)
    {
        fireAnimPacket.writeFloat(raystart.x());
        fireAnimPacket.writeFloat(raystart.y());
        fireAnimPacket.writeFloat(raystart.z());
        fireAnimPacket.writeFloat(rayend.x());
        fireAnimPacket.writeFloat(rayend.y());
        fireAnimPacket.writeFloat(rayend.z());
    }

    for(int a = 0; a<common_lua->users.size(); a++)
    {
        if(common_lua->users[a]->controlling && common_lua->users[a]->controlling == gun->heldBy)
            continue;

        common_lua->theServer->send(&fireAnimPacket,true);
    }

    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "weaponFire")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("weaponFire event dissapeared!");
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

            pushDynamic(common_lua->luaState,player);
            pushItem(common_lua->luaState,gun);
            lua_pushnumber(common_lua->luaState,x);
            lua_pushnumber(common_lua->luaState,y);
            lua_pushnumber(common_lua->luaState,z);
            lua_pushnumber(common_lua->luaState,dirX);
            lua_pushnumber(common_lua->luaState,dirY);
            lua_pushnumber(common_lua->luaState,dirZ);

            if(lua_pcall(common_lua->luaState,8,0,0))
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

bool dynamicDeath(dynamic *dying,std::string cause)
{

    eventListener *event = 0;
    for(unsigned int a = 0; a<common_lua->events.size(); a++)
    {
        if(common_lua->events[a].eventName == "dynamicDeath")
        {
            event = &common_lua->events[a];
            break;
        }
    }

    if(!event)
    {
        error("dynamicDeath event dissapeared!");
        return false;
    }

    int top = lua_gettop(common_lua->luaState);

    if(top > 0)
        lua_pop(common_lua->luaState,top);

    bool cancelled = false;

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
            pushDynamic(common_lua->luaState,dying);
            lua_pushstring(common_lua->luaState,cause.c_str());

            if(lua_pcall(common_lua->luaState,2,1,0))
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

void unifiedWorld::applyDamage(dynamic *d,float damage,std::string cause)
{
    if(!d->canTakeDamage)
        return;

    if(damage < 0)
        return;

    scope("unifiedWorld::applyDamage");

    if(d->isPlayer)
    {
        oofCounter++;
        if(oofCounter > 4)
            oofCounter = 1;

        btVector3 pos = d->getWorldTransform().getOrigin();

        addEmitter(getEmitterType("ouchEmitter"),pos.x()+ d->type->eyeOffsetX,pos.y()+ d->type->eyeOffsetY,pos.z()+ d->type->eyeOffsetZ);

        clientData *except = 0;

        for(int a = 0; a<users.size(); a++)
        {
            if(users[a]->controlling == d)
            {
                except = users[a];

                packet vignetteData;
                vignetteData.writeUInt(packetType_serverCommand,packetTypeBits);
                vignetteData.writeString("vignette");
                vignetteData.writeFloat(damage / 25.0);
                vignetteData.writeBit(false);
                users[a]->netRef->send(&vignetteData,true);

                users[a]->camX = pos.x() + d->type->eyeOffsetX;
                users[a]->camY = pos.y() + d->type->eyeOffsetY;
                users[a]->camZ = pos.z() + d->type->eyeOffsetZ;

                break;
            }
        }

        if(damage >= 10.0)
        {
            if(!except)
                playSound("oof" + std::to_string(oofCounter),pos.x(),pos.y(),pos.z(),0.65,1.0);
            else
            {
                playSoundExcept("oof" + std::to_string(oofCounter),pos.x(),pos.y(),pos.z(),except,0.65,1.0);
                playSound("oof" + std::to_string(oofCounter),0.65,1,except);
            }

        }
    }

    d->health -= damage;

    if(d->health > 0)
        return;

    if(dynamicDeath(d,cause))
        d->health = 1;
    else
    {
        if(d->isPlayer)
        {
            for(int a = 0; a<users.size(); a++)
            {
                if(users[a]->controlling == d)
                {
                    messageAll("[colour='FFFF0000']" + users[a]->name + " has died.","death");

                    for(int b = 0; b<queuedRespawn.size(); b++)
                    {
                        if(queuedRespawn[b] == users[a])
                            return;
                    }

                    btVector3 pos = d->getWorldTransform().getOrigin();

                    users[a]->camX = pos.x() + d->type->eyeOffsetX;
                    users[a]->camY = pos.y() + d->type->eyeOffsetY;
                    users[a]->camZ = pos.z() + d->type->eyeOffsetZ;

                    removeDynamic(d);
                    d = 0;

                    users[a]->cameraBoundToDynamic = false;
                    users[a]->cameraTarget = 0;
                    users[a]->cameraFreelookEnabled = true;

                    users[a]->sendCameraDetails();

                    queuedRespawn.push_back(users[a]);
                    queuedRespawnTime.push_back(SDL_GetTicks() + respawnTimeMS);

                    int secs = ceil(respawnTimeMS/1000.0);
                    users[a]->bottomPrint("You will respawn in " + std::to_string(secs) + " seconds.",respawnTimeMS);

                    //spawnPlayer(users[a],0,10,0);
                    return;
                }
            }
            removeDynamic(d);
            d=0;
        }
        else
        {
            removeDynamic(d);
            d=0;
        }
    }
}




