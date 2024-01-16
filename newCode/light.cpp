#include "light.h"

Tlight::Tlight()
{

}

Tlight::~Tlight()
{

}

void Tlight::addToCreationPacket(syj::packet *data) const
{
    data->writeUInt(netID,netIDbits);
    data->writeString(std::string(buffer,sizeof(buffer)));
    //mountPoint is the name of the node on the model we attached to i.e. "hand" or "face"
    if(netInfo.attachmentMode == simObjectDynamic || netInfo.attachmentMode == simObjectItem)
        data->writeString(mountPoint);
}

unsigned int Tlight::getUpdatePacketBits() const
{
    if(netInfo.attachmentMode == simObjectDynamic || netInfo.attachmentMode == simObjectItem)
        return (netIDbits + sizeof(buffer) + mountPoint.length() + 1) * 8; //The +1 is the length prefix for the string
    else
        return (netIDbits + sizeof(buffer)) * 8;
}

unsigned int Tlight::getCreationPacketBits() const
{
    if(netInfo.attachmentMode == simObjectDynamic || netInfo.attachmentMode == simObjectItem)
        return (netIDbits + sizeof(buffer) + mountPoint.length() + 1) * 8; //The +1 is the length prefix for the string
    else
        return (netIDbits + sizeof(buffer)) * 8;
}

void Tlight::addToUpdatePacketFinal(syj::packet * const data)
{
    data->writeUInt(netID,netIDbits);
    data->writeString(std::string(buffer,sizeof(buffer)));
    if(netInfo.attachmentMode == simObjectDynamic || netInfo.attachmentMode == simObjectItem)
        data->writeString(mountPoint);
}

void Tlight::pushLua(lua_State * const L)
{
    lua_newtable(L);
    lua_getglobal(L,"lightMETATABLE");
    lua_setmetatable(L,-2);
    lua_pushinteger(L,netID);
    lua_setfield(L,-2,"id");
    lua_pushlightuserdata(L,this);
    lua_setfield(L,-2,"pointer");
}

void Tlight::setPhi(float _phi)
{
    requiresUpdate = true;

    netInfo.phi = fabs(_phi);

    if(netInfo.phi >= 6.2831855)
        netInfo.isSpotlight = false;
    else
        netInfo.isSpotlight = true;
}

void Tlight::setSpotlightDirection(float _x,float _y,float _z)
{
    requiresUpdate = true;

    netInfo.dirX = _x;
    netInfo.dirY = _y;
    netInfo.dirZ = _z;
}

void Tlight::setBlinkSpeed(float _r,float _g,float _b)
{
    requiresUpdate = true;

    netInfo.blinkR = _r;
    netInfo.blinkG = _g;
    netInfo.blinkB = _b;
}

void Tlight::setLightColor(float _r,float _g,float _b)
{
    requiresUpdate = true;

    netInfo.red = _r;
    netInfo.green = _g;
    netInfo.blue = _b;
}

void Tlight::setYawVel(float _yawVel)
{
    requiresUpdate = true;

    netInfo.yawVel = _yawVel;
}




















