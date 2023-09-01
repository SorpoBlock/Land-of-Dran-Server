#include "code/lua/schedule.h"

static int scheduleLua(lua_State *L)
{
    scope("scheduleLua");

    int args = lua_gettop(L);

    if(args < 2)
    {
        error("At least 2 arguments required!");
        lua_pushnil(L);
        return 1;
    }

    const char *functionName = lua_tostring(L,2);

    if(!functionName)
    {
        error("Invalid function name string!");
        lua_pushnil(L);
        return 1;
    }

    int milliseconds = lua_tointeger(L,1);
    if(milliseconds < 1)
    {
        error("Milliseconds must be > 0");
        lua_pushnil(L);
        return 1;
    }

    lua_remove(L,2);
    lua_remove(L,1);

    if(args > 2)
    {
        lua_newtable(L);
        lua_insert(L,1);

        for(int a = 2; a<args; a++)
        {
            lua_pushinteger(L,a-1);
            lua_insert(L,-2);
            lua_settable(L,1);
        }

        lua_getglobal(L,"scheduleArgs");
        if(!lua_istable(L,-1))
        {
            lua_pop(L,1);
            lua_newtable(L);
            lua_setglobal(L,"scheduleArgs");
            lua_getglobal(L,"scheduleArgs");
        }

        lua_insert(L,1);

        lua_pushinteger(L,common_lua->lastScheduleID);
        lua_insert(L,-2);

        lua_settable(L,1);
        lua_pop(L,1);
    }

    schedule tmp;
    tmp.functionName = std::string(functionName);
    tmp.timeToExecute = SDL_GetTicks() + milliseconds;
    tmp.scheduleID = common_lua->lastScheduleID;
    tmp.numLuaArgs = args - 2;

    if(common_lua->runningSchedules)
        common_lua->tmpSchedules.push_back(tmp);
    else
        common_lua->schedules.push_back(tmp);

    lua_pushnumber(L,common_lua->lastScheduleID);
    common_lua->lastScheduleID++;

    return 1;
}

static int cancelLua(lua_State *L)
{
    scope("cancelLua");

    unsigned int id = lua_tointeger(L,-1);
    lua_pop(L,1);

    lua_getglobal(L,"scheduleArgs");
    if(!lua_istable(L,-1))
        error("Where did your scheduleArgs table go!");
    else
    {
        lua_pushinteger(L,id);
        lua_gettable(L,-2);

        if(lua_isnil(L,-1))
            lua_pop(L,1);
        else
        {
            lua_pop(L,1);

            lua_pushinteger(L,id);
            lua_pushnil(L);
            lua_settable(L,-3);
        }
    }
    lua_pop(L,1);

    if(common_lua->runningSchedules)
    {
        common_lua->tmpScheduleIDsToDelete.push_back(id);
    }
    else
    {
        for(unsigned int a = 0; a<common_lua->schedules.size(); a++)
        {
            if(common_lua->schedules[a].scheduleID == id)
            {
                common_lua->schedules.erase(common_lua->schedules.begin() + a);
                return 0;
            }
        }
    }

    return 0;
}

void registerScheduleFunctions(lua_State *L)
{
    lua_register(L,"schedule",scheduleLua);
    lua_register(L,"cancel",cancelLua);
}

