#ifndef DYNAMIC_H_INCLUDED
#define DYNAMIC_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/lua/pushPopClasses.h"

extern unifiedWorld *common_lua;

/*
static int LUAplayerToString(lua_State *L);
static int getPlayerIndex(lua_State *L);
*/

void registerDynamicFunctions(lua_State *L);


#endif // DYNAMIC_H_INCLUDED
