#ifndef SIMOBJECT_H_INCLUDED
#define SIMOBJECT_H_INCLUDED

#include "code/newCode/objHolder.h"
#include "code/base/server.h"

//TODO: Put in bulletIncludes.h and include that
#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletCollision/CollisionShapes/btTriangleMesh.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

class simObject
{
    friend objHolder<simObject>;

    protected:

    //objHolder will call sendToAll after this for you, don't sendToAll it in the constructor
    explicit simObject();

    uint32_t creationTime = 0;
    netIDtype netID = 0;
    bool requiresUpdate = false;

    public:

    //Yes, global scope is bad and all but we really can only have one dynamicsWorld and it's needed constantly by everything
    //The methods I used to use to get around making this global were honestly far worse
    static btDynamicsWorld *world;

    //Has this object been updated in such a way that we need to resend its properties to clients?
    bool requiresNetUpdate() const;

    //objHolder should call this if requiresNetUpdate is true, resets requiresUpdate
    void addToUpdatePacket(syj::packet * const data);

    //Get netID
    netIDtype getID() const;

    //Time in MS since program start
    uint32_t getCreationTime() const;

    //does not reset requiresUpdate
    virtual void addToUpdatePacketFinal(syj::packet * const data) = 0;

    //How many bytes does addToUpdatePacket add to a syj::packet
    virtual unsigned int getUpdatePacketBits() const = 0;

    //Networking to be done (along with other objects of this type) when a client joins the game or the object is created
    virtual void addToCreationPacket(syj::packet * const data) const = 0;

    //How many bytes does addToCreationPacket add to a syj::packet
    virtual unsigned int getCreationPacketBits() const = 0;

    /*
       Create a lua variable / return value that refers to this object
       This function cannot be const because lua needs a non const pointer
       It's worth noting this is a situation where the use of shared_ptr kind of breaks down
       Lua calls can still crash if they use an object that's been deleted after all shared_ptrs are out of scope
    */
    virtual void pushLua(lua_State * const L) = 0;

    ~simObject();
};

#endif // SIMOBJECT_H_INCLUDED
