#include "rope.h"

void rope::sendToClient(serverClientHandle *netRef)
{
    packet data;
    data.writeUInt(packetType_addRemoveRope,packetTypeBits);
    data.writeBit(true); //add
    data.writeUInt(serverID,12);
    data.writeUInt(handle->m_nodes.size(),8);
    netRef->send(&data,true);
}

void rope::removeFromClient(serverClientHandle *netRef)
{
    packet data;
    data.writeUInt(packetType_addRemoveRope,packetTypeBits);
    data.writeBit(false); //remove
    data.writeUInt(serverID,12);
    netRef->send(&data,true);
}

void rope::addToPacket(packet *data)
{
    data->writeUInt(serverID,12);
    data->writeUInt(handle->m_nodes.size(),8);
    for(int a = 0; a<handle->m_nodes.size(); a++)
    {
        btSoftBody::Node n = handle->m_nodes.at(a);
        data->writeFloat(n.m_x.x());
        data->writeFloat(n.m_x.y());
        data->writeFloat(n.m_x.z());
    }
}
