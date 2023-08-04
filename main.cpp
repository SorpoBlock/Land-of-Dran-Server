#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include "code/base/server.h"
#include "code/base/logger.h"
#include "code/base/preference.h"
#include "code/receiveData.h"
#include "code/onConnection.h"
#include "code/onDisconnect.h"
#include "code/brick.h"
#include "code/unifiedWorld.h"
#include "code/lua/miscFunctions.h"
#include "code/lua/dynamic.h"
#include "code/lua/client.h"
#include "code/lua/brickLua.h"
#include "code/lua/schedule.h"
#include "code/lua/events.h"
#include "code/lua/brickCarLua.h"
#include "code/lua/emitter.h"
#include "code/lua/lightLua.h"

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using namespace syj;

unifiedWorld common;
unifiedWorld *common_lua = 0;

void updateClientConsoles(std::string text)
{
    packet data;
    data.writeUInt(packetType_addMessage,packetTypeBits);
    data.writeUInt(2,2); //is console output
    data.writeString(text);

    for(unsigned int a = 0; a<common.users.size(); a++)
    {
        if(common.users[a])
        {
            if(common.users[a]->hasLuaAccess)
            {
                common.users[a]->netRef->send(&data,true);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int maxPlayersThisTime = 0;

    common_lua = &common;
    common.tree = new Octree<brick*>(brickTreeSize*2,0);
    common.tree->setEmptyValue(0);

    logger::setErrorFile("logs/error.txt");
    logger::setInfoFile("logs/log.txt");
    logger::setDebug(false);
    logger::setCallback(updateClientConsoles);

    info("Starting application...");

    common.curlHandle = curl_multi_init();

    if(SDL_Init(SDL_INIT_TIMER))
    {
        error("Could not start up SDL");
        return 0;
    }
    if(SDLNet_Init())
    {
        error("Could not start up SDLNet");
        return 0;
    }

    info("Initalizing physics...");

    btDefaultCollisionConfiguration *collisionConfiguration = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    btGImpactCollisionAlgorithm::registerAlgorithm(dispatcher);
    //common.physicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    common.physicsWorld = new btSoftRigidDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    btVector3 gravity = btVector3(0,-70,0);
    common.physicsWorld->setGravity(gravity);
    common.physicsWorld->setForceUpdateAllAabbs(false);
    common.softBodyWorldInfo.m_broadphase = broadphase;
    common.softBodyWorldInfo.m_dispatcher = dispatcher;
    common.softBodyWorldInfo.m_gravity = gravity;
    common.softBodyWorldInfo.m_sparsesdf.Initialize();

    btCollisionShape *plane = new btStaticPlaneShape(btVector3(0,1,0),0);
    btDefaultMotionState* planeState = new btDefaultMotionState();
    btRigidBody::btRigidBodyConstructionInfo planeCon(0,planeState,plane);
    btRigidBody *groundPlane = new btRigidBody(planeCon);
    groundPlane->setFriction(1.0);
    groundPlane->setUserIndex(bodyUserIndex_plane);
    common.physicsWorld->addRigidBody(groundPlane);

    info("Launching server...");
    networkingPreferences prefs;
    common.theServer = new server(prefs);
    common.theServer->receiveHandle = receiveData;
    common.theServer->connectHandle = onConnection;
    common.theServer->disconnectHandle = onDisconnect;
    common.theServer->userData = &common;

    info("Loading Lua libraries");

    common.luaState = luaL_newstate();
    luaL_openlibs(common.luaState);
    bindMiscFuncs(common.luaState);
    registerDynamicFunctions(common.luaState);
    registerClientFunctions(common.luaState);
    registerBrickFunctions(common.luaState);
    registerBrickCarFunctions(common.luaState);
    registerScheduleFunctions(common.luaState);
    registerEmitterFunctions(common.luaState);
    registerLightFunctions(common.luaState);
    addEventNames(common.luaState);

    info("Loading add-ons...");
    for (recursive_directory_iterator i("addons"), end; i != end; ++i)
    {
        if (!is_directory(i->path()))
        {
            if(i->path().filename() == "server.lua")
            {
                std::string addonName = i->path().parent_path().string();
                if(addonName.substr(0,7) == "addons\\")
                    addonName = addonName.substr(7,addonName.length() - 7);
                if(addonName.find("\\") != std::string::npos)
                {
                    error("Not loading " + i->path().string() + " because it is nested!");
                    continue;
                }
                if(common.luaState,i->path().string().length() < 1)
                    continue;

                info("Loading " + addonName);
                if(luaL_dofile(common.luaState,i->path().string().c_str()))
                {
                    error("Lua error loading addon " + addonName);
                    if(lua_gettop(common.luaState) > 0)
                    {
                        const char* err = lua_tostring(common.luaState,-1);
                        if(err)
                            error(err);
                        if(lua_gettop(common.luaState) > 0)
                            lua_pop(common.luaState,1);
                    }
                }
            }
        }
    }

    info("Loading decals");
    common.loadDecals("addons/decals");

    info("Loading default types...");
    common.brickTypes = new brickLoader("assets/brick/types","assets/brick/prints");

    dynamicType *defaultPlayerType = new dynamicType("assets/brickhead/brickhead.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    defaultPlayerType->eyeOffsetX = 0;
    defaultPlayerType->eyeOffsetY = 4.5;
    defaultPlayerType->eyeOffsetZ = 0;
    defaultPlayerType->standingFrame = 36;
    defaultPlayerType->filePath = "assets/brickhead/brickhead.txt";
    common.dynamicTypes.push_back(defaultPlayerType);

    dynamicType *hammer = new dynamicType("assets/tools/hammer.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    hammer->filePath = "assets/tools/hammer.txt";
    common.dynamicTypes.push_back(hammer);

    itemType *hammerItem = new itemType(hammer,0);
    hammerItem->uiName = "Hammer";
    hammerItem->handOffsetX = 1.8;
    hammerItem->handOffsetY = 3.5;
    hammerItem->handOffsetZ = -2.0;
    common.itemTypes.push_back(hammerItem);

    dynamicType *wrench = new dynamicType("assets/tools/wrench.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    wrench->filePath = "assets/tools/wrench.txt";
    common.dynamicTypes.push_back(wrench);

    itemType *wrenchItem = new itemType(wrench,1);
    wrenchItem->uiName = "Wrench";
    wrenchItem->handOffsetX = 1.8;
    wrenchItem->handOffsetY = 3.5;
    wrenchItem->handOffsetZ = -2.0;
    common.itemTypes.push_back(wrenchItem);

    dynamicType *spraycan = new dynamicType("assets/tools/spraycan.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    spraycan->filePath = "assets/tools/spraycan.txt";
    common.dynamicTypes.push_back(spraycan);

    itemType *spraycanItem = new itemType(spraycan,2);
    spraycanItem->uiName = "Paint Can";
    spraycanItem->handOffsetX = 1.8;
    spraycanItem->handOffsetY = 2.8;
    spraycanItem->handOffsetZ = -2.0;
    common.itemTypes.push_back(spraycanItem);

    common.addMusicType("None","");
    common.addSoundType("BrickBreak","assets/sound/breakBrick.wav");
    common.addSoundType("ClickMove","assets/sound/clickMove.wav");
    common.addSoundType("ClickPlant","assets/sound/clickPlant.wav");
    common.addSoundType("ClickRotate","assets/sound/clickRotate.wav");
    common.addSoundType("FastImpact","assets/sound/fastimpact.wav");
    common.addSoundType("HammerHit","assets/sound/hammerHit.WAV");
    common.addSoundType("Jump","assets/sound/jump.wav");
    common.addSoundType("PlayerConnect","assets/sound/playerConnect.wav");
    common.addSoundType("PlayerLeave","assets/sound/playerLeave.wav");
    common.addSoundType("PlayerMount","assets/sound/playerMount.wav");
    common.addSoundType("ToolSwitch","assets/sound/weaponSwitch.wav");
    common.addSoundType("WrenchHit","assets/sound/wrenchHit.wav");
    common.addSoundType("WrenchMiss","assets/sound/wrenchMiss.wav");
    common.addMusicType("Zero Day","assets/sound/zerodaywip.wav");
    common.addMusicType("Vehicle Hover","assets/sound/vehicleHover.wav");
    common.addMusicType("Police Siren","assets/sound/policeSiren.wav");
    common.addMusicType("Birds Chirping","assets/sound/springBirds.wav");
    common.addMusicType("Euphoria","assets/sound/djgriffinEuphoria.wav");
    common.addSoundType("Death","assets/sound/betterDeath.wav");
    common.addSoundType("Win","assets/sound/orchhith.wav");
    common.addSoundType("Splash","assets/sound/splash1.wav");
    common.addSoundType("Spawn","assets/sound/spawn.wav");
    common.addSoundType("NoteFs5","assets/sound/NoteFs5.wav");
    common.addSoundType("NoteA6","assets/sound/NoteA6.wav");
    common.addSoundType("Beep","assets/sound/error.wav");
    common.addSoundType("Rattle","assets/sound/sprayActivate.wav");
    common.addSoundType("Spray","assets/sound/sprayLoop.wav");
    common.addSoundType("ClickChange","assets/sound/clickChange.wav");
    common.addSoundType("Admin","assets/sound/admin.wav");
    common.addMusicType("Analog","assets/sound/analog.wav");
    common.addMusicType("Drums","assets/sound/drums.wav");

    info("Loading initial build...");
    common.loadBlocklandSave("assets/brick/CityCollab.bls");

    /*for(unsigned int a = 0; a<5; a++)
    {
        item *i = common.addItem(hammerItem);
        btTransform t;
        t.setOrigin(btVector3(90,100,100));
        t.setRotation(btQuaternion(0,0,0,1));
        i->setWorldTransform(t);

        i = common.addItem(wrenchItem);
        t.setOrigin(btVector3(110,110,100));
        t.setRotation(btQuaternion(0,0,0,1));
        i->setWorldTransform(t);
    }*/

    info("Start-up complete.");

    bool cont = true;
    float deltaT = 0;
    unsigned int lastTick = SDL_GetTicks();
    unsigned int lastFrameCheck = SDL_GetTicks();
    unsigned int frames = 0;

    //unsigned int msBetweenUpdates = 60;
    //bool everyOtherLoop = false;
    unsigned int msAtLastUpdate = SDL_GetTicks();
    while(cont)
    {
        if(common.curlHandle)
        {
            int runningThreads;
            CURLMcode mc = curl_multi_perform(common.curlHandle,&runningThreads);
            if(mc)
            {
                error("curl_multi_perform failed with error code " + std::to_string(mc));
            }
        }

        std::vector<serverClientHandle*> toKick;

        for(int a = 0; a<common.users.size(); a++)
        {
            //A log in failed...
            if(common.users[a]->logInState == 3)
            {
                //We can't kick them directly because they'll instantly be removed from common.users
                //Thus messing up our for loop
                toKick.push_back(common.users[a]->netRef);
                continue;
            }
            //Process a good log in that just happened
            else if(common.users[a]->logInState == 4)
            {
                clientData *source = common.users[a];

                packet data;
                data.writeUInt(packetType_connectionRepsonse,packetTypeBits);
                data.writeBit(true);
                data.writeUInt(getServerTime(),32);
                data.writeUInt(common.brickTypes->specialBrickTypes,10);
                data.writeUInt(common.dynamicTypes.size(),10);
                data.writeUInt(common.itemTypes.size(),10);
                source->netRef->send(&data,true);

                info("New logged in user from " + source->netRef->getIP() + " now known as " + source->name + " their netID is " + std::to_string(common.lastPlayerID));

                source->playerID = common.lastPlayerID;
                common.lastPlayerID++;

                for(int a = 0; a<common.users.size(); a++)
                {
                    packet data;
                    data.writeUInt(packetType_addOrRemovePlayer,packetTypeBits);
                    data.writeBit(true); // for player's list, not typing players list
                    data.writeBit(true); //add
                    data.writeString(source->name);
                    data.writeUInt(source->playerID,16);
                    common.users[a]->netRef->send(&data,true);
                }

                common.playSound("PlayerConnect");
                common.messageAll("[colour='FF0000FF']" + source->name + " connected.","server");

                sendInitalDataFirstHalf(&common,source,source->netRef);

                //This only needs to happen once...
                common.users[a]->logInState = 2;

                if(source->authHandle)
                {
                    curl_multi_remove_handle(common.curlHandle,source->authHandle);
                    curl_easy_cleanup(source->authHandle);
                    source->authHandle = 0;
                }
            }
        }

        for(int a = 0; a<toKick.size(); a++)
            common.theServer->kick(toKick[a]);

        if(common.users.size() > maxPlayersThisTime)
        {
            maxPlayersThisTime = common.users.size();
            info("New max player record: " + std::to_string(maxPlayersThisTime));
        }

        deltaT = SDL_GetTicks() - lastTick;
        lastTick = SDL_GetTicks();
        common.physicsWorld->stepSimulation(deltaT / 1000);

        if(SDL_GetTicks() > lastFrameCheck + 10000)
        {
            float fps = frames;
            fps /= 10.0;
            //if(fps < 10000)
              //  info("Last FPS: " + std::to_string(fps));
            frames = 0;
            lastFrameCheck = SDL_GetTicks();
        }

        for(unsigned int a = 0; a<common.brickCars.size(); a++)
        {
            if(!common.brickCars[a])
                continue;

            if(common.brickCars[a]->wheels.size() != 2)
                continue;

            if(!common.brickCars[a]->occupied)
                continue;

            btRigidBody *car = common.brickCars[a]->body;
            btTransform rotMatrix = car->getWorldTransform();
            rotMatrix.setOrigin(btVector3(0,0,0));
        }

        for(unsigned int a = 0; a<common.brickCars.size(); a++)
        {
            if(!common.brickCars[a])
                continue;

            brickCar *car = common.brickCars[a];

            if(car->body->getLinearVelocity().length() > 1000)
            {
                error("Remove car: lin vel > 1000");
                common.removeBrickCar(car);
                break;
            }
            if(car->body->getAngularVelocity().length() > 300)
            {
                error("Remove car: ang vel > 300");
                common.removeBrickCar(car);
                break;
            }
            if(car->body->getWorldTransform().getOrigin().length() > 10000)
            {
                error("Remove car: dist > 10000");
                common.removeBrickCar(car);
                break;
            }
            if(isnan(car->body->getWorldTransform().getOrigin().length()))
            {
                error("Remove car: nan position");
                common.removeBrickCar(car);
                break;
            }
            if(isnan(car->body->getWorldTransform().getOrigin().x()) || isinf(car->body->getWorldTransform().getOrigin().x()))
            {
                error("Remove car: nan position");
                common.removeBrickCar(car);
                break;
            }

            for(int b = 0; b<car->vehicle->getNumWheels(); b++)
            {
                car->wheels[b].dirtEmitterOn = false;
                if(fabs(car->vehicle->getCurrentSpeedKmHour()) > 50 && car->doTireEmitter && car->vehicle->getWheelInfo(b).m_raycastInfo.m_isInContact)
                {
                    car->wheels[b].dirtEmitterOn = true;
                }
            }
        }

        for(unsigned int a = 0; a<common.users.size(); a++)
        {
            //Check for partially loaded cars to purge:
            if(common.users[a]->loadingCar)
            {
                if(SDL_GetTicks() - common.users[a]->carLoadStartTime > 10000)
                {
                    info("User: " + common.users[a]->name + " has a partially loaded car that will now be deleted.");

                    for(int b = 0; b<common.users[a]->loadingCarLights.size(); b++)
                        delete common.users[a]->loadingCarLights[b];
                    common.users[a]->loadingCarLights.clear();
                    for(int b = 0; b<common.users[a]->loadingCar->bricks.size(); b++)
                        delete common.users[a]->loadingCar->bricks[b];
                    common.users[a]->loadingCar->bricks.clear();

                    delete common.users[a]->loadingCar;
                    common.users[a]->loadingCar = 0;

                    common.users[a]->basicBricksLeftToLoad = 0;
                    common.users[a]->specialBricksLeftToLoad = 0;
                    common.users[a]->loadedCarAmountsPacket = false;
                    common.users[a]->loadedCarLights = false;
                    common.users[a]->loadedCarWheels = false;
                }
            }


            if(!common.users[a]->controlling)
                continue;

            dynamic *player = common.users[a]->controlling;

            if(!player)
                continue;

            //Client physics position sync interpolation:
            const float interpolationTime = 200.0;
            float progress = SDL_GetTicks() - common.users[a]->interpolationStartTime;
            if(progress < interpolationTime)
            {
                float lastProgress = common.users[a]->lastInterpolation - common.users[a]->interpolationStartTime;
                float difference = progress - lastProgress;
                difference /= interpolationTime;

                btTransform t = player->getWorldTransform();
                t.setOrigin(t.getOrigin() + common.users[a]->interpolationOffset * btVector3(difference,difference,difference));
                player->setWorldTransform(t);


                common.users[a]->lastInterpolation = SDL_GetTicks();
            }

            float yaw = atan2(player->lastCamX,player->lastCamZ);
            bool playJumpSound = player->control(yaw,player->lastControlMask & 1,player->lastControlMask & 2,player->lastControlMask & 4,player->lastControlMask & 8,player->lastControlMask &16,common.physicsWorld,!common.users[a]->prohibitTurning,common.useRelativeWalkingSpeed,common.users[a]->debugColors,common.users[a]->debugPositions);

            btVector3 pos = player->getWorldTransform().getOrigin();
            //if(playJumpSound)
                //common.playSound("Jump",pos.x(),pos.y(),pos.z(),false);

            if(fabs(pos.x()) > 10000 || fabs(pos.y()) > 10000 || fabs(pos.z()) > 10000 || isnan(pos.x()) || isnan(pos.y()) || isnan(pos.z()) || isinf(pos.x()) || isinf(pos.y()) || isinf(pos.z()))
            {
                error("Player position messed up: " + std::to_string(pos.x()) + "," + std::to_string(pos.y()) + "," + std::to_string(pos.z()));
                common.spawnPlayer(common.users[a],5,15,5);
            }

        }

        for(unsigned int a = 0; a<common.dynamics.size(); a++)
        {
            dynamic *d = common.dynamics[a];
            if(!d)
                continue;

            //boyancy and jets
            float waterLevel = 15.0;
            if(!d->isJetting)
            {
                if(d->getWorldTransform().getOrigin().y() < waterLevel-2)
                {
                    d->setDamping(0.4,0);
                    d->setGravity(btVector3(0,-0.5,0) * common.physicsWorld->getGravity());
                }
                else if(d->getWorldTransform().getOrigin().y() < waterLevel)
                {
                    d->setDamping(0.4,0);
                    d->setGravity(btVector3(0,0,0));
                }
                else
                {
                    d->setDamping(0,0);
                    d->setGravity(common.physicsWorld->getGravity());
                }
            }

            if(d->sittingOn)
            {
                btTransform t = d->sittingOn->getWorldTransform();
                t.setOrigin(btVector3(0,0,0));

                //std::cout<<"Index: "<<d->getUserIndex()<<"\n";
                if(d->sittingOn->getUserIndex() == bodyUserIndex_builtCar)
                {
                    if(!d->sittingOn->getUserPointer())
                        continue;

                    brickCar *theCar = (brickCar*)d->sittingOn->getUserPointer();

                    btVector3 wheelPos = theCar->wheelPosition + btVector3(0,-1,theCar->builtBackwards ? 2 : -2);
                    wheelPos = t * wheelPos;
                    wheelPos = wheelPos + theCar->body->getWorldTransform().getOrigin();

                    t.setOrigin(wheelPos);

                    btTransform afterRotCorrection;
                    afterRotCorrection.setOrigin(btVector3(0,0,0));
                    afterRotCorrection.setRotation(t.getRotation());

                    btTransform rotCorrection;
                    rotCorrection.setOrigin(btVector3(0,0,0));
                    rotCorrection.setRotation(btQuaternion(3.1415,0,0));

                    if(!theCar->builtBackwards)
                        afterRotCorrection = afterRotCorrection * rotCorrection;

                    t.setRotation(afterRotCorrection.getRotation());

                    d->setWorldTransform(t);

                    continue;
                }

                btVector3 localUp = t * btVector3(0,1.5,0);
                t.setOrigin(t.getOrigin() + localUp);
                d->setWorldTransform(t);
            }
            else
            {
                btVector3 pos = d->getWorldTransform().getOrigin();
                btVector3 vel = d->getLinearVelocity();

                if(d->lastInWater)
                {
                    if(pos.y() > common.waterLevel + 2)
                        d->lastInWater = false;
                }
                else
                {
                    if(pos.y() < (common.waterLevel-1))
                    {
                        if(d->lastSplashEffect + 1000 < SDL_GetTicks())
                        {
                            d->lastSplashEffect = SDL_GetTicks();
                            d->lastInWater = true;
                            common.playSound("Splash",pos.x(),common.waterLevel,pos.z(),false);
                            common.addEmitter(common.getEmitterType("playerBubbleEmitter"),pos.x(),common.waterLevel,pos.z());
                        }
                    }
                }
            }
        }

        int msToWait = (msAtLastUpdate+20)-SDL_GetTicks();
        //We send time of packet in milliseconds so if the millisecond is the same that will cause errors
        if(msToWait > 0)
            SDL_Delay(msToWait);
        int msDiff = SDL_GetTicks() - msAtLastUpdate;
        msAtLastUpdate = SDL_GetTicks();

        //if(msAtLastUpdate+msBetweenUpdates <= SDL_GetTicks())
        if(true)
        {
            auto emitterIter = common.emitters.begin();
            while(emitterIter != common.emitters.end())
            {
                emitter *e = *emitterIter;
                if(e->type->lifetimeMS != 0)
                {
                    if(e->creationTime + e->type->lifetimeMS < SDL_GetTicks())
                    {
                        if(e->isJetEmitter)
                        {
                            for(int a = 0; a<common.users.size(); a++)
                            {
                                if(common.users[a]->leftJet == e)
                                    common.users[a]->leftJet = 0;
                                if(common.users[a]->rightJet == e)
                                    common.users[a]->rightJet = 0;
                                if(common.users[a]->paint == e)
                                    common.users[a]->paint = 0;
                                if(common.users[a]->adminOrb == e)
                                    common.users[a]->adminOrb = 0;
                            }
                        }

                        delete e;
                        emitterIter = common.emitters.erase(emitterIter);
                        continue;
                    }
                }
                ++emitterIter;
            }

            common.runningSchedules = true;
            auto sch = common.schedules.begin();
            while(sch != common.schedules.end())
            {
                int args = lua_gettop(common.luaState);
                if(args > 0)
                    lua_pop(common.luaState,args);

                if((*sch).timeToExecute < SDL_GetTicks())
                {
                    lua_getglobal(common.luaState,(*sch).functionName.c_str());
                    if(!lua_isfunction(common.luaState,1))
                    {
                        error("Schedule was passed nil or another type instead of a function! Name: " + (*sch).functionName);
                        error("Error in schedule " + std::to_string((*sch).scheduleID));
                        int args = lua_gettop(common.luaState);
                        if(args > 0)
                            lua_pop(common.luaState,args);
                    }
                    else
                    {
                        if(lua_pcall(common.luaState,0,0,0) != 0)
                        {
                            error("Error in schedule " + std::to_string((*sch).scheduleID) + " func: " + (*sch).functionName );
                            if(lua_gettop(common.luaState) > 0)
                            {
                                const char *err = lua_tostring(common.luaState,-1);
                                if(!err)
                                    continue;
                                std::string errorstr = err;

                                int args = lua_gettop(common.luaState);
                                if(args > 0)
                                    lua_pop(common.luaState,args);

                                replaceAll(errorstr,"[","\\[");

                                error("[colour='FFFF0000']" + errorstr);
                            }
                        }
                    }

                    sch = common.schedules.erase(sch);
                }
                else
                    ++sch;
            }
            common.runningSchedules = false;

            for(unsigned int a = 0; a<common.tmpSchedules.size(); a++)
                common.schedules.push_back(common.tmpSchedules[a]);
            common.tmpSchedules.clear();

            for(unsigned int a = 0; a<common.tmpScheduleIDsToDelete.size(); a++)
            {
                for(unsigned int b = 0; b<common.schedules.size(); b++)
                {
                    if(common.schedules[b].scheduleID == common.tmpScheduleIDsToDelete[a])
                    {
                        common.schedules.erase(common.schedules.begin() + b);
                        break;
                    }
                }
            }
            common.tmpScheduleIDsToDelete.clear();

            /*for(unsigned int i = 0; i<common.users.size(); i++)
            {
                if(common.users[i]->debugColors.size() > 0)
                {
                    packet data;
                    data.writeUInt(packetType_debugLocs,packetTypeBits);
                    int numLocs = common.users[i]->debugColors.size();
                    if(numLocs > 50)
                        numLocs = 50;
                    data.writeUInt(numLocs,6);
                    for(int k = 0; k<numLocs; k++)
                    {
                        data.writeFloat(common.users[i]->debugColors[k].x());
                        data.writeFloat(common.users[i]->debugColors[k].y());
                        data.writeFloat(common.users[i]->debugColors[k].z());
                        data.writeFloat(common.users[i]->debugPositions[k].x());
                        data.writeFloat(common.users[i]->debugPositions[k].y());
                        data.writeFloat(common.users[i]->debugPositions[k].z());
                    }
                    common.users[i]->netRef->send(&data,false);
                }
            }*/

            int numManifolds = common.physicsWorld->getDispatcher()->getNumManifolds();
            for (int i = 0; i < numManifolds; i++)
            {
                btPersistentManifold* contactManifold =  common.physicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
                btRigidBody* obA = (btRigidBody*)contactManifold->getBody0();
                btRigidBody* obB = (btRigidBody*)contactManifold->getBody1();

                if(!(obA->getUserIndex() == bodyUserIndex_builtCar || obB->getUserIndex() == bodyUserIndex_builtCar))
                    continue;

                if(obA->getUserIndex() == bodyUserIndex_plane || obB->getUserIndex() == bodyUserIndex_plane)
                    continue;

                if((unsigned)obA->getUserIndex2() + 1000 > SDL_GetTicks())
                    continue;
                if((unsigned)obB->getUserIndex2() + 1000 > SDL_GetTicks())
                    continue;

                if(obA->getUserIndex() == bodyUserIndex_builtCar)
                {
                    if(obB->getUserIndex() == bodyUserIndex_dynamic && obB->getUserPointer())
                    {
                        dynamic *other = (dynamic*)obB->getUserPointer();
                        if(other->sittingOn == obA)
                            continue;
                    }
                }
                else if(obB->getUserIndex() == bodyUserIndex_builtCar)
                {
                    if(obA->getUserIndex() == bodyUserIndex_dynamic && obA->getUserPointer())
                    {
                        dynamic *other = (dynamic*)obA->getUserPointer();
                        if(other->sittingOn == obB)
                            continue;
                    }
                }

                //std::cout<<"Speed: "<<(obA->getLinearVelocity() - obB->getLinearVelocity()).length()<<"\n";
                if((obA->getLinearVelocity() - obB->getLinearVelocity()).length() > 30)
                {
                    btVector3 pos = obA->getWorldTransform().getOrigin();
                    obA->setUserIndex2(SDL_GetTicks());
                    obB->setUserIndex2(SDL_GetTicks());
                    common.playSound("FastImpact",pos.x(),pos.y(),pos.z(),false);
                }
            }
            /*int active = 0;
            for(unsigned int a = 0; a<common.brickCars.size(); a++)
            {
                if(common.brickCars[a]->body->isActive())
                    active++;
            }
            std::cout<<"Cars: "<<common.brickCars.size()<<" active: "<<active<<"\n";*/

            for(unsigned int a = 0; a<common.users.size(); a++)
            {
                if(common.users[a])
                {
                    int timeSinceLastPing = SDL_GetTicks() - common.users[a]->lastPingSentAtMS;

                    if(timeSinceLastPing > 1000)
                        common.users[a]->sendPing();

                    if(common.users[a]->needsCameraUpdate)
                    {
                        common.users[a]->sendCameraDetails();
                        common.users[a]->needsCameraUpdate = false;
                    }
                }
            }

            int dynamicsSentSoFar = 0;
            int dynamicsToSend = common.dynamics.size();
            int vehiclesSentSoFar = 0;
            int vehiclesToSend = common.brickCars.size();
            while(dynamicsToSend > 0 || vehiclesToSend > 0)
            {
                int bitsLeft = packetMTUbits - (packetTypeBits+8+8+20+8);

                int dynamicsSentThisPacket = 0;
                int dynamicsSkippedThisPacket = 0;

                while((dynamicsSentThisPacket+dynamicsSkippedThisPacket) < dynamicsToSend && bitsLeft > 0)
                {
                    dynamic *tmp = common.dynamics[dynamicsSentSoFar + dynamicsSentThisPacket + dynamicsSkippedThisPacket];
                    /*if(tmp->otherwiseChanged || (tmp->getWorldTransform().getOrigin()-tmp->lastPosition).length() > 0.01 || (tmp->getWorldTransform().getRotation() - tmp->lastRotation).length() > 0.01 || SDL_GetTicks() - tmp->lastSentTransform > 300)
                    {*/
                        tmp->otherwiseChanged = false;
                        tmp->lastPosition = tmp->getWorldTransform().getOrigin();
                        tmp->lastRotation = tmp->getWorldTransform().getRotation();
                        tmp->lastSentTransform = SDL_GetTicks();
                        bitsLeft -= tmp->getPacketBits();
                        dynamicsSentThisPacket++;
                    /*}
                    else
                        dynamicsSkippedThisPacket++;*/
                }

                //std::cout<<"Dynamics sent this packet: "<<dynamicsSentThisPacket<<" skipped: "<<dynamicsSkippedThisPacket<<" bits left: "<<bitsLeft<<"\n";

                dynamicsToSend -= dynamicsSentThisPacket + dynamicsSkippedThisPacket;

                int vehiclesSentThisPacket = 0;

                while(vehiclesSentThisPacket < vehiclesToSend && bitsLeft > 0)
                {
                    bitsLeft -= common.brickCars[vehiclesSentSoFar + vehiclesSentThisPacket]->getPacketBits();
                    vehiclesSentThisPacket++;
                }

                //std::cout<<"Vehicles sent this packet: "<<vehiclesSentThisPacket<<" bits left: "<<bitsLeft<<"\n";

                vehiclesToSend -= vehiclesSentThisPacket;

                if(dynamicsSentThisPacket + vehiclesSentThisPacket <= 0)
                    break;

                packet transformPacket;
                transformPacket.writeUInt(packetType_updateDynamicTransforms,packetTypeBits);
                transformPacket.writeUInt(getServerTime(),32);
                //transformPacket.writeUInt(msDiff,8);
                transformPacket.writeUInt(dynamicsSentThisPacket,8);
                //transformPacket.writeUInt(common.lastSnapshotID,20);

                for(unsigned int a = dynamicsSentSoFar; a<dynamicsSentSoFar+dynamicsSentThisPacket+dynamicsSkippedThisPacket; a++)
                    common.dynamics[a]->addToPacket(&transformPacket);
                transformPacket.writeUInt(vehiclesSentThisPacket,8);
                for(unsigned int a = vehiclesSentSoFar; a<vehiclesSentSoFar+vehiclesSentThisPacket; a++)
                    common.brickCars[a]->addToPacket(&transformPacket);
                transformPacket.writeUInt(0,8); //no ropes

                common.theServer->send(&transformPacket,false);
                dynamicsSentSoFar += dynamicsSentThisPacket + dynamicsSkippedThisPacket;
                vehiclesSentSoFar += vehiclesSentThisPacket;
            }

            common.lastSnapshotID++;

            if(common.ropes.size() > 0)
            {
                packet ropeTransformPacket;
                ropeTransformPacket.writeUInt(packetType_updateDynamicTransforms,packetTypeBits);
                ropeTransformPacket.writeUInt(getServerTime(),32);
                ropeTransformPacket.writeUInt(0,8); //no vehicles or dynamics
                ropeTransformPacket.writeUInt(0,8);
                ropeTransformPacket.writeUInt(common.ropes.size(),8);
                for(int a = 0; a<common.ropes.size(); a++)
                    common.ropes[a]->addToPacket(&ropeTransformPacket);
                common.theServer->send(&ropeTransformPacket,false);
            }
        }

        frames++;

        common.theServer->run();
    }

    return 0;
}









