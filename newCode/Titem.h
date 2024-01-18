#ifndef TITEM_H_INCLUDED
#define TITEM_H_INCLUDED

#include "code/item.h"
#include "code/newCode/Tdynamic.h"

struct Titem : protected Tdynamic
{
    //friend objHolder<Tdynamic>;
    friend objHolder<Titem>;

    protected:

    explicit Titem(itemType *_type,const btVector3 &initialPos = btVector3(0,0,0));

    itemType *itemProperties = 0;

    //The player that's holding these must remember to update these if they drop this item!
    bool isBeingHeld = false;
    unsigned int holderNetID = 0;

    bool isHidden = false;
    bool isSwinging = false;

    //Related to behavior when an item is fired:

    //Should we call a raycast hit event when the player clicks ("fires")
    bool performRaycast = false;

    //Note other than sending info to clients, server doesn't do much with these values

    //Animation
    int nextFireAnim = -1;
    float nextFireAnimSpeed = 1.0;

    //Sound
    int nextFireSound = -1;
    float nextFireSoundPitch = 1.0;
    float nextFireSoundGain = 1.0;

    //Where on the model the emitter should be spawned, if not its origin
    std::string nextFireEmitterMesh = "";
    int nextFireEmitter = -1;

    //Milliseconds between firings
    int fireCooldownMS = 0;

    //bullet trail
    bool useBulletTrail = true;
    btVector3 bulletTrailColor = btVector3(1.0,0.5,0.0);
    float bulletTrailSpeed = 1.0;

    //End weapon fire settings

    //Compared to fireCooldownMS and SDL_GetTicks
    int lastFireEvent = 0;

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

    ~Titem();

    //Class specific methods:

    void removeHolder();
    void addHolder(unsigned int netID);

    void setHidden(bool value);
    void setSwinging(bool value);

    std::string getTypeName() const;

    //Item fire behavior:
    void setFireAnim(syj::server * const server,const std::string &animName,float animSpeed);
    void setFireSound(syj::server * const server,int soundIdx,float pitch,float gain);
    void setFireEmitter(syj::server * const server,int emitterIdx,const std::string &emitterMesh = "");
    void setItemCooldown(syj::server * const server,int milliseconds);
    void setItemBulletTrail(syj::server * const server,bool use,btVector3 color,float speed);
    void setPerformRaycast(bool toggle);
};

#endif // TITEM_H_INCLUDED


















