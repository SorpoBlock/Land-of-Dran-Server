#include "livingBrick.h"

int lastx = 0;

void brickCar::addToPacket(packet *data)
{
    data->writeUInt(serverID,10);

    //std::cout<<"Diff: "<<lastx-body->getWorldTransform().getOrigin().x()<<" Vel: "<<body->getLinearVelocity().x()<<"\n";
    lastx = body->getWorldTransform().getOrigin().x();

    data->writeFloat(body->getWorldTransform().getOrigin().x());
    data->writeFloat(body->getWorldTransform().getOrigin().y());
    data->writeFloat(body->getWorldTransform().getOrigin().z());
    writeQuat(body->getWorldTransform().getRotation(),data);

    data->writeUInt(wheels.size(),5);

    for(unsigned int wheel = 0; wheel<wheels.size(); wheel++)
    {
        vehicle->updateWheelTransform(wheel,false);
        btTransform t = vehicle->getWheelInfo(wheel).m_worldTransform;
        data->writeFloat(t.getOrigin().x());
        data->writeFloat(t.getOrigin().y());
        data->writeFloat(t.getOrigin().z());
        writeQuat(t.getRotation(),data);
        data->writeBit(wheels[wheel].dirtEmitterOn);
    }
}

unsigned int brickCar::getPacketBits()
{
    return 142 + wheels.size() * 126;
}

bool brickCar::driveCar(bool forward,bool backward,bool left,bool right,bool brake)
{
    doTireEmitter = brake || left || right;
    /*btVector3 vel = btVector3(0,0,0);

    if(left)
        vel.setZ(1);
    if(right)
        vel.setZ(-1);
    if(forward)
        vel.setX(1);
    if(backward)
        vel.setX(-1);

    vel = btMatrix3x3(body->getWorldTransform().getRotation()) * vel;

    body->setAngularVelocity(vel);
    body->setLinearVelocity(btVector3(0,0,0));

    return false;*/

    float steerMultiplier = 0;
    if(left)
        steerMultiplier = -1;
    if(right)
        steerMultiplier = 1;

    float engineMultiplier = 0;
    if(forward)
        engineMultiplier = 1;
    if(backward)
        engineMultiplier = -1;
    if(builtBackwards)
        engineMultiplier *= -1;

    for(unsigned int a = 0; a<wheels.size(); a++)
    {
        if(fabs(wheels[a].breakForce) > 0.01)
            vehicle->setBrake(brake ? wheels[a].breakForce : 0,a);
        if(fabs(wheels[a].steerAngle) > 0.01)
            vehicle->setSteeringValue(steerMultiplier * wheels[a].steerAngle,a);
        if(body->getLinearVelocity().length() < 200)
            vehicle->applyEngineForce(wheels[a].engineForce * engineMultiplier,a);
        else
            vehicle->applyEngineForce(0,a);
    }

    return body->getLinearVelocity().length() > 200;
}

void brickCar::addWheels(btVector3 halfExtents)
{
    //The direction of the raycast, the btRaycastVehicle uses raycasts instead of simiulating the wheels with rigid bodies
    btVector3 wheelDirectionCS0(0, -1, 0);
    //The axis which the wheel rotates arround
    btVector3 wheelAxleCS(-1, 0, 0);

    btScalar suspensionRestLength(0.7);
    btScalar wheelWidth(0.4);
    btScalar wheelRadius(1.2);

    //The height where the wheels are connected to the chassis
    btScalar connectionHeight(0.5);

    //All the wheel configuration assumes the vehicle is centered at the origin and a right handed coordinate system is used
    btVector3 wheelConnectionPoint(halfExtents.x() + wheelRadius, connectionHeight - halfExtents.y(), halfExtents.z() - wheelWidth);

    //Adds the front wheels
    vehicle->addWheel(wheelConnectionPoint, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, true);

    vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, 1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, true);

    //Adds the rear wheels
    vehicle->addWheel(wheelConnectionPoint* btVector3(1, 1, -1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, false);

    vehicle->addWheel(wheelConnectionPoint * btVector3(-1, 1, -1), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, false);

    //Configures each wheel of our vehicle, setting its friction, damping compression, etc.
    //For more details on what each parameter does, refer to the docs
    for (int i = 0; i < vehicle->getNumWheels(); i++)
    {
        btWheelInfo& wheel = vehicle->getWheelInfo(i);
        wheel.m_suspensionStiffness = 100;
        wheel.m_wheelsDampingCompression = btScalar(0.3) * 2 * btSqrt(wheel.m_suspensionStiffness);//btScalar(0.8);
        wheel.m_wheelsDampingRelaxation = btScalar(0.5) * 2 * btSqrt(wheel.m_suspensionStiffness);//1;
        //Larger friction slips will result in better handling
        wheel.m_frictionSlip = btScalar(1.2);
        wheel.m_rollInfluence = 1;
    }
}

void brickCar::sendBricksToClient(serverClientHandle *user)
{
    packet wheelPacket;
    wheelPacket.writeUInt(packetType_addBricksToBuiltCar,packetTypeBits);
    wheelPacket.writeUInt(serverID,10);
    wheelPacket.writeUInt(wheels.size(),5);
    for(unsigned int a = 0; a<wheels.size(); a++)
    {
        wheelPacket.writeFloat(wheels[a].scale.y());
        wheelPacket.writeFloat(wheels[a].carX);
        wheelPacket.writeFloat(wheels[a].carY);
        wheelPacket.writeFloat(wheels[a].carZ);
        wheelPacket.writeFloat(wheels[a].breakForce);
        wheelPacket.writeFloat(wheels[a].steerAngle);
        wheelPacket.writeFloat(wheels[a].engineForce);
        wheelPacket.writeFloat(wheels[a].suspensionLength);
        wheelPacket.writeFloat(wheels[a].suspensionStiffness);
        wheelPacket.writeFloat(wheels[a].dampingCompression);
        wheelPacket.writeFloat(wheels[a].dampingRelaxation);
        wheelPacket.writeFloat(wheels[a].frictionSlip);
        wheelPacket.writeFloat(wheels[a].rollInfluence);
        wheelPacket.writeUInt(wheels[a].brickTypeID,20);
    }
    wheelPacket.writeUInt(bricks.size(),16);
    wheelPacket.writeUInt(0,8);
    user->send(&wheelPacket,true);

    int iter = 0;
    int toSend = bricks.size();
    while(toSend > 0)
    {
        int sentThisPacket = toSend;
        if(sentThisPacket >= 30)
            sentThisPacket = 30;

        packet data;
        data.writeUInt(packetType_addBricksToBuiltCar,packetTypeBits);
        data.writeUInt(serverID,10);
        data.writeUInt(0,5); //number of wheels, wheels already sent above

        data.writeUInt(bricks.size(),16);
        data.writeUInt(sentThisPacket,8);
        for(int a = 0; a<sentThisPacket; a++)
        {
            brick *toSend = bricks[iter];
            iter++;

            toSend->addToPacket(&data,true);
        }

        user->send(&data,true);

        toSend -= sentThisPacket;
    }
}

brickCar::~brickCar()
{
    for(unsigned int a = 0; a<bricks.size(); a++)
    {
        if(bricks[a])
        {
            delete bricks[a];
            bricks[a] = 0;
        }
    }
    bricks.clear();
}



