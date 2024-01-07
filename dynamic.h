#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "code/base/server.h"
#include "code/base/logger.h"
#include "code/base/preference.h"
#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include "code/brick.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "code/quaternion.h"

#define bodyUserIndex_dynamic 99
#define inventorySize 3
#define dynamicObjectIDBits 12

using namespace syj;

struct animation
{
    int startFrame;
    int endFrame;
    float speedDefault = 0.04;
    std::string name;
};

struct dynamicType
{
    int dynamicTypeID = 0;
    std::string filePath;
    int standingFrame;
    std::vector<animation> animations;
    float eyeOffsetX,eyeOffsetY,eyeOffsetZ;
    btVector3 networkScale = btVector3(1,1,1);

    //Real values used in the physics engine and also sent to client:
    btVector3 finalHalfExtents;
    btVector3 finalOffset;

    btVector3 aabbMin;
    btVector3 aabbMax;
    btMotionState *defaultMotionState = 0;
    btCompoundShape *defaultCollisionShape = 0;
    float mass = 0;
    btVector3 defaultIntertia;

    dynamicType(std::string filePath,int id,btVector3 scale);

    void sendTypeToClient(serverClientHandle *client,int id);
};

struct item;

struct dynamic : btRigidBody
{
    float crouchProgress = 0.0;
    float health = 100.0;
    bool canTakeDamage = false;

    std::string projectileTag = "";
    bool isProjectile = false;

    bool lastInWater = false;
    unsigned int lastSplashEffect = 0;
    bool isJetting = false;

    unsigned int lastPlayerControl = 0;
    unsigned int lastWalked = 0;
    int typeID;
    int serverID;
    dynamicType *type;

    std::vector<std::string> nodeNames;
    std::vector<btVector3> nodeColors;
    int chosenDecal = 0;

    void setNodeColor(std::string nodeName,btVector3 nodeColor,server *broadcast);
    void nodeColorsPacket(packet *data);

    bool isPlayer = false;
    float lastCamX = 1;
    float lastCamY = 0;
    float lastCamZ = 0;
    int lastHeldSlot = 0;
    int pickUpItemFireCooldownStart = 0;
    int lastControlMask = 0;
    bool lastLeftMouseDown = false;

    float redTint = 1.0;
    float greenTint = 1.0;
    float blueTint = 1.0;
    float shapeNameR = 1.0;
    float shapeNameG = 1.0;
    float shapeNameB = 1.0;
    std::string shapeName = "";

    item *holding[inventorySize] = {0,0,0};

    btVector3 lastPosition = btVector3(0,0,0);
    btQuaternion lastRotation = btQuaternion(0,0,0);
    bool otherwiseChanged = false;
    unsigned int lastSentTransform = 0;

    bool walking = false;
    dynamic *sittingOn = 0;
    int oldCollisionFlags = -1;

    virtual void sendToClient(serverClientHandle *client);

    //Only changes physical properties, has nothing to do with networking:
    void makePlayer();

    //Returns true if player has just jumped
    bool control(float yaw,bool forward,bool backward,bool left,bool right,bool jump,bool crouch,btDynamicsWorld *world,bool allowTurning,bool relativeSpeed,std::vector<btVector3> &colors,std::vector<btVector3> &poses);

    virtual void addToPacket(packet *data);
    virtual unsigned int getPacketBits();
    dynamic(btScalar mass,btMotionState *ms,btCollisionShape *shape,btVector3 inertia,int idServer,btDynamicsWorld *world);
    dynamic(dynamicType *type,btDynamicsWorld *world,int idServer,int idType);
};

#endif // PLAYER_H_INCLUDED
