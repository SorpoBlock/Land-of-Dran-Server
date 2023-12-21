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
#include "code/lua/itemLua.h"
#include <bearssl/bearssl_hash.h>
#include <conio.h>

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

std::string GetHexRepresentation(const unsigned char *Bytes, size_t Length) {
    std::ostringstream os;
    os.fill('0');
    os<<std::hex;
    for(const unsigned char *ptr = Bytes; ptr < Bytes+Length; ++ptr) {
        os<<std::setw(2)<<(unsigned int)*ptr;
    }
    return os.str();
}

bool loginOkay = false;
std::string username = "";
std::string key = "";
std::string keyID = "";

size_t getHeartbeatResponse(void *buffer,size_t size,size_t nmemb,void *userp)
{
    /*if(nmemb > 0)
    {
        std::string response;
        response.assign((char*)buffer,nmemb);
        info(response);
    }*/
    return nmemb;
}

size_t getLoginResponse(void *buffer,size_t size,size_t nmemb,void *userp)
{
    if(nmemb > 0)
    {
        std::string response;
        response.assign((char*)buffer,nmemb);

        std::vector<std::string> words;
        std::stringstream ss(response);
        std::string word = "";
        while(ss >> word)
            words.push_back(word);

        if(words.size() < 1)
        {
            error("Login server bad response: " + response);
            return nmemb;
        }

        if(words[0] == "NOACCOUNT")
        {
            error("No account by that name. Register at https://dran.land");
            return nmemb;
        }
        else if(words[0] == "BAD")
        {
            error("Incorrect password!");
            return nmemb;
        }
        else if(words[0] == "GOOD")
        {
            if(words.size() < 3)
            {
                error("Login server bad response: " + response);
                return nmemb;
            }

            info("Welcome, " + username + ".");
            keyID = words[1];
            key = words[2];
            loginOkay = true;
        }
        else
            error("Login server bad response: " + response);
    }
    return nmemb;
}

bool MySearchCallback(brick *value){ return false; }

bool silent = false;

