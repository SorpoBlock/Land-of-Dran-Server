#ifndef SCHEDULE_H_INCLUDED
#define SCHEDULE_H_INCLUDED

#include "code/unifiedWorld.h"

extern unifiedWorld *common_lua;

void registerScheduleFunctions(lua_State *L);

#endif // SCHEDULE_H_INCLUDED
