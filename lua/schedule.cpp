#include "code/lua/schedule.h"

static int scheduleLua(lua_State *L)
{
    scope("scheduleLua");

    const char *argName;
    const char *funcName;

    if(lua_gettop(L) == 3)
    {
        argName = lua_tostring(L,-1);
        lua_pop(L,1);
        funcName = lua_tostring(L,-1);
        lua_pop(L,1);
    }
    else if(lua_gettop(L) == 2)
    {
        argName = 0;
        funcName = lua_tostring(L,-1);
        lua_pop(L,1);
    }
    else
    {
        error("Schedule takes 2 or 3 arguments.");
        lua_pushnil(L);
        return 1;
    }

    if(!funcName)
    {
        error("No function name provided!");
        lua_pushnil(L);
        return 1;
    }

    int ms = lua_tointeger(L,-1);
    lua_pop(L,1);

    //std::cout<<"Scheduled function: "<<std::string(funcName)<<" in "<<ms<<"ms with arg: "<<(argName?std::string(argName):"None")<<" "<<lua_gettop(L)<<" on stack\n";

    if(ms < 1)
    {
        error("Milliseconds delay should be > 0" + std::to_string(ms));
        lua_pushnil(L);
        return 1;
    }

    schedule tmp;
    tmp.functionName = funcName;
    if(argName)
        tmp.optionalString = argName;
    else
        tmp.optionalString = "";
    tmp.timeToExecute = SDL_GetTicks() + ms;
    tmp.scheduleID = common_lua->lastScheduleID;

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
    unsigned int id = lua_tointeger(L,-1);
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

