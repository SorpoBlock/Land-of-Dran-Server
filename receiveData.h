#ifndef RECEIVEDATA_H_INCLUDED
#define RECEIVEDATA_H_INCLUDED

#include "code/unifiedWorld.h"
#include "code/sendInitialData.h"
#include "code/aabbCallback.h"
#include "code/lua/events.h"

enum clientToServerPacketTypes
{
    requestName = 0,
    startLoadingStageTwo = 1,
    playerControlPacket = 2,
    plantBrickRequest = 3,
    undoBrick = 4,
    requestSelection = 5,
    evalPassword = 6,
    evalCode = 7,
    clicked = 8,
    dropItem = 9,
    wrenchSubmit = 10,
    carPlantRequest = 11,
    chatMessage = 12,
    avatarPrefs = 13,
    toggleAdminOrb = 14,
    pong = 15
};

using namespace syj;

float debugYaw = 0;

std::string bts(bool in)
{
    return in ? "t" : "f";
}

size_t getAuthResponse(void *buffer,size_t size,size_t nmemb,void *userp)
{
    clientData *source = (clientData*)userp;

    if(!source)
        return nmemb;

    std::string response;
    response.assign((char*)buffer,nmemb);
    info("Auth response: "+response+"\n");

    if(response.length() < 5)
    {
        source->logInState = 3;
        return nmemb;
    }

    if(response.substr(0,4) == "GOOD")
    {
        source->logInState = 4;
        std::string id = response.substr(4,response.length() - 4);
        source->accountID = atoi(id.c_str());
    }
    else
        source->logInState = 3;

    return nmemb;
}

