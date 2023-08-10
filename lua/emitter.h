#ifndef EMITTER_H_INCLUDED
#define EMITTER_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/lua/pushPopClasses.h"

extern unifiedWorld *common_lua;

void registerEmitterFunctions(lua_State *L);

#endif // EMITTER_H_INCLUDED
