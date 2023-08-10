#ifndef EMITTERSTRUCT_H_INCLUDED
#define EMITTERSTRUCT_H_INCLUDED

#include "code/base/server.h"
#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include "code/brick.h"
#include "code/dynamic.h"

using namespace syj;

struct particleType
{
    unsigned int serverID = 0;
    std::string filePath = "";
    btVector4 colors[4] = {btVector4(1,1,1,1),btVector4(1,1,1,1),btVector4(1,1,1,1),btVector4(1,1,1,1)};
    btVector3 drag = btVector3(0.1,0.1,0.1);
    btVector3 gravity = btVector3(0.01,0.1,0.01);
    float inheritedVelFactor = 0.0;
    float lifetimeMS = 1000.0;
    float lifetimeVarianceMS = 0.0;
    float sizes[4] = {1,1,1,1};
    float times[4] = {0,0.25,0.75,1};
    float spinSpeed = 0.0;
    bool useInvAlpha = false;
    bool needsSorting = false;
    std::string dbname = "";

    void sendToClient(serverClientHandle *netRef);
};

struct emitterType
{
    unsigned int serverID = 0;
    unsigned int lifetimeMS = 0;
    float ejectionOffset = 0;
    float ejectionPeriodMS = 200;
    float ejectionVelocity = 1.0;
    float periodVarianceMS = 0.0;
    float phiReferenceVel = 0.0;
    float phiVariance = 0.0;
    float thetaMin = 90;
    float thetaMax = 90;
    float velocityVariance = 0.0;
    std::string uiName = "";
    std::string dbname = "";
    std::vector<particleType*> particleTypes;

    void sendToClient(serverClientHandle *netRef);
};

struct emitter
{
    unsigned int creationTime = 0;
    bool isJetEmitter = false;

    unsigned int serverID = 0;
    btVector3 position;
    emitterType *type = 0;
    brick *attachedToBrick = 0;
    dynamic *attachedToModel = 0;
    btVector3 color = btVector3(1,1,1);
    bool shootFromHand = false;
    std::string nodeName = "";

    void sendToClient(serverClientHandle *netRef);
};

#endif // EMITTERSTRUCT_H_INCLUDED
