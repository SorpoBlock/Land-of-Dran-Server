#ifndef AABBCALLBACK_H_INCLUDED
#define AABBCALLBACK_H_INCLUDED

#include "code/livingBrick.h"
#include "code/lua/events.h"

struct AllButMeAabbResultCallback : public btBroadphaseAabbCallback
{
    AllButMeAabbResultCallback(btRigidBody* me) : me(me)
    {

    }

    virtual bool process(const btBroadphaseProxy* proxy)
    {
        btCollisionObject* collisionObject = static_cast<btCollisionObject*>(proxy->m_clientObject);
        if (collisionObject == this->me)
        {
            return true;
        }
        results.push_back(collisionObject);
        return true;
    }

    btRigidBody* me;
    std::vector<btCollisionObject*> results;
};

void clientDrewSelection(btVector3 start,btVector3 end,unifiedWorld *common,clientData *source)
{
    if(!source->controlling)
        return;

    AllButMeAabbResultCallback callback(source->controlling);
    common->physicsWorld->getBroadphase()->aabbTest(start,end,callback);

    if(callback.results.size() <= 0)
        return;

    info(source->name+" is slicing: "+std::to_string(callback.results.size())+" bricks");

    bool giveNotice = false;
    int yourBricks = 0;

    for(unsigned int a = 0; a<callback.results.size(); a++)
    {
        btCollisionObject *obj = callback.results[a];

        if(obj->getUserIndex() != bodyUserIndex_brick)
            continue;
        if(!obj->getUserPointer())
            continue;

        brick *theBrick = (brick*)obj->getUserPointer();

        if(theBrick->builtBy != (int)source->accountID)
        {
            giveNotice = true;
            break;
        }
        else
            yourBricks++;
    }

    if(giveNotice)
        source->bottomPrint("You can only slice your own bricks!",7000);

    if(yourBricks > maxCarBricks)
    {
        info(std::to_string(yourBricks) + " of those bricks were theirs.");
        source->bottomPrint("Car brick limit reached: [colour='FFFF0000']" + std::to_string(yourBricks) + "[colour='FFFFFFFF']/" + std::to_string(maxCarBricks),7000);
        return;
    }

    brickCar *car = new brickCar;
    car->ownerID = source->playerID;
    int additionalCarBricks = 0;

    for(unsigned int a = 0; a<callback.results.size(); a++)
    {
        btCollisionObject *obj = callback.results[a];

        if(obj->getUserIndex() != bodyUserIndex_brick)
            continue;
        if(!obj->getUserPointer())
            continue;

        brick *theBrick = (brick*)obj->getUserPointer();

        if(theBrick->builtBy != (int)source->accountID)
            continue;

        brick *copyOfTheBrick = 0;

        if(theBrick->isWheel)
        {
            copyOfTheBrick = new wheelBrick;
            copyOfTheBrick->isWheel = true;
            ((wheelBrick*)copyOfTheBrick)->wheelSettings = ((wheelBrick*)theBrick)->wheelSettings;
        }
        else if(theBrick->isSteeringWheel)
        {
            copyOfTheBrick = new steeringBrick;
            copyOfTheBrick->isSteeringWheel = true;
            ((steeringBrick*)copyOfTheBrick)->steeringSettings = ((steeringBrick*)theBrick)->steeringSettings;
        }
        else
        {
            additionalCarBricks++;
            copyOfTheBrick = new brick;
        }

        copyOfTheBrick->posX = theBrick->posX;
        copyOfTheBrick->uPosY = theBrick->uPosY;
        copyOfTheBrick->posZ = theBrick->posZ;
        copyOfTheBrick->xHalfPosition = theBrick->xHalfPosition;
        copyOfTheBrick->yHalfPosition = theBrick->yHalfPosition;
        copyOfTheBrick->zHalfPosition = theBrick->zHalfPosition;
        copyOfTheBrick->r = theBrick->r;
        copyOfTheBrick->g = theBrick->g;
        copyOfTheBrick->b = theBrick->b;
        copyOfTheBrick->a = theBrick->a;
        copyOfTheBrick->angleID = theBrick->angleID;
        copyOfTheBrick->material = theBrick->material;
        copyOfTheBrick->isSpecial = theBrick->isSpecial;
        copyOfTheBrick->width = theBrick->width;
        copyOfTheBrick->height = theBrick->height;
        copyOfTheBrick->length = theBrick->length;
        copyOfTheBrick->typeID = theBrick->typeID;
        if(theBrick->attachedLight)
        {
            std::cout<<"Car in slice has attached light!\n";
            copyOfTheBrick->attachedLight = theBrick->attachedLight;
            theBrick->attachedLight->attachedBrick = copyOfTheBrick;
            theBrick->attachedLight = 0;
        }
        car->bricks.push_back(copyOfTheBrick);
    }

    if(car->bricks.size() < 1 || additionalCarBricks < 1)
        delete car;
    else
    {
        bool cancelled = clientTryLoadCarEvent(source,car,true);

        if(cancelled)
        {
            delete car;
        }
        else
        {
            float dummyHeightCorrection;
            if(!common->compileBrickCar(car,dummyHeightCorrection))
            {
                source->bottomPrint("Wheels must face the same direction, and you need *one* steering wheel!",5000);
                for(unsigned int a = 0; a<car->bricks.size(); a++)
                    delete car->bricks[a];
                car->bricks.clear();
                delete car;
            }
            else
            {
                for(unsigned int a = 0; a<callback.results.size(); a++)
                {
                    btCollisionObject *obj = callback.results[a];

                    if(obj->getUserIndex() != bodyUserIndex_brick)
                        continue;
                    if(!obj->getUserPointer())
                        continue;

                    brick *theBrick = (brick*)obj->getUserPointer();
                    if(theBrick->builtBy != (int)source->accountID)
                        continue;
                    common->removeBrick(theBrick);
                }
            }
        }
    }
}

#endif // AABBCALLBACK_H_INCLUDED
