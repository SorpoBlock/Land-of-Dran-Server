#ifndef LIVINGBRICK_H_INCLUDED
#define LIVINGBRICK_H_INCLUDED

#include "code/brick.h"
#include "code/player.h"

#define maxCarBricks 10000

struct wheelData
{
    int loadOrder = 0;
    bool dirtEmitterOn = false;

    btVector3 offset;
    btVector3 scale;
    btVector3 wheelAxleCS;

    float breakForce = 400;
    float steerAngle = 0.5;
    float engineForce = 200;
    float suspensionLength = 0.7;
    float suspensionStiffness = 100;
    float dampingCompression = 6;
    float dampingRelaxation = 10;
    float frictionSlip = 1.2;
    float rollInfluence = 0.6;
    int brickTypeID = 0;

    /*std::string oldBrickDatablockName;
    int brickX,brickY,brickZ;
    bool brickHalfX,brickHalfY,brickHalfZ;
    unsigned int angleID;*/
    float carX,carY,carZ;
};

struct vehicleSettings
{
    bool realisticCenterOfMass = false;
    float angularDamping = 0.03;
    float mass = 1.5;
};

struct wheelBrick : brick
{
    wheelData wheelSettings;
    int loadOrder = 0;
};

struct steeringBrick : brick
{
    vehicleSettings steeringSettings;
};

struct brickCar
{
    bool doTireEmitter = false;

    std::vector<brick*> bricks;
    std::vector<light*> lights;

    int serverID = 0;
    int ownerID = 0;
    btVehicleRaycaster* vehicleRayCaster = 0;
    btRaycastVehicle *vehicle = 0;
    btRaycastVehicle::btVehicleTuning tuning;
    btVector3 halfExtents;

    std::vector<wheelData> wheels;

    btVector3 wheelPosition;
    bool builtBackwards = false;
    bool occupied = false;

    dynamic *body = 0;
    bool compiled = false;
    float mass = 1.0;

    void addToPacket(packet *data);
    unsigned int getPacketBits();
    void addWheels(btVector3 halfExtents);
    void sendBricksToClient(serverClientHandle *user);
    bool driveCar(bool forward,bool backward,bool left,bool right,bool brake);

    ~brickCar();
};

#endif // LIVINGBRICK_H_INCLUDED
