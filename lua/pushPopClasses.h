#ifndef PUSHPOPCLASSES_H_INCLUDED
#define PUSHPOPCLASSES_H_INCLUDED

#include "code/unifiedWorld.h"

extern unifiedWorld *common_lua;

void pushBrick(lua_State *L,brick *toPush);
brick *popBrick(lua_State *L);
void luaPushBrickCar(lua_State *L,brickCar *theCar);
brickCar *popBrickCar(lua_State *L);
void pushEmitter(lua_State *L,emitter *e);
emitter *popEmitter(lua_State *L);
clientData* popClient(lua_State *L);
void pushClient(lua_State *L,clientData* client);
void pushDynamic(lua_State *L,dynamic *object);
dynamic *popDynamic(lua_State *L,bool supressErrors = false);

#endif // PUSHPOPCLASSES_H_INCLUDED
