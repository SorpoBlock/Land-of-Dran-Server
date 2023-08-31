#ifndef EVENTS_H_INCLUDED
#define EVENTS_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/lua/brickCarLua.h"

extern unifiedWorld *common_lua;

void addEventNames(lua_State *L);
void registerEvent(std::string eventName,std::string functionName);
void unregisterEvent(std::string eventName,std::string functionName);

//bool indicates cancelled
bool clientAdminOrb(clientData* source,bool startOrStop,float &x,float &y,float &z);
bool spawnPlayerEvent(clientData *client,float &x,float &y,float &z,bool cancelled = false);
void clientClickBrickEvent(clientData *source,brick *theBrick,float x,float y,float z,bool isLeft);
void clientPlantBrickEvent(clientData *source,brick *theBrick);
void clientLeaveEvent(clientData *source);
bool clientTryLoadCarEvent(clientData *source,brickCar *theCar,bool loadCarAsCar);
bool clientChat(clientData *source,std::string &message);
void clientJoin(clientData *source);
void projectileHit(brick *hitBrick,float x,float y,float z,std::string tag);
void projectileHit(dynamic *hitDynamic,float x,float y,float z,std::string tag);
void weaponFire(dynamic *player,item *gun,float x,float y,float z,float dirX,float dirY,float dirZ);
bool dynamicDeath(dynamic *dying,std::string cause);

#endif // EVENTS_H_INCLUDED
