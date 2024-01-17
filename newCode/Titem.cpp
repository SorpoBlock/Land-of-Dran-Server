#include "Titem.h"

Titem::Titem(itemType *_type,const btVector3 &initialPos)
    : Tdynamic(_type->model,initialPos)
{

}

//Has this object been updated in such a way that we need to resend its properties to clients?
bool Titem::requiresNetUpdate() const
{
    return Tdynamic::requiresNetUpdate();
}

//Does not reset requiresUpdate, intended to be called by objHolder
void Titem::addToUpdatePacketFinal(syj::packet * const data)
{
    Tdynamic::addToUpdatePacketFinal(data);
}

//Networking to be done (along with other objects of this type) when a client joins the game or the object is created
void Titem::addToCreationPacket(syj::packet * const data) const
{
    Tdynamic::addToCreationPacket(data);
}

//How many bytes does addToUpdatePacket add to a syj::packet
unsigned int Titem::getCreationPacketBits() const
{
    return Tdynamic::getCreationPacketBits();
}

//How many bytes does addToUpdatePacket add to a syj::packet
unsigned int Titem::getUpdatePacketBits() const
{
    return Tdynamic::getUpdatePacketBits();
}

/*
   Create a lua variable / return value that refers to this object
   This function cannot be const because lua needs a non const pointer
   It's worth noting this is a situation where the use of shared_ptr kind of breaks down
   Lua calls can still crash if they use an object that's been deleted after all shared_ptrs are out of scope
*/
void Titem::pushLua(lua_State * const L)
{
    Tdynamic::pushLua(L);
}

Titem::~Titem()
{

}