void receiveData(server *host,serverClientHandle *client,packet *data)
{
    unifiedWorld *common = (unifiedWorld*)host->userData;
    clientData *source = 0;
    for(unsigned int a = 0; a<common->users.size(); a++)
    {
        if(common->users[a]->netRef == client)
        {
            source = common->users[a];
            break;
        }
    }

    if(!source)
    {
        error("Received packet from client which does not have an allocated clientData yet!");
        return;
    }

    clientToServerPacketTypes type = (clientToServerPacketTypes)data->readUInt(packetTypeBitsFromClient);
    switch(type)
    {
        case pong:
        {
            int pingID = data->readUInt(32);
            if(pingID == source->lastPingID)
                source->lastPingRecvAtMS = SDL_GetTicks();
            source->waitingOnPing = false;
            return;
        }
        case toggleAdminOrb:
        {
            bool useAdmin = data->readBit();
            if(!useAdmin) //end admin orb
            {
                float x = data->readFloat();
                float y = data->readFloat();
                float z = data->readFloat();

                bool cancelled = clientAdminOrb(source,false,x,y,z);

                if(cancelled)
                    return;

                source->cameraAdminOrb = useAdmin;
                source->sendCameraDetails();

                if(source->adminOrb)
                {
                    common->removeEmitter(source->adminOrb);
                    source->adminOrb = 0;
                }

                if(source->controlling)
                {
                    btTransform t = source->controlling->getWorldTransform();
                    t.setOrigin(btVector3(x,y,z));
                    source->controlling->setWorldTransform(t);
                    source->forceTransformUpdate();
                }

                if(source->driving && source->driving->body)
                {
                    btTransform t = source->driving->body->getWorldTransform();
                    t.setOrigin(btVector3(x,y,z));
                    source->driving->body->setWorldTransform(t);
                    source->driving->body->setLinearVelocity(btVector3(0,0,0));
                    source->driving->body->activate();
                }
            }
            else if(!source->adminOrb) //start admin orb
            {
                float x=0,y=0,z=0;
                if(source->controlling)
                {
                    btVector3 pos = source->controlling->getWorldTransform().getOrigin();
                    x = pos.x();
                    y = pos.y();
                    z = pos.z();
                }

                bool cancelled = clientAdminOrb(source,true,x,y,z);

                if(cancelled)
                    return;

                source->cameraAdminOrb = useAdmin;
                source->sendCameraDetails();
                source->adminOrb = common->addEmitter(common->getEmitterType("CameraEmitter"));
            }
            return;
        }
        case avatarPrefs:
        {
            source->wantedNodeColors.clear();
            source->wantedNodeNames.clear();

            int chosenDecal = data->readUInt(7);
            source->wantedDecal = chosenDecal;
            /*if(source->controlling)
                source->controlling->chosenDecal = chosenDecal;*/
            int howMany = data->readUInt(8);
            for(int a = 0; a<howMany; a++)
            {
                std::string nodeName = data->readString();
                float r = data->readUInt(8);
                r /= 255.0;
                float g = data->readUInt(8);
                g /= 255.0;
                float b = data->readUInt(8);
                b /= 255.0;
                //if(source->controlling)
                //{
                    source->wantedNodeNames.push_back(nodeName);
                    source->wantedNodeColors.push_back(btVector3(r,g,b));
                    //source->controlling->setNodeColor(nodeName,btVector3(r,g,b),0);
                //}
            }

            if(source->controlling)
                common->applyAppearancePrefs(source,source->controlling);

            return;
        }
        case chatMessage:
        {
            std::string chat = data->readString();

            if(chat.length() < 1)
                return;

            if(chat.length() > 200)
                chat = chat.substr(0,200);

            replaceAll(chat,"[","\\[");

            if(chat[0] == '>')
                chat = "[colour='FF00FF00']" + chat;
            if(chat[0] == '<')
                chat = "[colour='FFFF0000']" + chat;

            bool cancelled = clientChat(source,chat);

            if(cancelled)
                return;

            if(common->hasNonoWord(chat))
            {
                chat = common->replaceNonoWords(chat);
                source->message("[colour='FFFF0000']No-no word detected.","error");
            }

            if(source->lastChatMessage + 1000 > SDL_GetTicks())
                source->message("[colour='FFFF0000']Don't send messages so fast!","error");
            else
            {
                source->lastChatMessage = SDL_GetTicks();
                common->messageAll("[colour='FFFFFF00']" + source->name + "[colour='FFFFFFFF']: " + chat,"chat");
                info(source->name + ": " + chat);
            }

            return;
        }

        case carPlantRequest:
        {
            if(!source->loadingCar)
                source->loadingCar = new brickCar;

            //Main loop will clear out partial loads after a while:
            source->carLoadStartTime = SDL_GetTicks();

            source->loadingCar->ownerID = source->accountID;

            int packetSubType = data->readUInt(3);

            switch(packetSubType)
            {
                case 0: //Amounts and settings
                {
                    source->basicBricksLeftToLoad += data->readUInt(10);
                    source->specialBricksLeftToLoad += data->readUInt(10);
                    data->readUInt(6); //How many lights, redundant atm
                    data->readUInt(6); //How many wheels, redundant atm
                    source->loadCarAsCar = data->readBit();
                    source->loadedCarOrigin.setX(data->readFloat());
                    source->loadedCarOrigin.setY(data->readFloat());
                    source->loadedCarOrigin.setZ(data->readFloat());
                    source->loadedCarAmountsPacket = true;
                    break;
                }
                case 1: //Lights
                {
                    source->loadingCarLights.clear();

                    source->loadedCarLights = true;

                    int howMany = data->readUInt(8);
                    for(int a = 0; a<howMany; a++)
                    {
                        float r = data->readFloat();
                        float g = data->readFloat();
                        float b = data->readFloat();

                        light *l = common->addLight(btVector3(r,g,b),btVector3(0,0,0),true);
                        l->offset.setX(data->readFloat());
                        l->offset.setY(data->readFloat());
                        l->offset.setZ(data->readFloat());

                        l->isSpotlight = data->readBit();
                        if(l->isSpotlight)
                        {
                            l->direction.setX(data->readFloat());
                            l->direction.setY(data->readFloat());
                            l->direction.setZ(data->readFloat());
                            l->direction.setW(acos(data->readFloat()));
                        }

                        source->loadingCarLights.push_back(l);
                    }

                    break;
                }
                case 2: //Wheels
                {
                    source->loadedCarWheels = true;

                    int toRead = data->readUInt(5);
                    for(unsigned int i = 0; i<toRead; i++)
                    {
                        //wheel data
                        wheelData wheel;
                        wheel.loadOrder = source->totalLoadedWheels;
                        source->totalLoadedWheels++;
                        float scale = data->readFloat();
                        wheel.scale = btVector3(scale,scale,scale);
                        wheel.wheelAxleCS = btVector3(-1, 0, 0); //change for rotated cars
                        float carX = data->readFloat();
                        float carY = data->readFloat();
                        float carZ = data->readFloat();
                        wheel.carX = carX;
                        wheel.carY = carY;
                        wheel.carZ = carZ;
                        wheel.offset = btVector3(carX,carY,carZ);
                        wheel.breakForce = data->readFloat();
                        wheel.steerAngle = data->readFloat();
                        wheel.engineForce = data->readFloat();
                        wheel.suspensionLength = data->readFloat();
                        wheel.suspensionStiffness = data->readFloat();
                        wheel.dampingCompression = data->readFloat();
                        wheel.dampingRelaxation = data->readFloat();
                        wheel.frictionSlip = data->readFloat();
                        wheel.rollInfluence = data->readFloat();
                        int typeID = data->readUInt(10);
                        wheel.brickTypeID = 0;
                        if(typeID < 0 || typeID >= common->brickTypes->brickTypes.size())
                            error("Invalid typeID " + std::to_string(typeID) + " on special brick carPlant request.");
                        else
                        {
                            if(common->brickTypes->brickTypes[typeID]->isWheelType)
                                wheel.brickTypeID = typeID;
                            else
                                error("Wheel brick data type " + std::to_string(typeID) +"/" + common->brickTypes->brickTypes[typeID]->uiname + " from loaded car is not wheel?");
                        }

                        source->loadingCar->wheels.push_back(wheel);
                    }

                    break;
                }
                case 3: //Basic bricks
                {
                    int toRead = data->readUInt(6);
                    for(unsigned int i = 0; i<toRead; i++)
                    {
                        //basic brick data
                        brick *tmp = new brick;
                        tmp->carX = data->readFloat();
                        tmp->carY = data->readFloat();
                        tmp->carPlatesUp = data->readUInt(10);
                        tmp->yHalfPosition = data->readBit();
                        tmp->carZ = data->readFloat();

                        tmp->width = data->readUInt(8);
                        tmp->height = data->readUInt(8);
                        tmp->length = data->readUInt(8);
                        tmp->angleID = data->readUInt(2);
                        int shapeFx = data->readUInt(4);
                        int material = data->readUInt(4);
                        if(shapeFx == 1)
                            material += 1000;
                        else if(shapeFx == 2)
                            material += 2000;
                        tmp->material = material;

                        tmp->r = data->readUInt(8);
                        tmp->r /= 255.0;
                        tmp->g = data->readUInt(8);
                        tmp->g /= 255.0;
                        tmp->b = data->readUInt(8);
                        tmp->b /= 255.0;
                        tmp->a = data->readUInt(8);
                        tmp->a /= 255.0;
                        if(tmp->width <= 0 || tmp->width >= 255 || tmp->height <= 0 || tmp->height >= 255 || tmp->length <= 0 || tmp->length >= 255)
                            break;

                        tmp->builtBy = source->accountID;
                        source->loadingCar->bricks.push_back(tmp);
                    }

                    source->basicBricksLeftToLoad -= toRead;

                    break;
                }
                case 4: //Special bricks
                {
                    int toRead = data->readUInt(6);
                    for(unsigned int i = 0; i<toRead; i++)
                    {
                        int typeID = data->readUInt(10);

                        if(typeID < 0 || typeID >= common->brickTypes->brickTypes.size())
                        {
                            error("Invalid typeID " + std::to_string(typeID) + " on special brick carPlant request.");
                            break;
                        }

                        //special brick data
                        brick *tmp = 0;
                        if(common->brickTypes->brickTypes[typeID]->uiname.find("steering") != std::string::npos)
                        {
                            tmp = new steeringBrick;
                            tmp->isSteeringWheel = true;
                            tmp->isWheel = false;
                        }
                        else if(common->brickTypes->brickTypes[typeID]->isWheelType)
                        //else if(common->brickTypes->brickTypes[typeID]->uiname.find("vert wheel") != std::string::npos)
                        {
                            tmp = new wheelBrick;
                            tmp->isWheel = true;
                            tmp->isSteeringWheel = false;
                            ((wheelBrick*)tmp)->loadOrder = source->carWheelsMadeSoFar;
                            source->carWheelsMadeSoFar++;
                        }
                        else
                        {
                            tmp = new brick;
                            tmp->isSteeringWheel = false;
                            tmp->isWheel = false;
                        }

                        tmp->isSpecial = true;
                        tmp->typeID = typeID;
                        tmp->width = common->brickTypes->brickTypes[typeID]->width;
                        tmp->length = common->brickTypes->brickTypes[typeID]->length;
                        tmp->height = common->brickTypes->brickTypes[typeID]->height;

                        tmp->material = 0;

                        tmp->carX = data->readFloat();
                        tmp->carY = data->readFloat();
                        tmp->carPlatesUp = data->readUInt(10);
                        tmp->yHalfPosition = data->readBit();
                        tmp->carZ = data->readFloat();

                        tmp->isSpecial = true;
                        tmp->angleID = data->readUInt(2);
                        int shapeFx = data->readUInt(4);
                        int material = data->readUInt(4);
                        if(shapeFx == 1)
                            material += 1000;
                        else if(shapeFx == 2)
                            material += 2000;
                        tmp->material = material;

                        tmp->r = data->readUInt(8);
                        tmp->r /= 255.0;
                        tmp->g = data->readUInt(8);
                        tmp->g /= 255.0;
                        tmp->b = data->readUInt(8);
                        tmp->b /= 255.0;
                        tmp->a = data->readUInt(8);
                        tmp->a /= 255.0;

                        if(tmp->width <= 0 || tmp->width >= 255 || tmp->height <= 0 || tmp->height >= 255 || tmp->length <= 0 || tmp->length >= 255)
                            break;

                        tmp->builtBy = source->accountID;
                        source->loadingCar->bricks.push_back(tmp);
                    }

                    source->specialBricksLeftToLoad -= toRead;

                    break;
                }
            }

            if(!source->loadedCarAmountsPacket)
                return;
            if(source->basicBricksLeftToLoad > 0)
                return;
            if(source->specialBricksLeftToLoad > 0)
                return;
            if(!source->loadedCarLights)
                return;
            if(!source->loadedCarWheels)
                return;

            //Whatever happens, we're done with the car data:
            source->basicBricksLeftToLoad = 0;
            source->specialBricksLeftToLoad = 0;
            source->loadedCarAmountsPacket = false;
            source->loadedCarLights = false;
            source->loadedCarWheels = false;

            bool makeCar = true;
            if(source->loadingCar->bricks.size() < 2)
            {
                makeCar = false;
                source->bottomPrint("There were no bricks in your car to load!",5000);
            }
            if(source->lastAttemptedCarPlant + 5000 > SDL_GetTicks())
            {
                makeCar = false;
                source->bottomPrint("Please wait a bit longer before planting another car!",5000);
            }

            //See if any Lua scripts want to disable or change car making
            bool cancelled = clientTryLoadCarEvent(source,source->loadingCar,source->loadCarAsCar);
            if(cancelled)
                makeCar = false;

            if(makeCar)
            {
                source->lastAttemptedCarPlant = SDL_GetTicks();

                if(source->loadCarAsCar)
                {
                    float heightCorrection = 0;
                    common->compileBrickCar(source->loadingCar,heightCorrection,true,source->loadedCarOrigin);
                    for(int a = 0; a<source->loadingCarLights.size(); a++)
                    {
                        source->loadingCarLights[a]->attachedCar = source->loadingCar;
                        source->loadingCarLights[a]->offset.setY(source->loadingCarLights[a]->offset.y()-heightCorrection);
                        source->loadingCar->lights.push_back(source->loadingCarLights[a]);
                        for(int b = 0; b<common->users.size(); b++)
                            source->loadingCarLights[a]->sendToClient(common->users[b]->netRef);
                    }
                    source->loadingCarLights.clear();
                }
                else
                {
                    for(int a = 0; a<source->loadingCar->bricks.size(); a++)
                    {
                        if(!source->loadingCar->bricks[a]->isWheel)
                            continue;

                        wheelBrick *tmp = (wheelBrick*)source->loadingCar->bricks[a];
                        for(int b = 0; b<source->loadingCar->wheels.size(); b++)
                        {
                            if(source->loadingCar->wheels[b].loadOrder == tmp->loadOrder)
                            {
                                tmp->wheelSettings = source->loadingCar->wheels[b];
                            }
                        }
                    }

                    for(int a = 0; a<source->loadingCar->bricks.size(); a++)
                    {
                        source->loadingCar->bricks[a]->calcGridFromCarPos(source->loadedCarOrigin);
                        if(!common->addBrick(source->loadingCar->bricks[a],true))
                        {
                            delete source->loadingCar->bricks[a];
                            source->loadingCar->bricks[a] = 0;
                        }
                        else
                        {
                            source->ownedBricks.push_back(source->loadingCar->bricks[a]);
                            source->undoList.push_back(source->loadingCar->bricks[a]->serverID);
                        }
                    }
                }

                //for(int a = 0; a<source->loadingCarLights.size(); a++)
                    //delete source->loadingCarLights[a];
                source->loadingCarLights.clear();
            }
            else
            {
                for(int a = 0; a<source->loadingCarLights.size(); a++)
                {
                    for(int b = 0; b<common->lights.size(); b++)
                    {
                        if(common->lights[b] == source->loadingCarLights[a])
                        {
                            common->lights.erase(common->lights.begin() + b);
                            break;
                        }
                    }
                    delete source->loadingCarLights[a];
                }
                source->loadingCarLights.clear();
                for(int a = 0; a<source->loadingCar->bricks.size(); a++)
                    delete source->loadingCar->bricks[a];
                source->loadingCar->bricks.clear();
                delete source->loadingCar;
            }
            source->loadingCar = 0;

            return;
        }

        case wrenchSubmit:
        {
            if(!source->lastWrenchedBrick)
                return;

            bool isWheel = data->readBit();
            if(isWheel)
            {
                if(!source->lastWrenchedBrick->isWheel)
                {
                    error("Client submitted wheel wrench data for non-wheel brick!");
                    return;
                }

                wheelBrick *wheel = (wheelBrick*)source->lastWrenchedBrick;
                wheel->wheelSettings.breakForce = data->readFloat();
                wheel->wheelSettings.steerAngle = data->readFloat();
                wheel->wheelSettings.engineForce = data->readFloat();
                wheel->wheelSettings.suspensionLength = data->readFloat();
                wheel->wheelSettings.suspensionStiffness = data->readFloat();
                wheel->wheelSettings.frictionSlip = data->readFloat();
                wheel->wheelSettings.rollInfluence = data->readFloat();
                wheel->wheelSettings.dampingCompression = data->readFloat();
                wheel->wheelSettings.dampingRelaxation = data->readFloat();

                if(wheel->wheelSettings.breakForce < 0)
                    wheel->wheelSettings.breakForce = 0;
                if(wheel->wheelSettings.breakForce > 2000)
                    wheel->wheelSettings.breakForce = 2000;

                if(wheel->wheelSettings.steerAngle < -3.1415)
                    wheel->wheelSettings.steerAngle = -3.1415;
                if(wheel->wheelSettings.steerAngle > 3.1415)
                    wheel->wheelSettings.steerAngle = 3.1415;

                if(wheel->wheelSettings.engineForce < -2000)
                    wheel->wheelSettings.engineForce = -2000;
                if(wheel->wheelSettings.engineForce > 2000)
                    wheel->wheelSettings.engineForce = 2000;

                if(wheel->wheelSettings.suspensionLength < 0.1)
                    wheel->wheelSettings.suspensionLength = 0.1;
                if(wheel->wheelSettings.suspensionLength > 5)
                    wheel->wheelSettings.suspensionLength = 5;

                if(wheel->wheelSettings.suspensionStiffness < 1)
                    wheel->wheelSettings.suspensionStiffness = 1;
                if(wheel->wheelSettings.suspensionStiffness > 1000)
                    wheel->wheelSettings.suspensionStiffness = 1000;

                if(wheel->wheelSettings.frictionSlip < 0.1)
                    wheel->wheelSettings.frictionSlip = 0.1;
                if(wheel->wheelSettings.frictionSlip > 10)
                    wheel->wheelSettings.frictionSlip = 10;

                if(wheel->wheelSettings.rollInfluence < 0.1)
                    wheel->wheelSettings.rollInfluence = 0.1;
                if(wheel->wheelSettings.rollInfluence > 10)
                    wheel->wheelSettings.rollInfluence = 10;

                if(wheel->wheelSettings.dampingCompression < 1)
                    wheel->wheelSettings.dampingCompression = 1;
                if(wheel->wheelSettings.dampingCompression > 100)
                    wheel->wheelSettings.dampingCompression = 100;

                if(wheel->wheelSettings.dampingRelaxation < 1)
                    wheel->wheelSettings.dampingRelaxation = 1;
                if(wheel->wheelSettings.dampingRelaxation > 100)
                    wheel->wheelSettings.dampingRelaxation = 100;

                return;
            }

            bool isSteering = data->readBit();
            if(isSteering)
            {
                if(!source->lastWrenchedBrick->isSteeringWheel)
                {
                    error("Client submitted steering wrench data for non-steering brick!");
                    return;
                }

                steeringBrick *wheel = (steeringBrick*)source->lastWrenchedBrick;
                //wheel->steeringSettings.mass = data->readUInt(8);
                wheel->steeringSettings.mass = data->readFloat();
                wheel->steeringSettings.angularDamping = data->readFloat();
                wheel->steeringSettings.realisticCenterOfMass = !data->readBit();
                if(wheel->steeringSettings.angularDamping < 0)
                    wheel->steeringSettings.angularDamping = 0;
                if(wheel->steeringSettings.angularDamping > 1.0)
                    wheel->steeringSettings.angularDamping = 1.0;
                if(wheel->steeringSettings.mass < 1.5)
                    wheel->steeringSettings.mass = 1.5;
                if(wheel->steeringSettings.mass > 30)
                    wheel->steeringSettings.mass = 30;

                return;
            }

            bool colliding = data->readBit();
            bool up = data->readBit();
            bool down = data->readBit();
            bool north = data->readBit();
            bool south = data->readBit();
            bool east = data->readBit();
            bool west = data->readBit();
            int music = data->readUInt(10);
            float pitch = data->readFloat();
            std::string name = data->readString();

            //std::cout<<bts(up)<<bts(down)<<bts(north)<<bts(south)<<bts(east)<<bts(west)<<music<<"\n";

            int renderMask = source->lastWrenchedBrick->printMask & ~0b111111;
            renderMask |= north ? 4 : 0;
            renderMask |= south ? 8 : 0;
            renderMask |= up ? 1 : 0;
            renderMask |= down ? 2 : 0;
            renderMask |= east ? 16 : 0;
            renderMask |= west ? 32 : 0;
            source->lastWrenchedBrick->printMask = renderMask;
            if(pitch < 0.25)
                pitch = 0.25;
            if(pitch > 4.0)
                pitch = 4.0;
            source->lastWrenchedBrick->musicPitch = pitch;

            bool hasLight = data->readBit();
            if(hasLight)
            {
                float red = data->readFloat();
                float green = data->readFloat();
                float blue = data->readFloat();

                red = std::clamp(red,-10000.0f,10000.0f);
                green = std::clamp(green,-10000.0f,10000.0f);
                blue = std::clamp(blue,-10000.0f,10000.0f);

                if(!source->lastWrenchedBrick->attachedLight)
                    common->addLight(btVector3(red,green,blue),source->lastWrenchedBrick,true);
                else
                    source->lastWrenchedBrick->attachedLight->color = btVector3(red,green,blue);

                source->lastWrenchedBrick->attachedLight->isSpotlight = data->readBit();
                if(source->lastWrenchedBrick->attachedLight->isSpotlight)
                {
                    source->lastWrenchedBrick->attachedLight->direction.setW(data->readFloat());
                    float yaw = data->readFloat();
                    float lightPitch = data->readFloat();
                    source->lastWrenchedBrick->attachedLight->direction.setX(cos(lightPitch) * sin(yaw));
                    source->lastWrenchedBrick->attachedLight->direction.setY(sin(lightPitch));
                    source->lastWrenchedBrick->attachedLight->direction.setZ(cos(lightPitch) * cos(yaw));

                    /*source->lastWrenchedBrick->attachedLight->direction.setX(sin(yaw));
                    source->lastWrenchedBrick->attachedLight->direction.setY(sin(lightPitch));
                    source->lastWrenchedBrick->attachedLight->direction.setZ(cos(yaw));*/
                }

                for(int a = 0; a<common->users.size(); a++)
                    source->lastWrenchedBrick->attachedLight->sendToClient(common->users[a]->netRef);
            }
            else
            {
                if(source->lastWrenchedBrick->attachedLight)
                    common->removeLight(source->lastWrenchedBrick->attachedLight);
            }

            //common->setMusic(source->lastWrenchedBrick,music);

            if(!source->lastWrenchedBrick->car)
            {
                common->setBrickName(source->lastWrenchedBrick,name);

                source->lastWrenchedBrick->music = music;
                //Music is playing and we want it to stop
                if(source->lastWrenchedBrick->music == 0 && source->lastWrenchedBrick->musicLoopId != -1)
                {
                    common->stopSoundLoop(source->lastWrenchedBrick->musicLoopId);
                    source->lastWrenchedBrick->musicLoopId = -1;
                }
                //Music is playing and we want to change it
                else if(source->lastWrenchedBrick->music != 0 && source->lastWrenchedBrick->musicLoopId != -1)
                {
                    common->stopSoundLoop(source->lastWrenchedBrick->musicLoopId);
                    source->lastWrenchedBrick->musicLoopId = common->startSoundLoop(music,source->lastWrenchedBrick->getX(),source->lastWrenchedBrick->getY(),source->lastWrenchedBrick->getZ(),source->lastWrenchedBrick->musicPitch);
                }
                //Nothing is playing and we want to start
                else if(source->lastWrenchedBrick->music != 0 && source->lastWrenchedBrick->musicLoopId == -1)
                    source->lastWrenchedBrick->musicLoopId = common->startSoundLoop(music,source->lastWrenchedBrick->getX(),source->lastWrenchedBrick->getY(),source->lastWrenchedBrick->getZ(),source->lastWrenchedBrick->musicPitch);


                /*if(source->lastWrenchedBrick->body && !colliding)
                {
                    common->physicsWorld->removeRigidBody(source->lastWrenchedBrick->body);
                    delete source->lastWrenchedBrick->body;
                    source->lastWrenchedBrick->body = 0;
                    packet update;
                    source->lastWrenchedBrick->createUpdatePacket(&update);
                    common->theServer->send(&update,true);
                }
                else if(source->lastWrenchedBrick->body == 0 && colliding)
                {
                    common->brickTypes->addPhysicsToBrick(source->lastWrenchedBrick,common->physicsWorld);
                    packet update;
                    source->lastWrenchedBrick->createUpdatePacket(&update);
                    common->theServer->send(&update,true);
                }*/


                if(colliding != source->lastWrenchedBrick->isColliding())
                {
                    source->lastWrenchedBrick->setColliding(common->physicsWorld,colliding);

                    packet update;
                    source->lastWrenchedBrick->createUpdatePacket(&update);
                    common->theServer->send(&update,true);
                }
            }
            else
            {
                brickCar *theCar = (brickCar*)source->lastWrenchedBrick->car;
                theCar->music = music;
                theCar->musicPitch = pitch;
                //Music is playing and we want it to stop
                if(theCar->music == 0 && theCar->musicLoopId != -1)
                {
                    common->stopSoundLoop(theCar->musicLoopId);
                    theCar->musicLoopId = -1;
                }
                //Music is playing and we want to change it
                else if(theCar->music != 0 && theCar->musicLoopId != -1)
                {
                    common->stopSoundLoop(theCar->musicLoopId);
                    theCar->musicLoopId = common->startSoundLoop(music,theCar,theCar->musicPitch);
                }
                //Nothing is playing and we want to start
                else if(theCar->music != 0 && theCar->musicLoopId == -1)
                    theCar->musicLoopId = common->startSoundLoop(music,theCar,theCar->musicPitch);

            }

            source->lastWrenchedBrick = 0;

            return;
        }
        case dropItem:
        {
            int invSlot = data->readUInt(3);
            if(invSlot >= inventorySize)
            {
                //error("Drop item inventory slot greater than inventory size: " + std::to_string(invSlot));
                invSlot = -1;
            }

            if(!source->controlling)
                return;

            if(invSlot == -1)
                return;

            if(!source->controlling->holding[invSlot])
                return;

            btVector3 pos = source->controlling->getWorldTransform().getOrigin();
            common->playSound("ToolSwitch",pos.x(),pos.y(),pos.z());
            common->dropItem(source->controlling,invSlot);
            return;
        }
        case clicked:
        {
            bool isLeft = data->readBit();
            float camX = data->readFloat();
            float camY = data->readFloat();
            float camZ = data->readFloat();
            float dirX = data->readFloat();
            float dirY = data->readFloat();
            float dirZ = data->readFloat();
            int invSlot = data->readUInt(3);
            if(invSlot >= inventorySize)
            {
                //error("Clicked packet exceeded inventory size! " + std::to_string(invSlot));
                invSlot = -1;
            }

            if(!isLeft)
            {
                if(source->driving && source->controlling) //dismount vehicle get out of car
                {
                    source->driving->driveCar(false,false,false,false,false);
                    source->controlling->setCollisionFlags(source->controlling->oldCollisionFlags);

                    btTransform t = source->controlling->getWorldTransform();
                    btVector3 o = t.getOrigin();
                    o.setY(o.getY() + source->driving->halfExtents.y() + 0.1);
                    t.setOrigin(o);
                    source->controlling->setWorldTransform(t);
                    source->controlling->setLinearVelocity(source->driving->body->getLinearVelocity());
                    source->controlling->setAngularVelocity(btVector3(0,0,0));

                    source->setControlling(source->controlling);

                    source->driving->occupied = false;
                    source->driving = 0;
                    source->controlling->sittingOn = 0;
                    source->sendCameraDetails();
                    //source->forceTransformUpdate();
                    return;
                }
            }

            btVector3 raystart = btVector3(camX,camY,camZ);
            btVector3 rayend = raystart + btVector3(dirX,dirY,dirZ) * 30.0;
            btCollisionWorld::AllHitsRayResultCallback res(raystart,rayend);
            common->physicsWorld->rayTest(raystart,rayend,res);

            btRigidBody *closest = 0;
            float closestDist = 999999;
            btVector3 clickPos;

            for(int a = 0; a<res.m_collisionObjects.size(); a++)
            {
                if(source->cameraTarget)
                {
                    if(res.m_collisionObjects[a] == source->cameraTarget)
                        continue;
                }

                if(source->controlling)
                {
                    if(res.m_collisionObjects[a] == source->controlling)
                        continue;
                }

                if(closest == 0 || ((res.m_hitPointWorld[a] - raystart).length() < closestDist))
                {
                    closestDist = (res.m_hitPointWorld[a] - raystart).length();
                    closest = (btRigidBody*)res.m_collisionObjects[a];
                    clickPos = res.m_hitPointWorld[a];
                }
            }

            if(!closest)
                return;

            btRigidBody *body = closest;

            if(body->getUserIndex() == bodyUserIndex_brick)
            {
                if(!body->getUserPointer())
                    return;

                clientClickBrickEvent(source,(brick*)body->getUserPointer(),clickPos.x(),clickPos.y(),clickPos.z(),isLeft);
            }
            else if(body->getUserIndex() == bodyUserIndex_item)
            {
                if(!body->getUserPointer())
                    return;

                item *i = (item*)body->getUserPointer();

                int freeSlot = -1;
                for(unsigned int a = 0; a<inventorySize; a++)
                {
                    if(!source->controlling->holding[a])
                    {
                        freeSlot = a;
                        break;
                    }
                }

                if(freeSlot == -1)
                    return;

                common->pickUpItem(source->controlling,i,freeSlot,source);
                common->playSound("Beep",clickPos.x(),clickPos.y(),clickPos.z());

                return;
            }
            else if(body->getUserIndex() == bodyUserIndex_builtCar)
            {
                if(!body->getUserPointer())
                    return;
                brickCar *car = (brickCar*)body->getUserPointer();

                if(!isLeft)
                {
                    if(car->occupied)
                        return;

                    car->occupied = true;
                    source->driving = car;
                    source->controlling->sittingOn = car->body;
                    source->controlling->oldCollisionFlags = source->controlling->getCollisionFlags();
                    source->controlling->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
                    car->body->activate();

                    packet giveUpPlayerControl;
                    giveUpPlayerControl.writeUInt(packetType_clientPhysicsData,packetTypeBits);
                    giveUpPlayerControl.writeUInt(1,2); //subtype 1, resume client physics control
                    client->send(&giveUpPlayerControl,true);

                    //source->cameraTarget = car->body;
                    source->sendCameraDetails();
                    common->playSound("PlayerMount",raystart.x(),raystart.y(),raystart.z());
                    return;
                }
                else
                {
                    if(invSlot != -1)
                    {
                        if(source->controlling->holding[invSlot])
                        {
                            if(source->controlling->holding[invSlot]->heldItemType)
                            {
                                if(source->controlling->holding[invSlot]->heldItemType->uiName == "Wrench")
                                {
                                    return;
                                }
                            }
                        }
                    }

                    if(car->ownerID == source->accountID || source->hasLuaAccess)
                    {
                        //btVector3 relPos = car->body->getWorldTransform().getOrigin() - res.m_hitNormalWorld[a];
                        //car->body->applyImpulse(8.0 * btVector3(dirX,-dirY,dirZ),relPos);
                        //car->body->applyCentralImpulse(pow(car->mass,0.8) * 15.0 * btVector3(dirX,-dirY,dirZ));

                        btTransform t = car->body->getWorldTransform();
                        //t.setRotation(btQuaternion().getIdentity());

                        t.setRotation(btQuaternion(debugYaw,0,0));
                        debugYaw += 0.78539816339;
                        if(debugYaw >= 6.28318530718)
                            debugYaw -= 0;

                        btVector3 o = t.getOrigin();
                        o.setY(o.getY() + car->halfExtents.y() + 5);
                        t.setOrigin(o);

                        car->body->setWorldTransform(t);
                        car->body->setLinearVelocity(btVector3(0,0,0));
                        car->body->setAngularVelocity(btVector3(0,0,0));
                    }

                    return;
                }
            }

            return;
        }
        case evalCode:
        {
            if(!common->useLuaPassword)
            {
                error("Received eval code even though eval is turned off!");
                return;
            }

            if(!source->hasLuaAccess)
            {
                error("Received eval code from user who did not have access!");
                return;
            }

            std::string pass = data->readString();
            if(pass != common->luaPassword)
            {
                error("Client failed to send eval password with code to be eval'd");
                return;
            }

            std::string code = data->readString();
            std::string codeToEcho = code;
            replaceAll(codeToEcho,"[","\\[");
            info(source->name + " is executing code " + codeToEcho);
            if(code.length() < 1)
                return;
            if(luaL_dostring(common->luaState,code.c_str()) != LUA_OK)
            {
                if(lua_gettop(common->luaState) > 0)
                {
                    std::string err = lua_tostring(common->luaState,-1);
                    if(err.length() < 1)
                        return;

                    if(lua_gettop(common->luaState) > 0)
                        lua_pop(common->luaState,1);

                    replaceAll(err,"[","\\[");

                    error("[colour='FFFF0000']" + err);
                }
            }


            return;
        }
        case evalPassword:
        {
            std::string theirGuess = data->readString();

            if(!common->useLuaPassword)
                return;

            if(common->luaPassword.length() < 1)
                return;

            packet data;
            data.writeUInt(packetType_luaPasswordResponse,packetTypeBits);

            if(theirGuess == common->luaPassword)
            {
                info(source->name + " just authenticated and got Lua access!");
                source->hasLuaAccess = true;
                data.writeBit(true);
            }
            else
                data.writeBit(false);

            source->netRef->send(&data,true);

            return;
        }
        case requestSelection:
        {
            float minX = data->readFloat();
            float minY = data->readFloat();
            float minZ = data->readFloat();
            float maxX = data->readFloat();
            float maxY = data->readFloat();
            float maxZ = data->readFloat();
            clientDrewSelection(btVector3(minX,minY,minZ),btVector3(maxX,maxY,maxZ),common,source);
            return;
        }
        case undoBrick:
        {
            if(source->undoList.size() > 0)
            {
                int id = source->undoList[source->undoList.size()-1];
                source->undoList.pop_back();
                for(unsigned int a = 0; a<source->ownedBricks.size(); a++)
                {
                    if(source->ownedBricks[a]->serverID == id)
                    {
                        common->removeBrick(source->ownedBricks[a]);
                        return;
                    }
                }
            }
            return;
        }
        case plantBrickRequest:
        {
            if(!source->allowedToBuild)
            {
                source->bottomPrint("You cannot build!",5000);
                return;
            }

            bool isSteeringWheel = false;
            bool isWheel = false;

            /*float x = data->readFloat();
            float y = data->readFloat();
            float z = data->readFloat();*/

            int mainX = data->readUInt(14) - 8192;
            bool xPartial = data->readBit();

            int mainY = data->readUInt(16);
            bool yPartial = data->readBit();

            int mainZ = data->readUInt(14) - 8192;
            bool zPartial = data->readBit();

            float r = data->readFloat();
            float g = data->readFloat();
            float b = data->readFloat();
            float a = data->readFloat();
            float angleID = data->readUInt(2);
            float material = data->readUInt(16);
            float isSpecial = !data->readBit();
            unsigned int typeID = 0;
            unsigned int width = 0,height = 0,length = 0;

            if(isSpecial)
            {
                typeID = data->readUInt(10);

                //if(common->brickTypes->brickTypes[typeID]->uiname.find("vert wheel") != std::string::npos)
                if(common->brickTypes->brickTypes[typeID]->isWheelType)
                    isWheel = true;
                if(common->brickTypes->brickTypes[typeID]->uiname.find("steering") != std::string::npos)
                    isSteeringWheel = true;

                if(typeID < 0 || typeID >= common->brickTypes->brickTypes.size())
                {
                    error("Invalid type ID for client planted special brick!");
                    return;
                }

                if(!common->brickTypes->brickTypes[typeID]->special)
                {
                    error("Client somehow planted non-special brick as special.");
                    return;
                }
            }
            else
            {
                width = data->readUInt(8);
                height = data->readUInt(8);
                length = data->readUInt(8);

                if(width <= 0 || width >= 256 || height <= 0 || height >= 256 || length <= 0 || length >= 256)
                {
                    error("Client planted brick with invalid dims.");
                    return;
                }
            }

            brick *theBrick = 0;
            if(isWheel)
            {
                theBrick = new wheelBrick;
                theBrick->isWheel = true;
            }
            else if(isSteeringWheel)
            {
                theBrick = new steeringBrick;
                theBrick->isSteeringWheel = true;
            }
            else
                theBrick = new brick;

            theBrick->builtBy = source->accountID;

            theBrick->posX = mainX;
            theBrick->xHalfPosition = xPartial;

            theBrick->uPosY = mainY;
            theBrick->yHalfPosition = yPartial;

            theBrick->posZ = mainZ;
            theBrick->zHalfPosition = zPartial;

            theBrick->r = r;
            theBrick->g = g;
            theBrick->b = b;
            theBrick->a = a;
            theBrick->angleID = angleID;
            theBrick->material = material;
            theBrick->isSpecial = isSpecial;
            theBrick->typeID = typeID;
            theBrick->width = width;
            theBrick->height = height;
            theBrick->length = length;

            if(!common->addBrick(theBrick,true,true,false))
            {
                source->bottomPrint("Could not plant brick!",4000);
                delete theBrick;
            }
            else
            {
                common->bricksAddedThisFrame.push_back(theBrick);
                source->ownedBricks.push_back(theBrick);
                common->playSound("ClickPlant",theBrick->getX(),theBrick->getY(),theBrick->getZ());
                source->undoList.push_back(theBrick->serverID);
                clientPlantBrickEvent(source,theBrick);
            }

            return;
        }
        case playerControlPacket:
        {
            int controlMask = data->readUInt(5);
            bool didJump = data->readBit(); //Just controls the sound
            bool leftMouseDown = data->readBit();
            bool rightMouseDown = data->readBit();
            bool usingPaint = data->readBit();
            btVector4 paintColor;
            if(usingPaint)
            {
                paintColor.setX(data->readFloat());
                paintColor.setY(data->readFloat());
                paintColor.setZ(data->readFloat());
                paintColor.setW(data->readFloat());
            }
            float camDirX = data->readFloat();
            float camDirY = data->readFloat();
            float camDirZ = data->readFloat();
            int invSlot = data->readUInt(3);
            bool useAdminCam = data->readBit();
            if(useAdminCam)
            {
                source->camX = data->readFloat();
                source->camY = data->readFloat();
                source->camZ = data->readFloat();

                if(source->adminOrb)
                {
                    source->adminOrb->position = btVector3(source->camX,source->camY,source->camZ);
                    for(int a = 0; a<common->users.size(); a++)
                        source->adminOrb->sendToClient(common->users[a]->netRef);
                }
            }

            bool wasTyping = source->currentlyTyping;
            source->currentlyTyping = data->readBit();

            bool useClientPhysics = data->readBit();
            if(useClientPhysics)
            {
                float playerX = data->readFloat();
                float playerY = data->readFloat();
                float playerZ = data->readFloat();
                float velX = data->readFloat();
                float velY = data->readFloat();
                float velZ = data->readFloat();

                if(source->controlling)
                {
                    btTransform t = source->controlling->getWorldTransform();
                    btVector3 p = t.getOrigin();
                    /*t.setOrigin(btVector3(playerX,playerY,playerZ));
                    source->controlling->setWorldTransform(t);*/
                    source->interpolationOffset = btVector3(playerX,playerY,playerZ) - p;
                    source->interpolationStartTime = SDL_GetTicks();
                    source->lastInterpolation = SDL_GetTicks();
                    source->controlling->setLinearVelocity(btVector3(velX,velY,velZ));
                }
            }

            if(wasTyping != source->currentlyTyping)
            {
                packet data;
                data.writeUInt(packetType_addOrRemovePlayer,packetTypeBits);
                data.writeBit(false); // for typing players, not players list
                if(source->currentlyTyping)
                    data.writeBit(true); // add
                else
                    data.writeBit(false); // remove
                data.writeString(source->name);
                data.writeUInt(source->playerID,16);

                common->theServer->send(&data,true);
            }

            if(invSlot >= inventorySize)
            {
                //error("Control packet inv slot > inventorySize " + std::to_string(invSlot));
                invSlot = -1;
            }

            if(isnan(camDirX))
                return;
            if(isnan(camDirY))
                return;
            if(isnan(camDirZ))
                return;

            if(source->controlling)
            {
                btVector3 pos = source->controlling->getWorldTransform().getOrigin();

                if(didJump)
                    common->playSoundExcept("Jump",pos.x(),pos.y(),pos.z(),source);

                if(usingPaint && leftMouseDown && !source->paint)
                {
                    source->paint = common->addEmitter(common->getEmitterType("playerJetEmitter"));
                    source->paint->isJetEmitter = true;
                    source->paint->attachedToModel = source->controlling;
                    source->paint->nodeName = "Right_Hand";
                    source->paint->color = paintColor;
                    source->paint->shootFromHand = true;
                    for(unsigned int a = 0; a<common->users.size(); a++)
                        source->paint->sendToClient(common->users[a]->netRef);

                    btVector3 raystart = pos + btVector3(source->controlling->type->eyeOffsetX,source->controlling->type->eyeOffsetY,source->controlling->type->eyeOffsetZ);
                    btVector3 rayend = raystart + btVector3(camDirX,camDirY,camDirZ) * 30.0;
                    btCollisionWorld::ClosestRayResultCallback res(raystart,rayend);
                    common->physicsWorld->rayTest(raystart,rayend,res);


                    btRigidBody *body = (btRigidBody*)res.m_collisionObject;
                    if(body)
                    {
                        if(body->getUserIndex() == bodyUserIndex_brick)
                        {
                            if(body->getUserPointer())
                            {
                                brick *theBrick = (brick*)body->getUserPointer();
                                if(theBrick->builtBy == (int)source->accountID || theBrick->builtBy == 0)
                                {
                                    common->setBrickColor(theBrick,paintColor.x(),paintColor.y(),paintColor.z(),paintColor.w());
                                    return;
                                }
                            }
                        }
                    }
                }
                else if(source->paint)
                {
                    common->removeEmitter(source->paint);
                    source->paint = 0;
                }

                bool wasJetting = source->controlling->isJetting;

                if(!source->canJet)
                    source->controlling->isJetting = false;
                else
                    source->controlling->isJetting = rightMouseDown;

                if(wasJetting != source->controlling->isJetting)
                {
                    if(source->controlling->isJetting)
                    {
                        emitter *leftJet = common->addEmitter(common->getEmitterType("playerJetEmitter"));
                        leftJet->isJetEmitter = true;
                        leftJet->attachedToModel = source->controlling;
                        leftJet->nodeName = "Left_Foot";
                        for(unsigned int a = 0; a<common->users.size(); a++)
                            leftJet->sendToClient(common->users[a]->netRef);
                        source->leftJet = leftJet;

                        emitter *rightJet = common->addEmitter(common->getEmitterType("playerJetEmitter"));
                        rightJet->isJetEmitter = true;
                        rightJet->attachedToModel = source->controlling;
                        rightJet->nodeName = "Right_Foot";
                        for(unsigned int a = 0; a<common->users.size(); a++)
                            rightJet->sendToClient(common->users[a]->netRef);
                        source->rightJet = rightJet;
                    }
                    else
                    {
                        if(source->leftJet)
                        {
                            common->removeEmitter(source->leftJet);
                            source->leftJet = 0;
                        }
                        if(source->rightJet)
                        {
                            common->removeEmitter(source->rightJet);
                            source->rightJet = 0;
                        }
                    }
                }

                source->controlling->lastCamX = camDirX;
                source->controlling->lastCamY = camDirY;
                source->controlling->lastCamZ = camDirZ;
                source->controlling->lastControlMask = controlMask;
                source->controlling->lastLeftMouseDown = leftMouseDown;
                source->controlling->lastHeldSlot = invSlot;

                for(unsigned int a = 0; a<inventorySize; a++)
                {
                    if(source->controlling->holding[a])
                    {
                        source->controlling->holding[a]->setHidden(true);
                        source->controlling->holding[a]->setSwinging(false);
                    }
                }

                if(invSlot != -1)
                {
                    if(source->controlling->holding[invSlot])
                        source->controlling->holding[invSlot]->setHidden(false);
                }

                /*float yaw = atan2(camDirX,camDirZ);
                //std::cout<<source->playerID<<" controlling "<<source->controlling->serverID<<" command: "<<controlMask<<" "<<yaw<<"\n";
                bool playJumpSound = source->controlling->control(yaw,controlMask & 1,controlMask & 2,controlMask & 4,controlMask & 8,controlMask &16,common->physicsWorld,!source->prohibitTurning,common->useRelativeWalkingSpeed);

                if(playJumpSound)
                    common->playSound("Jump",pos.x(),pos.y(),pos.z());*/

                if(invSlot != -1 && ((SDL_GetTicks() - source->controlling->pickUpItemFireCooldownStart) > 250))
                {
                    if(source->controlling->holding[invSlot] && leftMouseDown)
                    {
                        btVector3 firePos = pos + btVector3(source->controlling->type->eyeOffsetX,source->controlling->type->eyeOffsetY,source->controlling->type->eyeOffsetZ);
                        weaponFire(source->controlling,source->controlling->holding[invSlot],firePos.x(),firePos.y(),firePos.z(),camDirX,camDirY,camDirZ);

                        source->controlling->holding[invSlot]->setSwinging(true);
                        //TODO: Yeah this is bad, but these tools will be moved to Lua soon
                        if(source->controlling->holding[invSlot]->heldItemType->uiName == "Hammer")
                        {
                            if(source->controlling->holding[invSlot]->lastFire+200 < SDL_GetTicks())
                            {
                                source->controlling->holding[invSlot]->lastFire = SDL_GetTicks();

                                btVector3 raystart = pos + btVector3(source->controlling->type->eyeOffsetX,source->controlling->type->eyeOffsetY,source->controlling->type->eyeOffsetZ);
                                btVector3 rayend = raystart + btVector3(camDirX,camDirY,camDirZ) * 30.0;
                                btCollisionWorld::ClosestRayResultCallback res(raystart,rayend);
                                common->physicsWorld->rayTest(raystart,rayend,res);

                                common->addEmitter(common->getEmitterType("hammerExplosionEmitter"),res.m_hitPointWorld.x(),res.m_hitPointWorld.y(),res.m_hitPointWorld.z());
                                common->addEmitter(common->getEmitterType("hammerSparkEmitter"),res.m_hitPointWorld.x(),res.m_hitPointWorld.y(),res.m_hitPointWorld.z());

                                btRigidBody *body = (btRigidBody*)res.m_collisionObject;
                                if(body)
                                {
                                    if(body->getUserIndex() == bodyUserIndex_brick)
                                    {
                                        if(body->getUserPointer())
                                        {
                                            brick *theBrick = (brick*)body->getUserPointer();
                                            if(theBrick->builtBy == (int)source->accountID || theBrick->builtBy == 0)
                                            {
                                                common->removeBrick(theBrick);
                                                return;
                                            }
                                        }
                                    }
                                    common->playSound("HammerHit",res.m_hitPointWorld.x(),res.m_hitPointWorld.y(),res.m_hitPointWorld.z());
                                }
                            }
                        }
                        else if(source->controlling->holding[invSlot]->heldItemType->uiName == "Wrench")
                        {
                            if(source->controlling->holding[invSlot]->lastFire+200 < SDL_GetTicks())
                            {
                                source->controlling->holding[invSlot]->lastFire = SDL_GetTicks();
                                btVector3 raystart = source->controlling->getWorldTransform().getOrigin() + btVector3(source->controlling->type->eyeOffsetX,source->controlling->type->eyeOffsetY,source->controlling->type->eyeOffsetZ);
                                btVector3 rayend = raystart + btVector3(camDirX,camDirY,camDirZ) * 30.0;
                                btCollisionWorld::ClosestRayResultCallback res(raystart,rayend);
                                res.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
                                res.m_collisionFilterMask = btBroadphaseProxy::AllFilter;
                                common->physicsWorld->rayTest(raystart,rayend,res);

                                common->addEmitter(common->getEmitterType("wrenchExplosionEmitter"),res.m_hitPointWorld.x(),res.m_hitPointWorld.y(),res.m_hitPointWorld.z());
                                common->addEmitter(common->getEmitterType("wrenchSparkEmitter"),res.m_hitPointWorld.x(),res.m_hitPointWorld.y(),res.m_hitPointWorld.z());

                                if(res.m_collisionObject)
                                {
                                    btRigidBody *body = (btRigidBody*)res.m_collisionObject;
                                    if(body->getUserIndex() == bodyUserIndex_brick && body->getUserPointer())
                                    {
                                        brick *theBrick = (brick*)body->getUserPointer();
                                        if((unsigned)theBrick->builtBy == source->accountID || source->hasLuaAccess || theBrick->builtBy == 0)
                                        {
                                            common->playSound("WrenchHit",rayend.x(),rayend.y(),rayend.z());
                                            source->lastWrenchedBrick = theBrick;

                                            if(theBrick->isWheel)
                                            {
                                                wheelBrick *wheel = (wheelBrick*)theBrick;

                                                packet data;
                                                data.writeUInt(packetType_forceWrenchDialog,packetTypeBits);
                                                data.writeBit(true);
                                                data.writeFloat(wheel->wheelSettings.breakForce);
                                                data.writeFloat(wheel->wheelSettings.steerAngle);
                                                data.writeFloat(wheel->wheelSettings.engineForce);
                                                data.writeFloat(wheel->wheelSettings.suspensionLength);
                                                data.writeFloat(wheel->wheelSettings.suspensionStiffness);
                                                data.writeFloat(wheel->wheelSettings.frictionSlip);
                                                data.writeFloat(wheel->wheelSettings.rollInfluence);
                                                data.writeFloat(wheel->wheelSettings.dampingCompression);
                                                data.writeFloat(wheel->wheelSettings.dampingRelaxation);
                                                client->send(&data,true);

                                                return;
                                            }

                                            if(theBrick->isSteeringWheel)
                                            {
                                                steeringBrick *wheel = (steeringBrick*)theBrick;

                                                packet data;
                                                data.writeUInt(packetType_forceWrenchDialog,packetTypeBits);
                                                data.writeBit(false);
                                                data.writeBit(true);
                                                data.writeFloat(wheel->steeringSettings.mass);
                                                data.writeFloat(wheel->steeringSettings.angularDamping);
                                                data.writeBit(!wheel->steeringSettings.realisticCenterOfMass);
                                                client->send(&data,true);

                                                return;
                                            }

                                            packet data;
                                            data.writeUInt(packetType_forceWrenchDialog,packetTypeBits);
                                            data.writeBit(false);
                                            data.writeBit(false);
                                            data.writeBit(theBrick->body ? true : false);
                                            data.writeBit(theBrick->printMask & 1);
                                            data.writeBit(theBrick->printMask & 2);
                                            data.writeBit(theBrick->printMask & 4);
                                            data.writeBit(theBrick->printMask & 8);
                                            data.writeBit(theBrick->printMask & 16);
                                            data.writeBit(theBrick->printMask & 32);
                                            data.writeUInt(theBrick->music,10);
                                            data.writeFloat(theBrick->musicPitch);
                                            data.writeString(theBrick->name);

                                            if(theBrick->attachedLight)
                                            {
                                                data.writeBit(true);
                                                data.writeFloat(theBrick->attachedLight->color.x());
                                                data.writeFloat(theBrick->attachedLight->color.y());
                                                data.writeFloat(theBrick->attachedLight->color.z());
                                                data.writeBit(theBrick->attachedLight->isSpotlight);
                                                if(theBrick->attachedLight->isSpotlight)
                                                {
                                                    data.writeFloat(theBrick->attachedLight->direction.w());
                                                    float yaw = atan2(theBrick->attachedLight->direction.x(),theBrick->attachedLight->direction.z());
                                                    float pitch = asin(theBrick->attachedLight->direction.y());
                                                    if(yaw < 0)
                                                        yaw += 6.28;
                                                    if(pitch < 0)
                                                        pitch += 6.28;
                                                    data.writeFloat(yaw);
                                                    data.writeFloat(pitch);
                                                }
                                            }
                                            else
                                                data.writeBit(false);

                                            client->send(&data,true);

                                            return;
                                        }
                                        else
                                        {
                                            source->bottomPrint("This brick was built by ID " + std::to_string(theBrick->builtBy) + "!",4000);
                                        }
                                    }
                                    else if(body->getUserIndex() == bodyUserIndex_builtCar && body->getUserPointer())
                                    {
                                        brickCar *theCar = (brickCar*)body->getUserPointer();
                                        source->lastWrenchedBrick = theCar->bricks[0];
                                        if(source->lastWrenchedBrick->isSteeringWheel && theCar->bricks.size() > 1)
                                            source->lastWrenchedBrick = theCar->bricks[1];

                                        packet data;
                                        data.writeUInt(packetType_forceWrenchDialog,packetTypeBits);
                                        data.writeBit(false);
                                        data.writeBit(false);
                                        data.writeBit(true);
                                        data.writeBit(source->lastWrenchedBrick->printMask & 1);
                                        data.writeBit(source->lastWrenchedBrick->printMask & 2);
                                        data.writeBit(source->lastWrenchedBrick->printMask & 4);
                                        data.writeBit(source->lastWrenchedBrick->printMask & 8);
                                        data.writeBit(source->lastWrenchedBrick->printMask & 16);
                                        data.writeBit(source->lastWrenchedBrick->printMask & 32);
                                        data.writeUInt(theCar->music,10);
                                        data.writeFloat(theCar->musicPitch);
                                        data.writeString(" ");
                                        client->send(&data,true);

                                        common->playSound("WrenchHit",rayend.x(),rayend.y(),rayend.z());
                                        return;
                                    }
                                }

                                common->playSound("WrenchMiss",rayend.x(),rayend.y(),rayend.z());
                            }
                        }
                    }
                }
            }
            if(source->driving)
            {
                bool speeding = source->driving->driveCar(controlMask & 1,controlMask & 2,controlMask & 4,controlMask & 8,controlMask &16);
                if(speeding)
                    source->bottomPrint("You have reached this car's max speed!",5000);
            }

            return;
        }
        case requestName:
        {
            int theirNetworkVersion = data->readUInt(32);

            if(theirNetworkVersion != hardCodedNetworkVersion)
            {
                if(theirNetworkVersion != quitRequestMagic)
                    info("Kicking client with outdated client version!");
                host->kick(client);
                return;
            }

            bool filesYesJoinNo = data->readBit();
            if(filesYesJoinNo)
            {
                info("Sending custom file list to client, files: " + std::to_string(common->customFiles.size()));

                packet commandFileAmount;
                commandFileAmount.writeUInt(packetType_serverCommand,packetTypeBits);
                commandFileAmount.writeString("numCustomFiles");
                commandFileAmount.writeUInt(common->customFiles.size(),16);
                source->netRef->send(&commandFileAmount,true);

                for(int a = 0; a<common->customFiles.size(); a++)
                {
                    packet commandFileDescription;
                    commandFileDescription.writeUInt(packetType_serverCommand,packetTypeBits);
                    commandFileDescription.writeString("customFileDescription");
                    commandFileDescription.writeString(common->customFiles[a].name);
                    commandFileDescription.writeString(common->customFiles[a].path);
                    commandFileDescription.writeUInt(common->customFiles[a].checksum,32);
                    commandFileDescription.writeUInt(common->customFiles[a].sizeBytes,32);
                    commandFileDescription.writeUInt(common->customFiles[a].type,4);
                    commandFileDescription.writeUInt(common->customFiles[a].id,16);
                    source->netRef->send(&commandFileDescription,true);
                }
                return;
            }

            source->stillInContentPhase = false;

            bool wantsLogin = data->readBit();
            if(wantsLogin)
            {
                std::string accountName = data->readString();
                std::string hashedSessionKey = data->readString();
                source->name = accountName;

                float red = data->readFloat();
                float green = data->readFloat();
                float blue = data->readFloat();
                source->preferredRed = red;
                source->preferredGreen = green;
                source->preferredBlue = blue;

                source->logInState = 1;

                source->authHandle = curl_easy_init();
                curl_easy_setopt(source->authHandle,CURLOPT_SSL_VERIFYHOST,0);
                curl_easy_setopt(source->authHandle,CURLOPT_SSL_VERIFYPEER,0);
                curl_easy_setopt(source->authHandle,CURLOPT_WRITEDATA,source);
                curl_easy_setopt(source->authHandle,CURLOPT_WRITEFUNCTION,getAuthResponse);
                std::string url = "https://dran.land/verifyPlayer.php";
                std::string args = "hash=" + hashedSessionKey + "&name=" + accountName;
                //std::cout<<"Using args: "<<args<<"\n";
                curl_easy_setopt(source->authHandle,CURLOPT_URL,url.c_str());
                //curl_easy_setopt(source->authHandle,CURLOPT_POSTFIELDS,args.c_str()); apparently this doesn't work for multi ffs
                curl_easy_setopt(source->authHandle,CURLOPT_POSTFIELDSIZE,args.length());
                curl_easy_setopt(source->authHandle,CURLOPT_COPYPOSTFIELDS,args.c_str());
                curl_multi_add_handle(common->curlHandle,source->authHandle);

                return;
            }
            else //Is guest
            {
                std::string name = data->readString();

                float red = data->readFloat();
                float green = data->readFloat();
                float blue = data->readFloat();

                if(common->hasNonoWord(name))
                    name = common->replaceNonoWords(name);

                bool validName = true;
                if(name.length() < 1)
                    validName = false;

                name = name + "_Guest";

                if(name.length() > 255)
                    validName = false;

                if(!validName)
                {
                    host->kick(client);
                    return;
                }

                if(name.length() > 32)
                    name = name.substr(0,32);

                for(unsigned int a = 0; a<common->users.size(); a++)
                {
                    if(common->users[a]->name == name)
                    {
                        validName = false;
                        break;
                    }
                }

                if(!validName)
                {
                    validName = true;
                    name = name + "_" + std::to_string(rand());
                }

                packet data;
                data.writeUInt(packetType_connectionRepsonse,packetTypeBits);
                data.writeBit(validName);
                data.writeUInt(getServerTime(),32);
                data.writeUInt(common->brickTypes->specialBrickTypes,10);
                data.writeUInt(common->dynamicTypes.size(),10);
                data.writeUInt(common->itemTypes.size(),10);
                client->send(&data,true);

                //Shouldn't ever trigger this and if we do it crashses anyway
                if(!source)
                    error("Player not yet created for new client!");

                info("New guest from " + client->getIP() + " now known as " + name + " their netID is " + std::to_string(common->lastPlayerID));

                source->name = name;
                source->playerID = common->lastPlayerID;
                common->lastPlayerID++;

                source->preferredRed = red;
                source->preferredGreen = green;
                source->preferredBlue = blue;

                for(int a = 0; a<common->users.size(); a++)
                {
                    packet data;
                    data.writeUInt(packetType_addOrRemovePlayer,packetTypeBits);
                    data.writeBit(true); // for player's list, not typing players list
                    data.writeBit(true); //add
                    data.writeString(name);
                    data.writeUInt(source->playerID,16);
                    common->users[a]->netRef->send(&data,true);
                }

                common->playSound("PlayerConnect");
                common->messageAll("[colour='FF0000FF']" + source->name + " connected.","server");

                sendInitalDataFirstHalf(common,source,client);

                clientJoin(source);

                return;
            }
        }
        case startLoadingStageTwo:
        {
            info("Client " + source->name + " started stage two of loading.");
            sendInitialDataSecondHalf(common,source,client);

            common->spawnPlayer(source,100,700,100);

            return;
        }
    }
}

#endif // RECEIVEDATA_H_INCLUDED
