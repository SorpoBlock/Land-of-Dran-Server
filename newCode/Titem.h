#ifndef TITEM_H_INCLUDED
#define TITEM_H_INCLUDED

#include "code/newCode/Tdynamic.h"
#include "code/item.h"

struct Titem : Tdynamic
{
    //friend objHolder<Tdynamic>;
    friend objHolder<Titem>;

    private:



    protected:

    explicit Titem(itemType *_type,const btVector3 &initialPos = btVector3(0,0,0));

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
};

#endif // TITEM_H_INCLUDED
