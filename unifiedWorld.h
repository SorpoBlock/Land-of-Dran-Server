#ifndef UNIFIEDWORLD_H_INCLUDED
#define UNIFIEDWORLD_H_INCLUDED

#include "code/light.h"
#include "code/brick.h"
#include "code/dynamic.h"
#include "code/livingBrick.h"
#include "code/item.h"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/glm.hpp>
#include "code/octree/octree.h"
#include "code/emitterStruct.h"
#include "code/rope.h"
#include <CURL/curl.h>
#include "code/octree/RTree.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

typedef RTree<brick*,double,3> brickPointerTree;

//#define bodyUserIndex_brick 55
//#define bodyUserIndex_builtCar 66
//#define bodyUserIndex_item 77
#define bodyUserIndex_plane 88
//#define bodyUserIndex_dynamic 99

//#define maxMusicBricks 32
#define brickTreeSize 32768

extern "C" {
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

#define hardCodedNetworkVersion 10015

struct clientData
{
    //0 if a guest or if we already logged in successfully:
    CURL *authHandle = 0;
    int logInState = 0; //0 = guest, 1 = waiting for master server auth, 2 = logged in totally, 3 = log in failed, 4 = good log in needs processing
    int accountID = 0;  //0 = guest

    int totalLoadedWheels = 0;
    int carWheelsMadeSoFar = 0;

    //Client physics:
    float interpolationStartTime = 0;
    float lastInterpolation = 0;
    btVector3 interpolationOffset;

    //Why would I ever store these here
    emitter *leftJet = 0;
    emitter *rightJet = 0;
    emitter *paint = 0;
    emitter *adminOrb = 0;
    bool canJet = true;
    bool currentlyTyping = false;

    std::vector<btVector3> wantedNodeColors;
    std::vector<std::string> wantedNodeNames;
    int wantedDecal = 0;

    std::vector<btVector3> debugColors;
    std::vector<btVector3> debugPositions;
    std::vector<brick*> ownedBricks;

    float preferredRed = 1,preferredGreen = 1,preferredBlue = 1;
    bool hasLuaAccess = false;
    std::vector<int> undoList;

    unsigned int playerID;
    std::string name = "Unnamed";

    brick *lastWrenchedBrick = 0;

    serverClientHandle *netRef = 0;

    dynamic *controlling = 0;
    bool prohibitTurning = false;

    int receivedBrickTypes = 0;
    int receivedDynamicTypes = 0;

    dynamic *cameraTarget = 0;
    bool cameraAdminOrb = false;
    bool cameraBoundToDynamic = false;
    bool cameraFreelookEnabled = true;
    float camX,camY,camZ;
    float camDirX,camDirY,camDirZ;

    bool needsCameraUpdate = false;

    brickCar *driving = 0;

    std::vector<light*> loadingCarLights;
    brickCar *loadingCar = 0;
    bool loadedCarWheels = false;
    bool loadedCarLights = false;
    bool loadCarAsCar = false;
    bool loadedCarAmountsPacket = false;
    int basicBricksLeftToLoad = 0;
    int specialBricksLeftToLoad = 0;
    btVector3 loadedCarOrigin = btVector3(0,0,0);
    unsigned int lastAttemptedCarPlant = 0;

    //Just used for deleting partial car load attempts
    unsigned int carLoadStartTime = 0;

    unsigned int lastChatMessage = 0;

    void sendCameraDetails();
    void bottomPrint(std::string text,int ms);
    void message(std::string text,std::string category = "generic");

    int lastPingSentAtMS = 0;
    int lastPingRecvAtMS = 0;
    int lastPingID = 0;
    bool waitingOnPing = false;
    void sendPing();
    void setControlling(dynamic *player);
    void forceTransformUpdate();
};

struct brickNameVector
{
    std::string name = "";
    std::vector<brick*> bricks;
};

struct schedule
{
    unsigned int timeToExecute = 0;
    unsigned int scheduleID = 0;
    std::string functionName = "";
    int numLuaArgs = 0;
    //std::string optionalString = "";
};

struct eventListener
{
    std::string eventName = "";
    std::vector<std::string> functionNames;
    eventListener(std::string name) : eventName(name) {}
};

struct dayNightCycle
{
    //The following are times, -1 = midnight, 1 = also midnight:
    double dawnStart = -0.8;
    double dawnEnd = -0.4;
    double duskStart = 0.4;
    double duskEnd = 0.8;
    double secondsInDay = 1000;

    //There are 4 colors to interpolate between for night, dawn, day, and dusk
    //I'd use an enum, but really we only use them in their interpolated form so why bother
    glm::vec3 dncSkyColors[4];
    glm::vec3 dncFogColors[4];
    glm::vec4 dncSunColors[4];

    bool useDNC = true;
    std::string skyBoxPath = "road/main.hdr";
    std::string radiancePath = "road/mainRad.hdr";
    std::string irradiancePath = "road/mainIrr.hdr";

    //time = 0.0;
    //+/-1 = midnight
    //-0.5 = dawn
    //0 = noon
    //0.5 = dusk
    //Actually it's like 0.6 is dusk and -0.6 is dawn for the given lat and decl

    dayNightCycle()
    {
        dncSunColors[0]        = glm::vec4(0.06,0.06,0.1,0);
        dncFogColors[0]        = glm::vec3(0.06,0.06,0.1);
        dncSkyColors[0]        = glm::vec3(1.0,1.0,1.0);

        dncSunColors[1]        = glm::vec4(235,116,26,255) / glm::vec4(256,256,256,255);
        dncFogColors[1]        = glm::vec3(21.0/255.0,26.0/255.0,29.0/255.0);
        dncSkyColors[1]        = glm::vec3(1.0  ,0.77 ,0.541);

        dncSunColors[2]        = glm::vec4(0.7,0.6,0.5,1.0/30.0) * glm::vec4(30.0);
        dncFogColors[2]        = glm::vec3(109.0/255.0,130.0/255.0,132.0/255.0);
        dncSkyColors[2]        = glm::vec3(1.0,1.0,1.0);

        dncSunColors[3]        = (glm::vec4(235,116,26,255) / glm::vec4(256,256,256,255)) * glm::vec4(1.2,1.2,1.2,1.0);
        dncFogColors[3]        = glm::vec3(74.0/255.0  ,71.0/255.0 ,56.0/255.0);
        dncSkyColors[3]        = glm::vec3(1.0  ,0.77 ,0.541);
    }

    enum dayNightCycleVar
    {
        none,useibl,dwS,dwE,dkS,dkE,secsDay,skyCol,fogCol,sunCol,skyBox,rad,irr
    };

    bool strToBool(std::string in)
    {
        if(in == "yes")
            return true;
        if(in == "true")
            return true;
        if(in == "1")
            return true;
        if(in == "t")
            return true;
        return false;
    }

    void loadFromFile()
    {
        dayNightCycleVar next = none;

        int idx = 0;

        std::ifstream envSet("environment.txt");
        if(envSet.is_open())
        {
            std::string line = "";
            while(!envSet.eof())
            {
                getline(envSet,line);

                if(line.length() == 0)
                    continue;

                if(line[0] == '#')
                    continue;

                std::string lc = lowercase(line);
                replaceAll(lc," ","");
                replaceAll(lc,"\t","");

                if(lc.length() < 1)
                    continue;

                if(lc == "useibl")
                    next = useibl;
                else if(lc == "skybox")
                    next = skyBox;
                else if(lc == "radiance")
                    next = rad;
                else if(lc == "irradiance")
                    next = irr;

                else if(lc == "dawnstart")
                    next = dwS;
                else if(lc == "dawnend")
                    next = dwE;
                else if(lc == "duskstart")
                    next = dkS;
                else if(lc == "duskend")
                    next = dkE;
                else if(lc == "secondsinday")
                    next = secsDay;

                else if(lc == "skycolors")
                {
                    next = skyCol;
                    idx = 0;
                }
                else if(lc == "suncolors")
                {
                    next = sunCol;
                    idx = 0;
                }
                else if(lc == "fogcolors")
                {
                    next = fogCol;
                    idx = 0;
                }

                else
                {
                    switch(next)
                    {
                        case useibl: useDNC = !strToBool(line); next=none; break;
                        case skyBox: skyBoxPath = line; next=none;  break;
                        case irr: irradiancePath = line; next=none;  break;
                        case rad: radiancePath = line; next=none;  break;
                        case dwS: dawnStart = atof(line.c_str()); next=none;  break;
                        case dwE: dawnEnd = atof(line.c_str()); next=none;  break;
                        case dkS: duskStart = atof(line.c_str()); next=none;  break;
                        case dkE: duskEnd = atof(line.c_str()); next=none;  break;
                        case secsDay: secondsInDay = atof(line.c_str()); next=none;  break;
                        case skyCol: dncSkyColors[idx/3][idx%3] = atof(line.c_str()); idx++; if(idx >= 12){next=none;} break;
                        case fogCol: dncFogColors[idx/3][idx%3] = atof(line.c_str()); idx++; if(idx >= 12){next=none;} break;
                        case sunCol: dncSunColors[idx/4][idx%4] = atof(line.c_str()); idx++; if(idx >= 16){next=none;} break;
                        case none: error("Malformed environment.txt file, line: " + line); break;
                    }
                }
            }

            envSet.close();
        }
        else
            error("Could not open environment.txt");
    }
};

enum fileType
{
    unknownFile = 0,
    audioFile = 1,
    brickFile = 2,
    textureFile = 3,
    modelFile = 4
};

fileType discernExtension(std::string extension);

struct fileDescriptor
{
    int id = -1;
    std::string path = "";
    fileType type = unknownFile;
    int sizeBytes = -1;
    std::string name = "";
    unsigned int checksum = 0;
};

struct unifiedWorld
{
    int nextFileID = 0;
    std::vector<fileDescriptor> customFiles;

    dayNightCycle cycle;
    glm::vec3 sunDirection = glm::vec3(0.540455,0.742971,0.394844);

    void sendEnvironmentToClient(clientData *client);

    unsigned int maxPlayers = 30;
    bool mature = false;
    std::string serverName = "My Cool Server";
    CURLM *curlHandle = 0;

    std::vector<std::string> badWords;
    std::vector<std::string> replacementWords;

    bool hasNonoWord(std::string in)
    {
        std::string lower = lowercase(in);
        for(int a = 0; a<badWords.size(); a++)
        {
            if(lower.find(badWords[a]) != std::string::npos)
                return true;
        }
        return false;
    }

    std::string replaceNonoWords(std::string in)
    {
        std::string lower = lowercase(in);
        for(int a = 0; a<badWords.size(); a++)
            replaceAll(lower,badWords[a],replacementWords[a]);
        return lower;
    }

    float waterLevel = 15.0;

    std::vector<light*> lights;
    std::vector<emitter*> emitters;
    std::vector<particleType*> particleTypes;
    std::vector<emitterType*> emitterTypes;
    emitterType *getEmitterType(std::string name);

    void removeLight(light *l);
    light *addLight(btVector3 color,brick *attachedBrick,bool forgoSending = false);
    light *addLight(btVector3 color,btVector3 position,bool forgoSending = false);
    light *addLight(btVector3 color,brickCar *attachedCar,btVector3 offset,bool forgoSending = false);

    void removeEmitter(emitter *e);
    emitter *addEmitter(emitterType *type,float x=0,float y=0,float z=0,bool dontSend = false);

    bool runningSchedules = false;
    unsigned int lastScheduleID = 1;
    std::vector<schedule> schedules;
    std::vector<schedule> tmpSchedules;
    std::vector<unsigned int> tmpScheduleIDsToDelete;

    std::vector<eventListener> events;
    void callEvent(std::string eventName);

    lua_State* luaState = 0;
    std::string luaPassword = "changeme";
    bool useLuaPassword = false;

    bool useRelativeWalkingSpeed = false;

    unsigned int lastEmitterID = 0;
    unsigned int lastSnapshotID = 0;
    unsigned int lastPlayerID = 0;
    unsigned int lastDynamicID = 0;
    unsigned int lastBrickID = 0;
    unsigned int lastBuiltVehicleID = 0;
    unsigned int lastRopeID = 0;
    unsigned int lastLightID = 0;

    item* addItem(itemType *type);
    item* addItem(itemType *type,float x,float y,float z);
    dynamic* addDynamic(int typeID,float red = 1,float green = 1,float blue = 1);
    void removeItem(item *toRemove);
    void removeDynamic(dynamic *toRemove,bool dontSendPacket = false);

    std::vector<itemType*> itemTypes;
    std::vector<dynamicType*> dynamicTypes;
    brickLoader *brickTypes = 0;

    //btDynamicsWorld *physicsWorld = 0;
    btDynamicsWorld *physicsWorld = 0;
    btSoftBodyWorldInfo softBodyWorldInfo;
    server *theServer = 0;

    std::vector<std::string> soundScriptNames;
    std::vector<std::string> soundFilePaths;
    std::vector<bool> soundIsMusic;
    void addMusicType(std::string scriptName,std::string filePath);
    void addSoundType(std::string scriptName,std::string filePath);

    void playSound(std::string scriptName,float x,float y,float z,float pitch = 1.0,float vol = 1.0);
    void playSoundExcept(std::string scriptName,float x,float y,float z,clientData *except,float pitch = 1.0,float vol = 1.0);
    void playSound(int soundId,float x,float y,float z,float pitch = 1.0,float vol = 1.0);
    //void loopSound(int songId,brickCar *mount,int loopId,float pitch = 1.0);
    //void loopSound(std::string scriptName,brickCar *mount,int loopId);
    void playSound(std::string scriptName,float pitch = 1.0,float vol = 1.0);
    void playSound(std::string scriptName,float pitch,float vol,clientData *target);

    std::vector<std::string> decalFilePaths;
    void loadDecals(std::string searchFolder);
    void sendDecals(serverClientHandle *target);

    //std::vector<brick*> musicBricks;
    //brick *musicBricks[maxMusicBricks];
    //void setMusic(brick *theBrick,int music);

    //loopID,soundID,carID,pitch
    std::vector<std::tuple<int,int,int,float>> carMusicLoops;
    //loopID,soundID,worldPos,pitch
    std::vector<std::tuple<int,int,float,float,float,float>> worldMusicLoops;
    int currentMusicLoopID = 0;
    //Returns loop ID:
    int startSoundLoop(int soundID,float x,float y,float z,float pitch);
    int startSoundLoop(int soundID,brickCar *mount,float pitch);
    void stopSoundLoop(int loopID);

    std::vector<dynamic*> projectiles;
    std::vector<item*> items;
    std::vector<clientData*> users;
    std::vector<dynamic*> dynamics;
    std::vector<brick*> bricksAddedThisFrame;
    std::vector<brick*> bricks;
    std::vector<brickNameVector> namedBricks;
    //Octree<brick*> *tree = 0;
    brickPointerTree *overlapTree = 0;
    std::vector<brickCar*> brickCars;
    std::vector<vec4> colorSet;
    std::vector<brick*> recentlyUpdatedBricks;
    std::vector<rope*> ropes;

    rope *addRope(btRigidBody *a,btRigidBody *b,bool useCenterPos = true,btVector3 posA = btVector3(0,0,0),btVector3 posB = btVector3(0,0,0));
    void removeRope(rope *toRemove,bool removeFromVector = true);

    void setShapeName(dynamic *toSet,std::string text,float r,float g,float b);

    void setBrickColor(brick *theBrick,float r,float g,float b,float a);
    void setBrickPosition(brick *theBrick,int x,int y,int z);
    void setBrickMaterial(brick *theBrick,int material);
    void setBrickAngleID(brick *theBrick,int angleID);

    void clearBricks(clientData *source);
    void removeBrick(brick *theBrick);
    void setBrickName(brick *theBrick,std::string name);
    bool addBrick(brick *theBrick,bool stopOverlaps = false,bool colliding = true,bool networking = true);
    void loadBlocklandSave(std::string filePath,std::vector<brick*> &loadedBricks);
    void loadLodSave(std::string filePath,std::vector<brick*> &loadedBricks,int xOffset,int yOffset,int zOffset);

    void dropItem(dynamic *holder,int slot,bool tryUpdateInventory = true);
    void pickUpItem(dynamic *holder,item *i,int slot,clientData *source=0);
    void spawnPlayer(clientData *source,float x,float y,float z);
    void applyAppearancePrefs(clientData *client,dynamic *player);

    itemType *getItemType(std::string name)
    {
        for(unsigned int i = 0; i<itemTypes.size(); i++)
        {
            if(itemTypes[i]->uiName == name)
                return itemTypes[i];
        }
        return 0;
    }

    void radiusImpulse(btVector3 pos,float radius,float power,bool doDamage);

    //Returns false if wheels have different rotations
    bool compileBrickCar(brickCar *toAdd,float &heightCorrection,bool wheelsAlready = false,btVector3 origin = btVector3(0,0,0));
    void removeBrickCar(brickCar *toRemove);

    void messageAll(std::string text,std::string category = "generic");

    unsigned int respawnTimeMS = 5000;
    std::vector<clientData*> queuedRespawn;
    std::vector<unsigned int> queuedRespawnTime;
    unsigned int oofCounter = 1;
    void applyDamage(dynamic *d,float damage,std::string cause="");
};


#endif // UNIFIEDWORLD_H_INCLUDED