int main(int argc, char *argv[])
{
    //silent = true;

    int maxPlayersThisTime = 0;

    common_lua = &common;
    /*common.tree = new Octree<brick*>(brickTreeSize*2,0);
    common.tree->setEmptyValue(0);*/
    common.overlapTree = new brickPointerTree;

    for(int a = 0; a<64; a++)
        common.colorSet.push_back({1,0,0,1});

    logger::setErrorFile("logs/error.txt");
    logger::setInfoFile("logs/log.txt");
    logger::setDebug(false);
    logger::setCallback(updateClientConsoles);

    info("Starting application...");

    if(argc > 0)
    {
        info(argv[0]);
        if(argc > 1)
        {
            if(std::string(argv[1]) == std::string("-silent"))
            {
                info("Silent mode activated.");
                silent = true;
            }
        }
    }

    preferenceFile settingsFile;
    settingsFile.importFromFile("config.txt");

    settingsFile.addStringPreference("SERVERNAME","My Server!");
    settingsFile.addBoolPreference("MATURE",false);
    settingsFile.addBoolPreference("USEEVALPASSWORD",false);
    settingsFile.addStringPreference("EVALPASSWORD","changeme");

    std::ifstream nonowords("assets/nonowords.txt");
    if(nonowords.is_open())
    {
        std::string line = "";

        while(!nonowords.eof())
        {
            getline(nonowords,line);

            if(line.length() < 2)
                continue;

            if(line.find("\t") != std::string::npos)
            {
                int tabPos = line.find("\t");
                std::string word = line.substr(0,tabPos);
                std::string replacement = line.substr(tabPos+1,line.length() - (tabPos+1));
                if(word.length() < 1)
                    continue;

                common.badWords.push_back(word);
                common.replacementWords.push_back(replacement);
            }
            else
            {
                common.badWords.push_back(line);
                common.replacementWords.push_back("****");
            }
        }

        nonowords.close();
    }
    else
        error("No assets/nonowords.txt found!");

    common.mature = settingsFile.getPreference("MATURE")->toBool();
    std::string tmpName = settingsFile.getPreference("SERVERNAME")->toString();
    common.serverName = "";

    for(int a = 0; a<tmpName.length(); a++)
    {
        if(tmpName[a] >= 'a' && tmpName[a] <= 'z')
            common.serverName = common.serverName + tmpName[a];
        else if(tmpName[a] >= 'A' && tmpName[a] <= 'Z')
            common.serverName = common.serverName + tmpName[a];
        else if(tmpName[a] >= '0' && tmpName[a] <= '9')
            common.serverName = common.serverName + tmpName[a];
        else if(tmpName[a] == ' ' || tmpName[a] == '-' || tmpName[a] == '_')
            common.serverName = common.serverName + tmpName[a];
    }

    common.useLuaPassword = settingsFile.getPreference("USEEVALPASSWORD")->toBool();
    common.luaPassword = settingsFile.getPreference("EVALPASSWORD")->toString();
    if(common.luaPassword == " " || common.luaPassword == "changeme" || common.luaPassword.length() < 1)
        common.useLuaPassword = false;

    unsigned char hash[32];
    br_sha256_context shaContext;
    br_sha256_init(&shaContext);
    br_sha256_update(&shaContext,common.luaPassword.c_str(),common.luaPassword.length());
    br_sha256_out(&shaContext,hash);
    std::string hexStr = GetHexRepresentation(hash,32);
    common.luaPassword = hexStr;

    settingsFile.exportToFile("config.txt");

    if(!silent)
    {
        int loginInfoMagicNumber = 38398271;

        std::ifstream loginInfo("loginInfo.bin",std::ios::binary);
        if(loginInfo.is_open())
        {
            int magicNumber = 0;
            loginInfo.read((char*)&magicNumber,sizeof(int));
            if(magicNumber == loginInfoMagicNumber)
            {
                //Actually get username
                loginInfo.read((char*)&magicNumber,sizeof(int));
                if(magicNumber > 0 && magicNumber < 256)
                {
                    char nameBuf[256];

                    loginInfo.read(nameBuf,magicNumber);
                    nameBuf[magicNumber] = 0;
                    username = nameBuf;

                    //Get ID
                    loginInfo.read((char*)&magicNumber,sizeof(int));
                    if(magicNumber < 0 || magicNumber > 255)
                        error("Login info file existed but was invalid.");
                    else
                    {
                        loginInfo.read(nameBuf,magicNumber);
                        nameBuf[magicNumber] = 0;
                        keyID = nameBuf;

                        //Get key
                        loginInfo.read(nameBuf,40);
                        nameBuf[40] = 0;
                        key = nameBuf;
                    }
                }
                else
                    error("Login info file existed but was invalid.");
            }
            else
                error("Login info file existed but was invalid.");
            loginInfo.close();
        }

        if(username.length() < 1 || key.length() < 1 || keyID.length() < 1)
        {
            forceLogin:

            info("No hosting key detected, please login to create one.");
            info("Username: ");
            std::cin>>username;
            info("Password:");
            std::string password;

            bool keepGettingInput = true;
            while(keepGettingInput)
            {
                char a = getch();
                if(a == '\n' || a == '\r')
                    keepGettingInput = false;
                else
                    password += a;
            }

            CURL *curlHandle = curl_easy_init();
            curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER , 0);
            curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST , 0);
            curl_easy_setopt(curlHandle,CURLOPT_WRITEFUNCTION,getLoginResponse);
            std::string url = "https://dran.land/getHostingKey.php";
            std::string args = "pass=" + password + "&name=" + username;
            curl_easy_setopt(curlHandle,CURLOPT_URL,url.c_str());
            curl_easy_setopt(curlHandle,CURLOPT_POSTFIELDS,args.c_str());
            CURLcode res = curl_easy_perform(curlHandle);
            curl_easy_cleanup(curlHandle);

            if(!loginOkay)
                goto forceLogin;

            std::ofstream loginInfo("loginInfo.bin",std::ios::binary);
            if(loginInfo.is_open())
            {
                loginInfo.write((char*)&loginInfoMagicNumber,sizeof(int));

                int nameLength = username.length();
                loginInfo.write((char*)&nameLength,sizeof(int));
                loginInfo.write(username.c_str(),username.length());

                nameLength = keyID.length();
                loginInfo.write((char*)&nameLength,sizeof(int));
                loginInfo.write(keyID.c_str(),keyID.length());

                loginInfo.write(key.c_str(),40);

                loginInfo.close();
            }
        }
    }

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
    //TODO: This line was only added after radiusImpulse, does it decrease performance?
    common.physicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    btVector3 gravity = btVector3(0,-70,0);
    common.physicsWorld->setGravity(gravity);
    common.physicsWorld->setForceUpdateAllAabbs(false);
    common.softBodyWorldInfo.m_broadphase = broadphase;
    common.softBodyWorldInfo.m_dispatcher = dispatcher;
    common.softBodyWorldInfo.m_gravity = gravity;
    common.softBodyWorldInfo.m_sparsesdf.Initialize();

    common.cycle.loadFromFile();

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
    registerItemFunctions(common.luaState);
    addEventNames(common.luaState);

    lua_newtable(common.luaState);
    lua_setglobal(common.luaState,"scheduleArgs");

    info("Loading decals");
    common.loadDecals("add-ons/decals");

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
    hammerItem->iconPath = "assets/tools/icons/hammerIcon.png";
    common.itemTypes.push_back(hammerItem);

    dynamicType *wrench = new dynamicType("assets/tools/wrench.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    wrench->filePath = "assets/tools/wrench.txt";
    common.dynamicTypes.push_back(wrench);

    itemType *wrenchItem = new itemType(wrench,1);
    wrenchItem->uiName = "Wrench";
    wrenchItem->handOffsetX = 1.8;
    wrenchItem->handOffsetY = 3.5;
    wrenchItem->handOffsetZ = -2.0;
    wrenchItem->iconPath = "assets/tools/icons/wrenchIcon.png";
    common.itemTypes.push_back(wrenchItem);

    dynamicType *spraycan = new dynamicType("assets/tools/spraycan.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    spraycan->filePath = "assets/tools/spraycan.txt";
    common.dynamicTypes.push_back(spraycan);

    itemType *spraycanItem = new itemType(spraycan,2);
    spraycanItem->uiName = "Paint Can";
    spraycanItem->handOffsetX = 1.8;
    spraycanItem->handOffsetY = 2.8;
    spraycanItem->handOffsetZ = -2.0;
    spraycanItem->iconPath = "assets/tools/icons/paintCanIcon.png";
    common.itemTypes.push_back(spraycanItem);

    dynamicType *gun = new dynamicType("assets/gun/gun.txt",common.dynamicTypes.size(),btVector3(0.02,0.02,0.02));
    gun->standingFrame = 1;
    gun->filePath = "assets/gun/gun.txt";
    common.dynamicTypes.push_back(gun);

    itemType *gunItem = new itemType(gun,3);
    gunItem->uiName = "Launcher";
    gunItem->useDefaultSwing = false;
    gunItem->handOffsetX = 1.8;
    gunItem->handOffsetY = 3.3;
    gunItem->handOffsetZ = -1.0;
    gunItem->iconPath = "assets/gun/icon.png";
    common.itemTypes.push_back(gunItem);

    animation fireGun;
    fireGun.name = "fire";
    fireGun.startFrame = 1;
    fireGun.endFrame = 28;
    fireGun.speedDefault = 0.04;
    gun->animations.push_back(fireGun);

    dynamicType *shell = new dynamicType("assets/shell/shell.txt",common.dynamicTypes.size(),btVector3(0.015,0.015,0.015));
    shell->filePath = "assets/shell/shell.txt";
    common.dynamicTypes.push_back(shell);

    dynamicType *beretta = new dynamicType("assets/beretta/gun.txt",common.dynamicTypes.size(),btVector3(0.3,0.3,0.3));
    beretta->standingFrame = 85;
    beretta->filePath = "assets/beretta/gun.txt";
    common.dynamicTypes.push_back(beretta);

    itemType *berettaItem = new itemType(beretta,4);
    berettaItem->uiName = "Beretta";
    berettaItem->useDefaultSwing = false;
    berettaItem->handOffsetX = 1.8;
    berettaItem->handOffsetY = 3.8;
    berettaItem->handOffsetZ = -1.5;
    berettaItem->iconPath = "assets/beretta/icon.png";
    common.itemTypes.push_back(berettaItem);

    glm::quat berettaRot = glm::quat(glm::vec3(0,3.1415,0));
    berettaItem->rotW = berettaRot.w;
    berettaItem->rotX = berettaRot.x;
    berettaItem->rotY = berettaRot.y;
    berettaItem->rotZ = berettaRot.z;

    animation reloadBeretta;
    reloadBeretta.name = "reload";
    reloadBeretta.startFrame = 0;
    reloadBeretta.endFrame = 20;
    reloadBeretta.speedDefault = 0.04;
    beretta->animations.push_back(reloadBeretta);

    animation fireBeretta;
    fireBeretta.name = "fire";
    fireBeretta.startFrame = 42;
    fireBeretta.endFrame = 75;
    fireBeretta.speedDefault = 0.04;
    beretta->animations.push_back(fireBeretta);

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

    common.addSoundType("C5Bass","assets/sound/BassC5.wav");
    common.addSoundType("C5Brass","assets/sound/BrassC5.wav");
    common.addSoundType("C5Flute","assets/sound/FluteC5.wav");
    common.addSoundType("C5Guitar","assets/sound/GuitarC5.wav");
    common.addSoundType("C5Piano","assets/sound/PianoC5.wav");
    common.addSoundType("C5Saw","assets/sound/SawC5.wav");
    common.addSoundType("C5Square","assets/sound/SquareC5.wav");
    common.addSoundType("C5SteelDrum","assets/sound/SteelDrumC5.wav");
    common.addSoundType("C5String","assets/sound/StringC5.wav");
    common.addSoundType("C5Synth","assets/sound/SynthC5.wav");
    common.addSoundType("C5Timpani","assets/sound/TimpaniC5.wav");
    common.addSoundType("C5Vibraphone","assets/sound/VibraphoneC5.wav");

    common.addSoundType("DrumsetClap","assets/sound/Drumset/DrumsetClap.wav");
    common.addSoundType("DrumsetCymbal","assets/sound/Drumset/DrumsetCymbal.wav");
    common.addSoundType("DrumsetDrumSnare","assets/sound/Drumset/DrumsetDrumSnare.wav");
    common.addSoundType("DrumsetESnare","assets/sound/Drumset/DrumsetESnare.wav");
    common.addSoundType("DrumsetKick","assets/sound/Drumset/DrumsetKick.wav");
    common.addSoundType("DrumsetPedalC","assets/sound/Drumset/DrumsetPedalC.wav");
    common.addSoundType("DrumsetPowerSnare","assets/sound/Drumset/DrumsetPowerSnare.wav");
    common.addSoundType("DrumsetTom","assets/sound/Drumset/DrumsetTom.wav");

    common.addSoundType("Launch","assets/sound/Launch.wav");
    common.addSoundType("Explosion","assets/sound/665092__tgerginov__explosion.wav");

    common.addSoundType("oof1","assets/sound/pain/oof1.wav");
    common.addSoundType("oof2","assets/sound/pain/oof2.wav");
    common.addSoundType("oof3","assets/sound/pain/oof3.wav");
    common.addSoundType("oof4","assets/sound/pain/oof4.wav");

    common.addSoundType("berettaShot","assets/sound/berettaShot.wav");
    common.addSoundType("berettaReload","assets/sound/berettaReload.wav");

    preferenceFile addonsList;
    addonsList.importFromFile("add-ons/add-onsList.txt");

    info("Loading add-ons...");
    for (recursive_directory_iterator i("add-ons"), end; i != end; ++i)
    {
        if (!is_directory(i->path()))
        {
            if(i->path().filename() == "server.lua")
            {
                std::string addonName = i->path().parent_path().string();
                if(addonName.substr(0,8) == "add-ons\\")
                    addonName = addonName.substr(8,addonName.length() - 8);
                if(addonName.find("\\") != std::string::npos)
                {
                    error("Not loading " + i->path().string() + " because it is nested!");
                    continue;
                }
                if(common.luaState,i->path().string().length() < 1)
                    continue;

                if(addonsList.getPreference(addonName))
                {
                    if(!addonsList.getPreference(addonName)->toBool())
                    {
                        info(addonName + " disabled, skipping");
                        continue;
                    }
                }
                else
                {
                    info(addonName + " is new, enabling");
                    addonsList.addBoolPreference(addonName,true);
                }

                info("Loading " + addonName);
                if(luaL_dofile(common.luaState,i->path().string().c_str()))
                {
                    error("Lua error loading add-on " + addonName);
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

    addonsList.exportToFile("add-ons/add-onsList.txt");
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

    info("Starting content server...");

    IPaddress ip;
    if(SDLNet_ResolveHost(&ip,NULL,20001) == -1)
    {
        error("Error resolving content server internal IP");
        return 0;
    }

    TCPsocket cdnSocket = SDLNet_TCP_Open(&ip);
    if(!cdnSocket)
    {
        error("Error opening content server.");
        return 0;
    }

    std::vector<TCPsocket> cdnClients;
    std::vector<SDLNet_SocketSet> cdnClientSocketSets;
    std::vector<std::string> cdnClientResponseString;

    info("Start-up complete.");

    bool cont = true;
    float deltaT = 0;
    unsigned int lastTick = SDL_GetTicks();
    unsigned int lastFrameCheck = SDL_GetTicks();
    unsigned int frames = 0;

    unsigned int lastMasterListPost = 0;
    CURL *masterServerPoster = 0;

    if(!silent)
    {
        if(masterServerPoster)
        {
            curl_easy_cleanup(masterServerPoster);
            masterServerPoster = 0;
        }

        //info("Posting to master server...");
        masterServerPoster = curl_easy_init();
        curl_easy_setopt(masterServerPoster,CURLOPT_SSL_VERIFYHOST,0);
        curl_easy_setopt(masterServerPoster,CURLOPT_SSL_VERIFYPEER,0);
        //curl_easy_setopt(masterServerPoster,CURLOPT_WRITEDATA,source);
        curl_easy_setopt(masterServerPoster,CURLOPT_WRITEFUNCTION,getHeartbeatResponse);
        std::string url = "https://dran.land/heartbeat.php";
        std::string args = "key=" + key + "&id=" + keyID + "&players=" + std::to_string(common.users.size()) + "&maxplayers=30&mature=" + (common.mature?"1":"0") + "&servername=" + common.serverName + "&bricks=" + std::to_string(common.bricks.size());
        curl_easy_setopt(masterServerPoster,CURLOPT_URL,url.c_str());
        curl_easy_setopt(masterServerPoster,CURLOPT_POSTFIELDSIZE,args.length());
        curl_easy_setopt(masterServerPoster,CURLOPT_COPYPOSTFIELDS,args.c_str());
        CURLMcode code = curl_multi_add_handle(common.curlHandle,masterServerPoster);
        if(code != CURLM_OK)
            error(std::string("Posting to master server resulted in error ") + curl_multi_strerror(code));
    }

    //unsigned int msBetweenUpdates = 60;
    //bool everyOtherLoop = false;
    unsigned int msAtLastUpdate = 0;
    while(cont)
    {
        //Start CDN server code
        TCPsocket cdnClient = SDLNet_TCP_Accept(cdnSocket);
        if(cdnClient)
        {
            if(cdnClients.size() >= 10)
            {
                error("More than 10 simultaneous CDN connections, rejecting connection.");
                SDLNet_TCP_Close(cdnClient);
            }
            else
            {
                info("Accepted a cdn client.");
                SDLNet_SocketSet tmpSocketSet = SDLNet_AllocSocketSet(1);
                SDLNet_TCP_AddSocket(tmpSocketSet,cdnClient);
                cdnClientSocketSets.push_back(tmpSocketSet);
                cdnClients.push_back(cdnClient);
                cdnClientResponseString.push_back("");
            }
        }

        if(cdnClients.size() > 0)
        {
            for(int a = 0; a<cdnClients.size(); a++)
            {
                int checkSocketsRet = SDLNet_CheckSockets(cdnClientSocketSets[a],0);
                if(checkSocketsRet > 0)
                {
                    char *recvData = new char[1024];
                    int recvBytes = SDLNet_TCP_Recv(cdnClients[a],recvData,1024);
                    if(recvBytes <= 0)
                    {
                        info("CDN client disconnected!");
                        SDLNet_TCP_DelSocket(cdnClientSocketSets[a],cdnClients[a]);
                        SDLNet_FreeSocketSet(cdnClientSocketSets[a]);
                        cdnClientSocketSets[a] = 0;
                        cdnClientSocketSets.erase(cdnClientSocketSets.begin() + a);
                        SDLNet_TCP_Close(cdnClients[a]);
                        cdnClients[a] = 0;
                        cdnClients.erase(cdnClients.begin() + a);
                        cdnClientResponseString.erase(cdnClientResponseString.begin() + a);
                        delete recvData;
                        break;
                    }
                    else
                    {
                        cdnClientResponseString[a] += std::string(recvData).substr(0,recvBytes);
                        if(cdnClientResponseString[a].find("END") != std::string::npos)
                        {
                            std::stringstream s(cdnClientResponseString[a]);
                            std::string line = "";
                            int expectedFiles = -1;
                            bool breakOuterLoop = false;

                            while(getline(s,line,'\n'))
                            {
                                if(expectedFiles == -1)
                                {
                                    if(line.find("FILES") == std::string::npos)
                                    {
                                        error("CDN Request from client malformed!");
                                        SDLNet_TCP_DelSocket(cdnClientSocketSets[a],cdnClients[a]);
                                        SDLNet_FreeSocketSet(cdnClientSocketSets[a]);
                                        cdnClientSocketSets[a] = 0;
                                        cdnClientSocketSets.erase(cdnClientSocketSets.begin() + a);
                                        SDLNet_TCP_Close(cdnClients[a]);
                                        cdnClients[a] = 0;
                                        cdnClients.erase(cdnClients.begin() + a);
                                        cdnClientResponseString.erase(cdnClientResponseString.begin() + a);
                                        delete recvData;
                                        breakOuterLoop = true;
                                        break;
                                    }
                                    expectedFiles = atoi(line.substr(5,line.length()-5).c_str());
                                    info("New client requesting download of " + std::to_string(expectedFiles) + " files.");
                                    continue;
                                }

                                if(expectedFiles == 0)
                                {
                                    if(line != "END")
                                        error("End of CDN Request from client malformed!");

                                    SDLNet_TCP_DelSocket(cdnClientSocketSets[a],cdnClients[a]);
                                    SDLNet_FreeSocketSet(cdnClientSocketSets[a]);
                                    cdnClientSocketSets[a] = 0;
                                    cdnClientSocketSets.erase(cdnClientSocketSets.begin() + a);
                                    SDLNet_TCP_Close(cdnClients[a]);
                                    cdnClients[a] = 0;
                                    cdnClients.erase(cdnClients.begin() + a);
                                    cdnClientResponseString.erase(cdnClientResponseString.begin() + a);
                                    delete recvData;
                                    breakOuterLoop = true;
                                    break;
                                }

                                int fileID = atoi(line.c_str());
                                //std::cout<<"Sending file "<<fileID<<"\n";

                                if(fileID >= common.customFiles.size() || fileID < 0)
                                {
                                    error("Invalid custom file index from CDN client.");
                                    SDLNet_TCP_DelSocket(cdnClientSocketSets[a],cdnClients[a]);
                                    SDLNet_FreeSocketSet(cdnClientSocketSets[a]);
                                    cdnClientSocketSets[a] = 0;
                                    cdnClientSocketSets.erase(cdnClientSocketSets.begin() + a);
                                    SDLNet_TCP_Close(cdnClients[a]);
                                    cdnClients[a] = 0;
                                    cdnClients.erase(cdnClients.begin() + a);
                                    cdnClientResponseString.erase(cdnClientResponseString.begin() + a);
                                    delete recvData;
                                    breakOuterLoop = true;
                                    break;
                                }

                                std::ifstream cdnFileHandle(std::string("add-ons/"+common.customFiles[fileID].path).c_str(),std::ios::binary);

                                int bytesToSend = common.customFiles[fileID].sizeBytes;
                                int bytesSent = 0;
                                while(bytesToSend > 0)
                                {
                                    int bytesThisPacket = std::min(1024,bytesToSend);

                                    char *buf = new char[bytesThisPacket];
                                    cdnFileHandle.read(buf,bytesThisPacket);
                                    SDLNet_TCP_Send(cdnClients[a],buf,bytesThisPacket);
                                    delete buf;

                                    bytesToSend -= bytesThisPacket;
                                    bytesSent += bytesThisPacket;
                                }

                                cdnFileHandle.close();

                                expectedFiles--;
                            }
                            if(breakOuterLoop)
                                break;
                        }
                    }
                    delete recvData;
                }
                else if(checkSocketsRet < 0)
                    error("SDLNet_CheckSockets returned error");
            }
        }

        //End CDN server code

        if(!silent && (SDL_GetTicks() - lastMasterListPost > 60000*4))
        {
            lastMasterListPost = SDL_GetTicks();

            if(masterServerPoster)
            {
                curl_easy_cleanup(masterServerPoster);
                masterServerPoster = 0;
            }

            //info("Posting to master server...");
            masterServerPoster = curl_easy_init();
            curl_easy_setopt(masterServerPoster,CURLOPT_SSL_VERIFYHOST,0);
            curl_easy_setopt(masterServerPoster,CURLOPT_SSL_VERIFYPEER,0);
            //curl_easy_setopt(masterServerPoster,CURLOPT_WRITEDATA,source);
            curl_easy_setopt(masterServerPoster,CURLOPT_WRITEFUNCTION,getHeartbeatResponse);
            std::string url = "https://dran.land/heartbeat.php";
            std::string args = "key=" + key + "&id=" + keyID + "&players=" + std::to_string(common.users.size()) + "&maxplayers=30&mature=" + (common.mature?"1":"0") + "&servername=" + common.serverName + "&bricks=" + std::to_string(common.bricks.size());
            curl_easy_setopt(masterServerPoster,CURLOPT_URL,url.c_str());
            curl_easy_setopt(masterServerPoster,CURLOPT_POSTFIELDSIZE,args.length());
            curl_easy_setopt(masterServerPoster,CURLOPT_COPYPOSTFIELDS,args.c_str());
            CURLMcode code = curl_multi_add_handle(common.curlHandle,masterServerPoster);
            if(code != CURLM_OK)
                error(std::string("Posting to master server resulted in error ") + curl_multi_strerror(code));
        }

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

                clientJoin(source);
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
            std::cout<<fps<<"\n";
            //if(fps < 10000)
              //  info("Last FPS: " + std::to_string(fps));
            frames = 0;
            lastFrameCheck = SDL_GetTicks();
        }

        for(unsigned int a = 0; a<common.projectiles.size(); a++)
        {
            dynamic *proj = common.projectiles[a];
            if(!proj)
                continue;

            if(proj->getLinearVelocity().length() < 8)
                continue;

            btTransform t = proj->getWorldTransform();
            t.setOrigin(btVector3(0,0,0));
            btQuaternion oldQuat = t.getRotation();

            btVector3 currentUp = t * btVector3(0,1,0);
            btVector3 desiredUp = proj->getLinearVelocity();
            desiredUp = desiredUp.normalized();

            float dot = btDot(currentUp,desiredUp);
            if(dot > 0.99999 || dot < -0.99999)
                continue;

            btVector3 cross = btCross(currentUp,desiredUp);
            btQuaternion rot = btQuaternion(cross.x(),cross.y(),cross.z(),1+dot);
            rot = rot.normalized();

            t = proj->getWorldTransform();
            t.setRotation(rot * oldQuat);
            proj->setWorldTransform(t);
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
            float waterLevel = common.waterLevel;
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
                            common.playSound("Splash",pos.x(),common.waterLevel,pos.z());
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

        std::vector<dynamic*> projectilesToDeleteThatHitStuff;

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
                        int argsToPush = 0;

                        if((*sch).numLuaArgs > 0)
                        {
                            lua_getglobal(common.luaState,"scheduleArgs");
                            if(!lua_istable(common.luaState,-1))
                            {
                                error("Where did your scheduleArgs table go!");
                                lua_pop(common.luaState,1);
                            }
                            else
                            {
                                lua_pushinteger(common.luaState,(*sch).scheduleID);
                                lua_gettable(common.luaState,-2);

                                //func name
                                //scheduleArgs table
                                //our specific args table

                                if(lua_isnil(common.luaState,-1))
                                    lua_pop(common.luaState,2);
                                else
                                {
                                    for(int g = (*sch).numLuaArgs-1; g>=0; g--)
                                    {
                                        lua_pushinteger(common.luaState,g+1);
                                        lua_gettable(common.luaState,3);
                                    }

                                    //func name
                                    //scheduleArgs table
                                    //our specific args table
                                    //arg1...
                                    //arg2...

                                    lua_pushinteger(common.luaState,(*sch).scheduleID);
                                    lua_pushnil(common.luaState);

                                    lua_settable(common.luaState,2);

                                    lua_remove(common.luaState,3);
                                    lua_remove(common.luaState,2);

                                    argsToPush = (*sch).numLuaArgs;
                                }
                            }
                        }

                        if(lua_pcall(common.luaState,argsToPush,0,0) != 0)
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

            //Send over batched brick addition and removal packets:

            if(common.bricksAddedThisFrame.size() > 0)
            {
                std::vector<packet*> resultPackets;
                addBrickPacketsFromVector(common.bricksAddedThisFrame,resultPackets);
                for(int a = 0; a<resultPackets.size(); a++)
                {
                    common.theServer->send(resultPackets[a],true);
                    delete resultPackets[a];
                }

                common.bricksAddedThisFrame.clear();
            }

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

                if(obA == obB)
                    continue;

                if(!(obA->getUserIndex() == bodyUserIndex_plane || obB->getUserIndex() == bodyUserIndex_plane))
                {
                    if(obB->getUserIndex() == bodyUserIndex_dynamic)
                    {
                        dynamic *other = (dynamic*)obB->getUserPointer();
                        if(other->isProjectile)
                        {
                            bool projCont = false;
                            for(int x = 0; x<projectilesToDeleteThatHitStuff.size(); x++)
                            {
                                if(projectilesToDeleteThatHitStuff[x] == other)
                                {
                                    projCont = true;
                                    break;
                                }
                            }
                            if(projCont)
                                continue;

                            if(obA->getUserIndex() == bodyUserIndex_brick)
                            {
                                btVector3 o = other->getWorldTransform().getOrigin();
                                projectileHit((brick*)obA->getUserPointer(),o.x(),o.y(),o.z(),other->projectileTag);
                            }
                            else if(obA->getUserIndex() == bodyUserIndex_dynamic)
                            {
                                btVector3 o = other->getWorldTransform().getOrigin();
                                projectileHit((dynamic*)obA->getUserPointer(),o.x(),o.y(),o.z(),other->projectileTag);
                            }

                            projectilesToDeleteThatHitStuff.push_back(other);
                            continue;
                        }
                    }

                    if(obA->getUserIndex() == bodyUserIndex_dynamic)
                    {
                        dynamic *other = (dynamic*)obA->getUserPointer();
                        if(other->isProjectile)
                        {
                            bool projCont = false;
                            for(int x = 0; x<projectilesToDeleteThatHitStuff.size(); x++)
                            {
                                if(projectilesToDeleteThatHitStuff[x] == other)
                                {
                                    projCont = true;
                                    break;
                                }
                            }
                            if(projCont)
                                continue;

                            if(obB->getUserIndex() == bodyUserIndex_brick)
                            {
                                btVector3 o = other->getWorldTransform().getOrigin();
                                projectileHit((brick*)obB->getUserPointer(),o.x(),o.y(),o.z(),other->projectileTag);
                            }
                            else if(obB->getUserIndex() == bodyUserIndex_dynamic)
                            {
                                btVector3 o = other->getWorldTransform().getOrigin();
                                projectileHit((dynamic*)obB->getUserPointer(),o.x(),o.y(),o.z(),other->projectileTag);
                            }

                            projectilesToDeleteThatHitStuff.push_back(other);
                            continue;
                        }
                    }
                }
                /*else
                {
                    std::cout<<"Breaking: "<<contactManifold->getContactBreakingThreshold()<<"\n";
                    std::cout<<"Processing: "<<contactManifold->getContactProcessingThreshold()<<"\n";
                    std::cout<<"Contacts: "<<contactManifold->getNumContacts()<<"\n";
                    std::cout<<"Distance: "<<contactManifold->getContactPoint(0).getDistance()<<"\n";
                }*/

                if(!(obA->getUserIndex() == bodyUserIndex_builtCar || obB->getUserIndex() == bodyUserIndex_builtCar))
                    continue;

                if(obA->getUserIndex() == bodyUserIndex_plane || obB->getUserIndex() == bodyUserIndex_plane)
                    continue;

                //All of the following is just for the impact sound lmao:

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
                    common.playSound("FastImpact",pos.x(),pos.y(),pos.z());
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
                transformPacket.writeUInt(dynamicsSentThisPacket,8);

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

        for(int a = 0; a<projectilesToDeleteThatHitStuff.size(); a++)
            common.removeDynamic(projectilesToDeleteThatHitStuff[a]);
        projectilesToDeleteThatHitStuff.clear();

        for(int a = 0; a<common.queuedRespawn.size(); a++)
        {
            if(common.queuedRespawnTime[a] < SDL_GetTicks())
            {
                common.spawnPlayer(common.queuedRespawn[a],0,10,0);

                common.queuedRespawn.erase(common.queuedRespawn.begin() + a);
                common.queuedRespawnTime.erase(common.queuedRespawnTime.begin() + a);
                break;
            }
        }

        frames++;

        common.theServer->run();
    }

    SDLNet_TCP_Close(cdnSocket);

    return 0;
}









