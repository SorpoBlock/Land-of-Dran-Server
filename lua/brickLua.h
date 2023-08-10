#ifndef BRICKLUA_H_INCLUDED
#define BRICKLUA_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/lua/pushPopClasses.h"

extern unifiedWorld *common_lua;

void registerBrickFunctions(lua_State *L);

#endif // BRICKLUA_H_INCLUDED
