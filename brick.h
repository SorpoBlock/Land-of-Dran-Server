#ifndef BRICK_H_INCLUDED
#define BRICK_H_INCLUDED

#include "code/base/server.h"
#include <sstream>
#include "code/base/logger.h"
#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include "octree/octree.h"
#include <filesystem>

using namespace syj;
using namespace std::filesystem;

#define bodyUserIndex_brick 55
#define bodyUserIndex_builtCar 66

#define landOfDranBuildMagic 16483534

enum faceDirection{FACE_UP=0,FACE_DOWN=1,FACE_NORTH=2,FACE_SOUTH=3,FACE_EAST=4,FACE_WEST=5,FACE_OMNI=6};
enum brickMaterial{none=0,tempBrickEffect=1,undulo=1000,bob=2000,peral=2,chrome=3,glow=4,blink=5,swirl=6,rainbow=7,slippery=8};

struct vec4
{
    float r,g,b,a;
};

struct printAlias
{
    int width = -1,length = -1,height = -1;
    std::string uiName = "";
    std::string fileName = "";
    int faceMask = 0;
    void addFace(faceDirection toAdd)
    {
        faceMask |= 1 << toAdd;
    }
};

struct light;

struct brickType
{
    btBoxShape *shape = 0;
    int width=0,height=0,length=0;
    std::string uiname = "";
    std::string fileName = "";
    bool special = false;

    bool isModTerrain = false;
    bool isWheelType = false;
    //btBvhTriangleMeshShape *modTerShape = 0;
    btConvexHullShape *modTerShape = 0;

    void initModTerrain(std::string blbFile);

    void init(int w,int h,int l,Octree<btBoxShape*> *shapes)
    {
        if(shape)
            return;

        btBoxShape *tmp = shapes->at(w,h,l);
        if(tmp)
            shape = tmp;
        else
        {
            shape = new btBoxShape(btVector3(((float)w)/2.0,((float)h)/(2.0*2.5),((float)l)/2.0));
            shapes->set(w,h,l,shape);
        }

        width = w;
        height = h;
        length = l;
    }
};

struct brick
{
    light *attachedLight = 0;

    std::string name = "";
    int builtBy = -1;
    int serverID;

    bool collides = true;

    const bool isColliding()
    {
        return collides;
    }

    void setColliding(btDynamicsWorld *world,bool colliding)
    {
        if(!body)
            return;

        collides = colliding;
        if(colliding)
        {
            world->removeRigidBody(body);
            world->addRigidBody(body);
        }
        else
        {
            world->removeRigidBody(body);
            world->addRigidBody(body,int(btBroadphaseProxy::SensorTrigger),int(btBroadphaseProxy::SensorTrigger));
        }
    }

    //For all bricks:
    int material;

    short posX,posZ;
    bool xHalfPosition;
    bool zHalfPosition;
    unsigned short uPosY;
    bool yHalfPosition;

    float getX() { return ((float)posX) + (xHalfPosition ? 0.5 : 0.0); }
    float getY() { return (((float)uPosY) + (yHalfPosition ? 0.5 : 0.0)) * 0.4; }
    float getZ() { return ((float)posZ) + (zHalfPosition ? 0.5 : 0.0); }

    float carX,carY,carZ;
    unsigned int carPlatesUp = 0;

    int angleID;
    float r,g,b,a;

    //For special bricks:
    bool isSpecial = false;
    unsigned int typeID;

    bool isRendering = true;
    bool isModTerrain = false;
    bool isSteeringWheel = false;
    bool isWheel = false;
    void *car = 0;

    //For basic bricks:
    int width,height,length;

    //For bricks with prints:
    int printMask = 0;
    int printID = -1;
    //TODO: Remove this and send print ID
    std::string printName = "";

    int musicLoopId = -1;
    int music = 0;
    float musicPitch = 1.0;

    btRigidBody *body = 0;

    unsigned int getPacketBits(bool fullPos = false);
    void addToPacket(packet *data,bool fullPos = false);
    void createUpdatePacket(packet *data);

    void calcGridFromCarPos(btVector3 carOrigin);
};

void addBrickPacketsFromVector(std::vector<brick*> toAdd,std::vector<packet*> &result);
void removeBrickPacketsFromVector(std::vector<brick*> toRemove,std::vector<packet*> &result);

struct brickLoader
{
    std::vector<std::string> printNames;
    std::vector<std::string> printFilePaths;

    float rotations[4][4] = {
    0,0,1,0,
    4.7122,0,1,0,
    3.1415,0,1,0,
    1.5708,0,1,0};
    Octree<btBoxShape*> *collisionShapes = 0;
    std::vector<printAlias*>    printTypes;
    std::vector<brickType*>     brickTypes;
    int specialBrickTypes = 0;

    void addPhysicsToBrick(brick *toAdd,btDynamicsWorld *world);
    void sendTypesToClient(serverClientHandle *target);

    brickLoader(std::string typesFolder,std::string printsFolder);
};

#endif // BRICK_H_INCLUDED
