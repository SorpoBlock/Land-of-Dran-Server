#include "quaternion.h"

unsigned char greatestOfQuat(btQuaternion quat)
{
    quat.setW(fabs(quat.getW()));
    quat.setX(fabs(quat.getX()));
    quat.setY(fabs(quat.getY()));
    quat.setZ(fabs(quat.getZ()));

    if(quat.w() > quat.x() && quat.w() > quat.y() && quat.w() > quat.z())
        return 0;
    if(quat.x() > quat.y() && quat.x() > quat.z())
        return 1;
    if(quat.y() > quat.z())
        return 2;
    return 3;
}

btQuaternion readQuat(packet *data)
{
    btQuaternion ret;

    int greatest = data->readUInt(2);
    bool flipGreatest = data->readBit();
    switch(greatest)
    {
        case 0:
        {
            float x = data->readUInt(9);
            float y = data->readUInt(9);
            float z = data->readUInt(9);

            ret.setX(((x / 512.0) * 1.414214) - 0.707107);
            ret.setY(((y / 512.0) * 1.414214) - 0.707107);
            ret.setZ(((z / 512.0) * 1.414214) - 0.707107);
            ret.setW( sqrt(1 - ret.x() * ret.x() - ret.y() * ret.y() - ret.z() * ret.z()) );
            if(flipGreatest)
                ret.setW(-ret.getW());

            return ret;
        }
        case 1:
        {
            float w = data->readUInt(9);
            float y = data->readUInt(9);
            float z = data->readUInt(9);

            ret.setW(((w / 512.0) * 1.414214) - 0.707107);
            ret.setY(((y / 512.0) * 1.414214) - 0.707107);
            ret.setZ(((z / 512.0) * 1.414214) - 0.707107);
            ret.setX( sqrt(1 - ret.w() * ret.w() - ret.y() * ret.y() - ret.z() * ret.z()) );
            if(flipGreatest)
                ret.setX(-ret.getX());

            return ret;
        }
        case 2:
        {
            float w = data->readUInt(9);
            float x = data->readUInt(9);
            float z = data->readUInt(9);

            ret.setW(((w / 512.0) * 1.414214) - 0.707107);
            ret.setX(((x / 512.0) * 1.414214) - 0.707107);
            ret.setZ(((z / 512.0) * 1.414214) - 0.707107);
            ret.setY( sqrt(1 - ret.w() * ret.w() - ret.x() * ret.x() - ret.z() * ret.z()) );
            if(flipGreatest)
                ret.setY(-ret.getY());

            return ret;
        }
        case 3:
        {
            float w = data->readUInt(9);
            float x = data->readUInt(9);
            float y = data->readUInt(9);

            ret.setW(((w / 512.0) * 1.414214) - 0.707107);
            ret.setX(((x / 512.0) * 1.414214) - 0.707107);
            ret.setY(((y / 512.0) * 1.414214) - 0.707107);
            ret.setZ( sqrt(1 - ret.w() * ret.w() - ret.x() * ret.x() - ret.y() * ret.y()) );
            if(flipGreatest)
                ret.setZ(-ret.getZ());

            return ret;
        }
    }

    error("Invalid quaternion received in packet.");
    return btQuaternion::getIdentity();
}

//#define debugQuatWriter

void writeQuat(btQuaternion quat,packet *toWrite)
{
    int greatest = greatestOfQuat(quat);
    switch(greatest)
    {
        case 0:
        {
            /*if(quat.w() < 0)
                quat = btQuaternion(-1,-1,-1,-1) * quat;
            quat = quat.normalized();*/

            float x = ((quat.x() + 0.707107) / 1.414214) * 512.0;
            float y = ((quat.y() + 0.707107) / 1.414214) * 512.0;
            float z = ((quat.z() + 0.707107) / 1.414214) * 512.0;

            toWrite->writeUInt(greatest,2);
            toWrite->writeBit(quat.w() < 0);
            toWrite->writeUInt(x,9);
            toWrite->writeUInt(y,9);
            toWrite->writeUInt(z,9);

            break;
        }

        case 1:
        {
            /*if(quat.x() < 0)
                quat = btQuaternion(-1,-1,-1,-1) * quat;
            quat = quat.normalized();*/

            float w = ((quat.w() + 0.707107) / 1.414214) * 512.0;
            float y = ((quat.y() + 0.707107) / 1.414214) * 512.0;
            float z = ((quat.z() + 0.707107) / 1.414214) * 512.0;

            toWrite->writeUInt(greatest,2);
            toWrite->writeBit(quat.x() < 0);
            toWrite->writeUInt(w,9);
            toWrite->writeUInt(y,9);
            toWrite->writeUInt(z,9);

            break;
        }
        case 2:
        {
            /*if(quat.y() < 0)
                quat = btQuaternion(-1,-1,-1,-1) * quat;
            quat = quat.normalized();*/

            float w = ((quat.w() + 0.707107) / 1.414214) * 512.0;
            float x = ((quat.x() + 0.707107) / 1.414214) * 512.0;
            float z = ((quat.z() + 0.707107) / 1.414214) * 512.0;

            toWrite->writeUInt(greatest,2);
            toWrite->writeBit(quat.y() < 0);
            toWrite->writeUInt(w,9);
            toWrite->writeUInt(x,9);
            toWrite->writeUInt(z,9);

            break;
        }
        case 3:
        {
            /*if(quat.z() < 0)
                quat = btQuaternion(-1,-1,-1,-1) * quat;
            quat = quat.normalized();*/

            float w = ((quat.w() + 0.707107) / 1.414214) * 512.0;
            float x = ((quat.x() + 0.707107) / 1.414214) * 512.0;
            float y = ((quat.y() + 0.707107) / 1.414214) * 512.0;

            toWrite->writeUInt(greatest,2);
            toWrite->writeBit(quat.z() < 0);
            toWrite->writeUInt(w,9);
            toWrite->writeUInt(x,9);
            toWrite->writeUInt(y,9);

            break;
        }
    }

    #ifdef debugQuatWriter

    toWrite->seek(toWrite->getStreamPos() - 30);
    btQuaternion res = readQuat(toWrite);
    std::cout<<"Diff: " << (res-quat).length() <<" \n";

    #endif // debugQuatWriter

}




