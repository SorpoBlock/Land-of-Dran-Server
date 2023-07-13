#ifndef QUATERNION_H_INCLUDED
#define QUATERNION_H_INCLUDED

#include "code/base/packet.h"
#define BT_USE_DOUBLE_PRECISION
#include <btBulletDynamicsCommon.h>
#include "code/base/logger.h"

using namespace syj;

unsigned char greatestOfQuat(btQuaternion quat);
btQuaternion readQuat(packet *data);
void writeQuat(btQuaternion quat,packet *toWrite);

#endif // QUATERNION_H_INCLUDED
