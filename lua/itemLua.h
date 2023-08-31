#ifndef ITEMLUA_H_INCLUDED
#define ITEMLUA_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/lua/pushPopClasses.h"

extern unifiedWorld *common_lua;

void registerItemFunctions(lua_State *L);


#endif // ITEMLUA_H_INCLUDED
