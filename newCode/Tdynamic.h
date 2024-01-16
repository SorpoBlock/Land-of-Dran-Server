#ifndef TDYNAMIC_H_INCLUDED
#define TDYNAMIC_H_INCLUDED

#include "code/newCode/simObject.h"
#include "code/dynamic.h"

/*
    Dynamics are any physics object. All movable btRigidBody(s) except car chassis (for now) are owned by dynamics.
    Projectiles are dynamics with a flag set.
    Items (held by players) and Players (can hold items or drive cars) are derived classes from dynamics
*/

class Tdynamic : simObject
{
    friend objHolder<Tdynamic>;

    private:

    //Always assigned in constructor, contains collision shape and animations among other things
    dynamicType *type = 0;

    //Physics object handle, always created in constructor and destroyed in destructor
    btRigidBody *body = 0;

    //Combat stuff, even items can be destroyed:
    float health = 100.0;
    bool canTakeDamage = false;

    /*
        Projectiles are just dynamics with a flag set
        Projectiles rotate in the direction of their velocity each frame
        And they are destroyed upon hitting something, at which point an event is fired
        The tag is arbitrary and is set in Lua by the user
    */
    std::string projectileTag = "";
    bool isProjectile = false;

    //Used to see if we need to play a splash sound / show a splash emitter
    bool lastInWater = false;
    unsigned int lastSplashEffect = 0;

    //Used for avatar customization, but theoretically any object/item/player/projectile can be customized
    std::vector<std::string> nodeNames;
    std::vector<btVector3> nodeColors;
    int chosenDecal = 0;

    //Players and items are classes which inherit dynamic and set these flags
    bool isPlayer = false;
    bool isItem = false;

    //Shape names are text billboards above an object i.e. player names, by default they aren't on
    btVector3 shapeNameColor = btVector3(1.0,1.0,1.0);
    std::string shapeName = "";

    //Set by addToUpdatePacketFinal, no update needs to be sent to clients if object didn't move much since last update
    btVector3 lastPosition = btVector3(0.0,0.0,0.0);
    btQuaternion lastRotation = btQuaternion(0.0,0.0,0.0);
    unsigned int lastSendTransform = 0; //In MS since server start. So we can make sure even non-moving objects occasionally get sent

    protected:

    explicit Tdynamic(dynamicType *_type,btVector3 initialPos = btVector3(0,0,0));

    public:

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

    ~Tdynamic();

    //Start type specific functions:

    //Per frame calculations for non-players that make them float in water or splash
    void applyBuoyancy();

    //Per frame calculations if isProjectile is true that makes object rotation in direction of velocity
    void applyProjectileRotation();

    //Gets a packet that when sent to a client updates the node colors and decal texture selection
    void getAppearancePacket(packet * const data) const;

    //Sets the color of one node, getAppearancePacket result should be broadcast after any node changes in a given frame are completed
    void setNodeColor(const std::string &node,float r,float g,float b);

    //Same as setNodeColor, send getAppearancePacket after calling this to update clients
    void setDecal(unsigned int decalIdx);

    //Changes or removes the text above an object and broadcasts it to all connected clients
    void setShapeName(const std::string &text,float r,float g,float b,syj::server * const server);

    //Applies damage, objects by default have 100 health, does nothing if canTakeDamage is false, returns if the object has died / should be destroyed
    bool applyDamage(float amount,const std::string &cause = "");

    //Animation status is not tracked anywhere server-side, so there's no way to poll if an animation is playing or any guarentee what frame it is in
    //1.0 is normal speed, resetFrame sets the frame of that particular animation back to 0 if it was already playing
    void playAnimation(const std::string &animationName,float speed,bool resetFrame,bool loop = false);

    //Blank string stops all animations
    void stopAnimation(const std::string &animationName = "");
};

#endif // TDYNAMIC_H_INCLUDED
