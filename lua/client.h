#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "code/unifiedWorld.h"

extern unifiedWorld *common_lua;

void registerClientFunctions(lua_State *L);

#endif // CLIENT_H_INCLUDED
