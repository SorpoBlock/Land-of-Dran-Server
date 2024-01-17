#ifndef TDYNAMIC_H_INCLUDED
#define TDYNAMIC_H_INCLUDED

#include "code/newCode/simObject.h"
#include "code/dynamic.h"

/*
    Dynamics are any physics object. All movable btRigidBody(s) except car chassis (for now) are owned by dynamics.
    Projectiles are dynamics with a flag set.
    Items (held by players) and Players (can hold items or drive cars) are derived classes from dynamics
*/

/*
    for packetType_clientPhysicsData
    These types of packets tell a client to ignore server simulation and only simulate the object locally
    This is used for the client's player at the moment
    0 is used when a client gets assigned a player (ex. they spawn, or get out of a car)
    1 is used when they get in a car or otherwise lose control of the player directly
    2 is used when they either get a new player, or their player is destroyed (i.e. they die)
    0 can also be used in response to lua calls that change a player's position or velocity, to force an update clientside
*/
enum clientPhysicsPacketType
{
    startClientPhysics = 0,
    suspendClientPhysics = 1,
    destroyClientPhysics = 2
};

//Maximum of 1024 different types of dynamics / models
#define dynamicTypeIDBits 10

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
    unsigned int lastSendTransformTime = 0; //In MS since server start. So we can make sure even non-moving objects occasionally get sent

    protected:

    explicit Tdynamic(dynamicType *_type,const btVector3 &initialPos = btVector3(0,0,0));

    public:

    //Has this object been updated in such a way that we need to resend its properties to clients?
    bool requiresNetUpdate() const override;

    //Does not reset requiresUpdate, intended to be called by objHolder
    virtual void addToUpdatePacketFinal(syj::packet * const data) override;

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

    /*
        This sends a packet that contains exact lin. vel., ang. vel. transform and collision bounds data
        When a client receives this packet it will start or resume simulating physics client-side for this object instead of server-side
        Further server-side updates (say, teleporting a client's player) require sending another one of these to the client
        There is a different kind of packet you send using just a client member function that pauses this, i.e. player entered a car
    */
    void sendClientPhysics(syj::serverClientHandle * const target) const;
    void pauseClientPhysics(syj::serverClientHandle * const target) const;
    void stopClientPhysics(syj::serverClientHandle * const target) const;

    //Per frame calculations for non-players that make them float in water or splash
    void applyBuoyancy(float waterLevel);

    //Per frame calculations if isProjectile is true that makes object rotation in direction of velocity
    void applyProjectileRotation();

    //Gets a packet that when sent to a client updates the node colors and decal texture selection
    //Returns false if the packet isn't needed (no custom appearance, packet not written to)
    bool getAppearancePacket(packet * const data) const;

    //Sets the color of one node, getAppearancePacket result should be broadcast after any node changes in a given frame are completed
    void setNodeColor(const std::string &node,const btVector3 &color);

    //Same as setNodeColor, send getAppearancePacket after calling this to update clients
    void setDecal(unsigned int decalIdx);

    //Changes or removes the text above an object and broadcasts it to all connected clients
    void setShapeName(syj::server * const server,const std::string &text,float r,float g,float b);

    //Applies damage, objects by default have 100 health, does nothing if canTakeDamage is false
    //returns if the object has died / should be destroyed
    bool applyDamage(syj::server * const server,float amount);

    //Animation status is not tracked anywhere server-side, so there's no way to poll if an animation is playing or any guarentee what frame it is in
    //1.0 is normal speed, resetFrame sets the frame of that particular animation back to 0 if it was already playing
    void playAnimation(syj::server * const server,const std::string &animationName,float speed,bool resetFrame,bool loop = false);

    //Blank string stops all animations
    void stopAnimation(syj::server * const server,const std::string &animationName = "");
};

#endif // TDYNAMIC_H_INCLUDED
