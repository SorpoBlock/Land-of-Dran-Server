#ifndef LIGHTLUA_H_INCLUDED
#define LIGHTLUA_H_INCLUDED

#include "code/light.h"
#include "code/unifiedWorld.h"
#include "code/lua/pushPopClasses.h"

extern unifiedWorld *common_lua;

void registerLightFunctions(lua_State *L);

#endif // LIGHTLUA_H_INCLUDED
