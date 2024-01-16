#ifndef TLIGHT_H_INCLUDED
#define TLIGHT_H_INCLUDED

#include "code/newCode/simObject.h"

class Tlight : simObject
{
    friend objHolder<Tlight>;

    private:

    //Any fixed-length data about this object that needs to be sent to clients
    //Easy conversion to a char[] -> length prefixed string - > syj::packet
    union
    {
        struct
        {
            //attachmentMode == simObjectNone if light is not attached to anything
            simObjectType attachmentMode;
            uint32_t attachedID;

            float red,green,blue;

            //Can be world position, or offset relative to something it is attached to:
            float x,y,z;

            //dir is straight forward direction spotlight faces, phi is width of the spotlight beam
            float dirX,dirY,dirZ,phi;

            //How fast the color of each component changes according to a wave function, 0 is no change, sign determines cos/sin
            float blinkR,blinkG,blinkB;

            //For spinning spotlights
            float yawVel;

            bool isSpotlight;

            uint16_t lifeTimeMS;
        } netInfo;

        char buffer[sizeof(netInfo)];
    };

    //Variable length, can't go in netInfo
    std::string mountPoint = "";

    protected:

    explicit Tlight();

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

    ~Tlight();

    //Start type specific functions:

    //Basic setters, that will mostly be used by Lua calls
    void setPhi(float _phi);
    void setSpotlightDirection(float _x,float _y,float _z);
    void setBlinkSpeed(float _r,float _g,float _b);
    void setLightColor(float _r,float _g,float _b);
    void setYawVel(float _yawVel);
};

#endif // TLIGHT_H_INCLUDED
