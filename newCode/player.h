#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "code/newCode/Titem.h"

#define inventorySize 3

struct player : protected Tdynamic
{
    //friend objHolder<Tdynamic>;
    friend objHolder<player>;

    protected:

    explicit player(dynamicType *_type,const btVector3 &initialPos = btVector3(0,0,0));

    //Result of control method:
    float crouchProgress = 0.0;
    bool isJetting = false;
    unsigned int lastPlayerControl = 0;
    unsigned int lastWalked = 0;
    float lastCamX=1,lastCamY=0,lastCamZ=0;
    int lastControlMask = 0;
    bool lastLeftMouseDown = false;
    bool walking = false;

    //Ability to hold items
    int lastHeldSlot = 0;
    int lastPickedUpItemTime = 0;
    item *holding[inventorySize] = {0,0,0};

    //Vehicle:
    dynamic *sittingOn = 0;

    public:

    //Has this object been updated in such a way that we need to resend its properties to clients?
    bool requiresNetUpdate() const override;

    //Does not reset requiresUpdate, intended to be called by objHolder
    void addToUpdatePacketFinal(syj::packet * const data) override;

    //Networking to be done (along with other objects of this type) when a client joins the game or the object is created
    void addToCreationPacket(syj::packet * const data) const override;

    //How many bytes does addToUpdatePacket add to a syj::packet
    unsigned int getCreationPacketBits() const override;

    //How many bytes does addToUpdatePacket add to a syj::packet
    unsigned int getUpdatePacketBits() const override;

    /*
       Create a lua variable / return value that refers to this object
       This function cannot be const because lua needs a non const pointer
       It's worth noting this is a situation where the use of shared_ptr kind of breaks down
       Lua calls can still crash if they use an object that's been deleted after all shared_ptrs are out of scope
    */
    void pushLua(lua_State * const L) override;

    ~player();

    //Class specific methods:

};

#endif // PLAYER_H_INCLUDED
