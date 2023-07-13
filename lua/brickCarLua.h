#ifndef BRICKCARLUA_H_INCLUDED
#define BRICKCARLUA_H_INCLUDED

#include "code/unifiedWorld.h"

extern unifiedWorld *common_lua;

void registerBrickCarFunctions(lua_State *L);
void luaPushBrickCar(lua_State *L,brickCar *theCar);

#endif // BRICKCARLUA_H_INCLUDED
