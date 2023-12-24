#include "brick.h"

//Be sure to deallocate packets afterwards:
void addBrickPacketsFromVector(std::vector<brick*> toAdd,std::vector<packet*> &result)
{
    int bricksToSend = toAdd.size();
    int bricksSentSoFar = 0;
    while(bricksToSend > 0)
    {
        int sentThisTime = 0;
        int bitsLeft = packetMTUbits - (packetTypeBits + 8);
        while(bitsLeft > 0 && (sentThisTime < bricksToSend))
        {
            brick *tmp = toAdd[sentThisTime + bricksSentSoFar];
            bitsLeft -= tmp->getPacketBits();
            sentThisTime++;
        }

        packet *data = new packet;
        data->writeUInt(packetType_addBricks,packetTypeBits);
        data->writeUInt(sentThisTime,8);
        for(int a = bricksSentSoFar; a<bricksSentSoFar+sentThisTime; a++)
            toAdd[a]->addToPacket(data);

        result.push_back(data);

        bricksToSend -= sentThisTime;
        bricksSentSoFar += sentThisTime;
    }
}

//Be sure to deallocate packets afterwards:
void removeBrickPacketsFromVector(std::vector<brick*> toRemove,std::vector<packet*> &result)
{

}

void brick::calcGridFromCarPos(btVector3 carOrigin)
{

    //uPosY -= height/2;
    //yHalfPosition = (((int)height) % 2);

    if(angleID % 2 == 0)
    {
        xHalfPosition = (((int)width) % 2);
        zHalfPosition = (((int)length) % 2);
    }
    else
    {
        zHalfPosition = (((int)width) % 2);
        xHalfPosition = (((int)length) % 2);
    }

    float newCarX = carX - (xHalfPosition ? 0.5 : 0.0);
    //float newCarY = carY - (yHalfPosition ? 0.5 : 0.0);
    float newCarZ = carZ - (zHalfPosition ? 0.5 : 0.0);

    posX = std::floor(newCarX) + std::round(carOrigin.x());
    //uPosY = std::round(newCarY/0.4) + std::round(carOrigin.y()/0.4);
    posZ = std::floor(newCarZ) + std::round(carOrigin.z());

    uPosY = std::round(carOrigin.y()/0.4) + carPlatesUp;

    //This is a hack idk why it's needed
    /*if(height % 2 == 0)
    {
        std::cout<<carY<<" carY "<<uPosY<<" uPosY\n";
        uPosY--;
    }*/
    /*if(height % 2 == 1 && uPosY % 2 == 0)
    {
        std::cout<<carY<<" carY "<<uPosY<<" uPosY\n";
        uPosY--;
    }*/
}

void brick::createUpdatePacket(packet *data)
{
    data->writeUInt(packetType_updateBrick,packetTypeBits);
    data->writeUInt(serverID,20);

    unsigned int nr = r*255;
    data->writeUInt(nr,8);
    unsigned int ng = g*255;
    data->writeUInt(ng,8);
    unsigned int nb = b*255;
    data->writeUInt(nb,8);
    unsigned int na = a*255;
    data->writeUInt(na,8);

    data->writeUInt(posX+8192,14);
    data->writeBit(xHalfPosition);
    data->writeUInt(uPosY,16);
    data->writeBit(yHalfPosition);
    data->writeUInt(posZ+8192,14);
    data->writeBit(zHalfPosition);
    data->writeUInt(angleID,2);
    int tmpMat = material;
    int shapeFx = 0;
    if(tmpMat >= bob)
    {
        tmpMat -= bob;
        shapeFx += 2;
    }
    else if(tmpMat >= undulo)
    {
        tmpMat -= undulo;
        shapeFx += 1;
    }
    data->writeUInt(shapeFx,4);
    data->writeUInt(tmpMat,4);
    data->writeBit(isColliding());

    if(printMask != 0 && printID != -1 && printName.length() > 0)
    {
        data->writeBit(true);
        data->writeUInt(printMask,6);
        data->writeString(printName);
    }
    else
        data->writeBit(false);
}

void brickType::initModTerrain(const aiScene *scene)
{
    modTerShape = new btConvexHullShape();
    btVector3 minDim(9999,9999,9999),maxDim(-9999,-9999,-9999);

    aiMesh *src = scene->mMeshes[0];
    for(int a = 0; a<src->mNumVertices; a++)
    {
        if(src->mVertices[a].x > maxDim.x())
            maxDim.setX(src->mVertices[a].x);
        if(src->mVertices[a].y > maxDim.y())
            maxDim.setY(src->mVertices[a].y);
        if(src->mVertices[a].z > maxDim.z())
            maxDim.setZ(src->mVertices[a].z);

        if(src->mVertices[a].x < minDim.x())
            minDim.setX(src->mVertices[a].x);
        if(src->mVertices[a].y < minDim.y())
            minDim.setY(src->mVertices[a].y);
        if(src->mVertices[a].z < minDim.z())
            minDim.setZ(src->mVertices[a].z);

        modTerShape->addPoint(btVector3(src->mVertices[a].x,src->mVertices[a].y,src->mVertices[a].z),false);
    }

    modTerShape->recalcLocalAabb();
    isModTerrain = true;

    width = ceil(maxDim.x() - minDim.x());
    height = ceil(maxDim.y() - minDim.y())*2.5;
    length = ceil(maxDim.z() - minDim.z());
}

void brickType::initModTerrain(std::string blbFile)
{
    modTerShape = new btConvexHullShape();
    btVector3 minDim(9999,9999,9999),maxDim(-9999,-9999,-9999);

    std::ifstream file(blbFile.c_str());
    if(!file.is_open())
    {
        error("Could not open mod terrain blb file " + blbFile);
        return;
    }

    bool readingVerts = false;
    std::string line;
    std::vector<std::string> words;
    int vertsReadSincePositionLine = 0;
    while(!file.eof())
    {
        getline(file,line);

        if(!readingVerts)
        {
            if(line.find("POSITION:") != std::string::npos)
                readingVerts = true;
        }
        else
        {
            if(line.find("POSITION:") != std::string::npos)
            {
                error("Found POSITION: in mod terrain blb file before a full quad had been specified.");
                continue;
            }

            if(line.length() > 2)
            {
                split(line,words);
                if(words.size() == 3)
                {
                    float x = atof(words[0].c_str());
                    float y = atof(words[1].c_str());
                    float z = atof(words[2].c_str());

                    std::swap(z,y);
                    y /= 2.5;

                    if(x > maxDim.x())
                        maxDim.setX(x);
                    if(y > maxDim.y())
                        maxDim.setY(y);
                    if(z > maxDim.z())
                        maxDim.setZ(z);

                    if(x < minDim.x())
                        minDim.setX(x);
                    if(y < minDim.y())
                        minDim.setY(y);
                    if(z < minDim.z())
                        minDim.setZ(z);

                    if(isnanf(x) || isnanf(y) || isnanf(z))
                    {
                        error("Invalid coordinate for mod terrain blb file " + blbFile + " | " + line);
                        continue;
                    }

                    btVector3 vec = btVector3(x,y,z);

                    if(vec.length() > 100)
                    {
                        error("Invalid coordinate for mod terrain blb file " + blbFile + " | " + line);
                        continue;
                    }

                    modTerShape->addPoint(vec,false);
                    //verts.push_back(vec);

                    vertsReadSincePositionLine++;
                    if(vertsReadSincePositionLine >= 4)
                    {
                        vertsReadSincePositionLine = 0;
                        readingVerts = false;
                    }
                }
                else
                {
                    error(blbFile + " line " + line + " was rejected as being malformed?");
                }
            }
        }
    }

    file.close();

    modTerShape->recalcLocalAabb();
    isModTerrain = true;

    btVector3 dim = maxDim - minDim;

    width = maxDim.x() - minDim.x();
    height = maxDim.y() - minDim.y();
    length = maxDim.z() - minDim.z();

}

void brickLoader::sendTypesToClient(serverClientHandle *target)
{
    int totalSent = 0;
    int nextToSend = 0;
    int typesLeftToSend = brickTypes.size();

    while(typesLeftToSend > 0)
    {
        unsigned int totalBytes = 0;
        unsigned int howManyToSend = 0;
        unsigned int howManyToSkip = 0;
        //1400 is how many bytes we assume we have for an unbroken UDP packet?
        while(totalBytes<1400)
        {
            //63 is related to the 6 bits for howManyToSend down below
            if(howManyToSend >= 63)
                break;

            if(nextToSend + howManyToSend + howManyToSkip >= brickTypes.size())
                break;

            if(brickTypes[nextToSend + howManyToSend + howManyToSkip]->special)
            {
                totalBytes += brickTypes[nextToSend + howManyToSend + howManyToSkip]->fileName.length() + 3;
                howManyToSend++;
            }
            else
                howManyToSkip++;
        }

        packet data;
        data.writeUInt(packetType_addSpecialBrickType,packetTypeBits);
        data.writeUInt(howManyToSend,6);

        int leftToSend = howManyToSend;
        int leftToSkip = howManyToSkip;
        int iter = 0;
        int inPacket = 0;
        while(leftToSend > 0 || leftToSkip > 0)
        {
            if(brickTypes[nextToSend + iter]->special)
            {
                totalSent++;
                data.writeUInt(nextToSend + iter,10); //Maximum of 1024 brick types for now
                data.writeString(brickTypes[nextToSend+iter]->fileName);
                data.writeBit(brickTypes[nextToSend+iter]->isModTerrain);
                leftToSend--;
                inPacket++;
            }
            else
                leftToSkip--;
            iter++;
        }

        if(target)
            target->send(&data,true);

        typesLeftToSend -= howManyToSend + howManyToSkip;
        nextToSend += howManyToSend + howManyToSkip;
    }

    debug("Sent a total of " + std::to_string(totalSent) + " special types to client.");
}

unsigned int brick::getPacketBits(bool fullPos)
{
    int bits = 63 + (fullPos ? 107 : 47) + (isSpecial ? 10 : 24);
    if(!fullPos)
    {
        if(printMask != 0 && printID != -1 && printName.length() > 0)
        {
            packet tmp;
            tmp.writeString(printName);
            //std::cout<<"String bits: "<<tmp.getStreamPos()<<"\n";
            bits += tmp.getStreamPos() + 7;
        }
        else
            bits++;
    }
    //std::cout<<"Bits: "<<bits<<"\n";
    return bits;
}

void brick::addToPacket(packet *data,bool fullPos)
{
    data->writeUInt(serverID,20);

    unsigned int nr = r*255;
    data->writeUInt(nr,8);
    unsigned int ng = g*255;
    data->writeUInt(ng,8);
    unsigned int nb = b*255;
    data->writeUInt(nb,8);
    unsigned int na = a*255;
    data->writeUInt(na,8);

    /*if(!fullPos)
    {
        unsigned int nx = floor(x*2)+2048;
        data->writeUInt(nx,13);

        unsigned int ny = floor(y*10)+2048;
        data->writeUInt(ny,16);

        unsigned int nz = floor(z*2)+2048;
        data->writeUInt(nz,13);
    }
    else
    {
        data->writeFloat(x);
        data->writeFloat(y);
        data->writeFloat(z);
    }*/

    if(fullPos)
    {
        data->writeFloat(carX);
        data->writeFloat(carY);
        data->writeUInt(carPlatesUp,10);
        data->writeBit(yHalfPosition);
        data->writeFloat(carZ);
    }
    else
    {
        data->writeUInt(posX+8192,14);
        data->writeBit(xHalfPosition);

        data->writeUInt(uPosY,16);
        data->writeBit(yHalfPosition);

        data->writeUInt(posZ+8192,14);
        data->writeBit(zHalfPosition);
    }

    data->writeUInt(angleID,2);

    int tmpMat = material;
    int shapeFx = 0;
    if(tmpMat >= bob)
    {
        tmpMat -= bob;
        shapeFx += 2;
    }
    else if(tmpMat >= undulo)
    {
        tmpMat -= undulo;
        shapeFx += 1;
    }
    data->writeUInt(shapeFx,4);
    data->writeUInt(tmpMat,4);

    data->writeBit(isSpecial);
    if(isSpecial)
        data->writeUInt(typeID,10);
    else
    {
        data->writeUInt(width,8);
        data->writeUInt(height,8);
        data->writeUInt(length,8);
    }

    if(!fullPos)
    {
        data->writeBit(isColliding());
        if(printMask != 0 && printID != -1 && printName.length() > 0)
        {
            data->writeBit(true);
            data->writeUInt(printMask,6);
            data->writeString(printName);
        }
        else
            data->writeBit(false);
    }
}

void brickLoader::addPhysicsToBrick(brick *toAdd,btDynamicsWorld *world)
{
    btCollisionShape *shape = 0;

    if(toAdd->isSpecial && brickTypes[toAdd->typeID]->isModTerrain)
    {
        shape = brickTypes[toAdd->typeID]->modTerShape;
    }
    else
    {
        shape = collisionShapes->at(toAdd->width,toAdd->height,toAdd->length);
        if(!shape)
        {
            btVector3 dim = btVector3(((float)toAdd->width)/2.0,((float)toAdd->height)/(2.0*2.5),((float)toAdd->length)/2.0);
            btBoxShape *shapeNew = new btBoxShape(dim);
            collisionShapes->set(toAdd->width,toAdd->height,toAdd->length,shapeNew);
            shape = shapeNew;
        }
    }

    //float vx=toAdd->x,vy=toAdd->y,vz=toAdd->z;
    float vx = toAdd->getX();
    float vy = toAdd->getY();
    float vz = toAdd->getZ();

    if(toAdd->angleID == 0 || toAdd->angleID == 2)
    {
        vx -= toAdd->width/2.0;
        vy -= toAdd->height/2.0;
        vz -= toAdd->length/2.0;
    }
    else
    {
        vz -= toAdd->width/2.0;
        vy -= toAdd->height/2.0;
        vx -= toAdd->length/2.0;
    }
    btTransform startTrans = btTransform::getIdentity();

    if(toAdd->angleID == 0 || toAdd->angleID == 2)
        startTrans.setOrigin(btVector3(vx+(toAdd->width/2.0),vy+(toAdd->height/2.0),vz+(toAdd->length/2.0)));
    else
    {
        startTrans.setOrigin(btVector3(vx+(toAdd->length/2.0),vy+(toAdd->height/2.0),vz+(toAdd->width/2.0)));
        startTrans.setRotation(btQuaternion(1.57,0,0));
    }

    if(toAdd->isSpecial && brickTypes[toAdd->typeID]->isModTerrain)
    {
        /*
        glm::quat(0,0,1,0),
        glm::quat(4.7122,0,1,0),
        glm::quat(3.1415,0,1,0),
        glm::quat(1.5708,0,1,0)};
        */
        if(toAdd->angleID == 0)
            startTrans.setRotation(btQuaternion(0,0,0));
        if(toAdd->angleID == 1)
            startTrans.setRotation(btQuaternion(1.5708,0,0));
        if(toAdd->angleID == 2)
            startTrans.setRotation(btQuaternion(3.1415,0,0));
        if(toAdd->angleID == 3)
            startTrans.setRotation(btQuaternion(4.7122,0,0));
    }

    btMotionState* ms = new btDefaultMotionState(startTrans);
    btRigidBody::btRigidBodyConstructionInfo info(0.0,ms,shape,btVector3(0,0,0));
    toAdd->body = new btRigidBody(info);
    toAdd->body->setCollisionFlags( btCollisionObject::CF_STATIC_OBJECT);
    toAdd->body->setFriction(toAdd->material==slippery ? 0.3 : 1.0);
    toAdd->body->setRestitution(toAdd->material==bob ? 2.0 : 0.0);
    toAdd->body->forceActivationState(ISLAND_SLEEPING);
    world->addRigidBody(toAdd->body);
    toAdd->body->setUserPointer(toAdd);
    toAdd->body->setUserIndex(bodyUserIndex_brick);
}

brickLoader::brickLoader(std::string typesFolder,std::string printsFolder)
{
    for (recursive_directory_iterator i(printsFolder.c_str()), end; i != end; ++i)
    {
        if (!is_directory(i->path()))
        {
            if(i->path().parent_path().string().find("prints") != std::string::npos && i->path().parent_path().string().find("icons") == std::string::npos)
            {
                if(i->path().extension() == ".png")
                {
                    std::string addonName = i->path().parent_path().string();
                    addonName = addonName.substr(0,addonName.length() - 7);
                    int pos = addonName.find("\\Print_")+7;
                    addonName = addonName.substr(pos,addonName.length() - pos);
                    if(addonName.find("_Default") != std::string::npos)
                        addonName = addonName.substr(0,addonName.length() - 8);
                    addonName += "/" + i->path().stem().string();

                    printNames.push_back(addonName);
                    printFilePaths.push_back(i->path().string());
                }
            }
        }
    }

    std::cout<<printNames.size()<<" prints found!\n";

    info("Loading brick types...");

    collisionShapes = new Octree<btBoxShape*>(256);

    printAlias *print1x1 = new printAlias;
    print1x1->uiName = "1x1 print";
    print1x1->fileName = "1x1print";
    print1x1->width = 1;
    print1x1->height = 3;
    print1x1->length = 1;
    print1x1->addFace(FACE_SOUTH);
    printTypes.push_back(print1x1);

    printAlias *print1x4x4 = new printAlias;
    print1x4x4->fileName = "1x4x4print";
    print1x4x4->uiName = "1x4x4 print";
    print1x4x4->width = 4;
    print1x4x4->height = 12;
    print1x4x4->length = 1;
    print1x4x4->addFace(FACE_SOUTH);
    printTypes.push_back(print1x4x4);

    printAlias *print2x2f = new printAlias;
    print2x2f->uiName = "2x2f print";
    print2x2f->fileName = "2x2fprint";
    print2x2f->width = 2;
    print2x2f->height = 1;
    print2x2f->length = 2;
    print2x2f->addFace(FACE_UP);
    printTypes.push_back(print2x2f);

    printAlias *print1x2f = new printAlias;
    print1x2f->uiName = "1x2f print";
    print1x2f->fileName = "1x2fprint";
    print1x2f->width = 1;
    print1x2f->height = 1;
    print1x2f->length = 2;
    print1x2f->addFace(FACE_UP);
    printTypes.push_back(print1x2f);

    printAlias *print1x1f = new printAlias;
    print1x1f->uiName = "1x1f print";
    print1x1f->fileName = "1x1fprint";
    print1x1f->width = 1;
    print1x1f->height = 1;
    print1x1f->length = 1;
    print1x1f->addFace(FACE_UP);
    printTypes.push_back(print1x1f);

    std::ifstream f((typesFolder + "/types.txt").c_str());

    if(!f.is_open())
    {
        error("Could not open aliases!");
        return;
    }

    std::string line = "";
    while(!f.eof())
    {
        getline(f,line);
        std::size_t barPos = line.find("|");
        if(barPos == std::string::npos)
            break;

        std::string key = line.substr(0,barPos);
        std::string val = line.substr(barPos+1,line.length() - (barPos+4));

        brickType *tmp = new brickType;
        tmp->fileName = lowercase(val);
        tmp->uiname = lowercase(key);
        brickTypes.push_back(tmp);
    }
    f.close();

    for (recursive_directory_iterator i(typesFolder.c_str()), end; i != end; ++i)
    {
        if (!is_directory(i->path()))
        {
            if(i->path().extension() == ".blb")
            {
                std::ifstream blb(i->path());
                if(!blb.is_open())
                {
                    error("Cannot open " + i->path().string());
                    continue;
                }

                std::string fname = lowercase(i->path().stem().string());

                brickType *toEdit = 0;

                for(unsigned int a = 0; a<brickTypes.size(); a++)
                {
                    if(brickTypes[a]->fileName == fname)
                    {
                        toEdit = brickTypes[a];
                        break;
                    }
                }

                if(!toEdit)
                {
                    for(unsigned int a = 0; a<brickTypes.size(); a++)
                    {
                        if(brickTypes[a]->uiname == fname)
                        {
                            toEdit = brickTypes[a];
                            break;
                        }
                    }
                    if(!toEdit)
                    {
                        error("Could not find alias for " + fname);
                        continue;
                    }
                }

                std::string dims = "";
                getline(blb,dims);
                std::vector<std::string> words;
                split(dims,words);
                getline(blb,dims);
                blb.close();

                toEdit->fileName = i->path().string().substr(typesFolder.length());

                float x = atoi(words[0].c_str());
                float y = atoi(words[2].c_str());
                float z = atoi(words[1].c_str());

                if(i->path().string().find("ModTer") != std::string::npos)
                    toEdit->initModTerrain(i->path().string());
                else
                    toEdit->init(x,y,z,collisionShapes);

                if(dims == "SPECIAL" || dims == "SPECIALBRICK")
                {
                    toEdit->special = true;
                    specialBrickTypes++;

                    /*std::ofstream luaFile(i->path().parent_path().string() + "/server.lua",std::ios::app);

                    std::string addonName = i->path().parent_path().filename().string();
                    if(addonName.substr(0,6) != "Brick_")
                        addonName = "Brick_" + addonName;

                    // addSpecialBrickType("Brick_Arch/1x3arch.blb","1x3arch"

                    bool modTer = toEdit->fileName.find("ModTer") != std::string::npos;

                    luaFile<<"addSpecialBrickType(\""<<addonName<<"/"<<i->path().filename().string()<<"\",\""<<toEdit->uiname<<"\","<<(modTer?"true":"false")<<")\n";

                    luaFile.close();*/
                }
                else if(dims == "BRICK")
                {

                }
                else
                    error("Blb file neither BRICK nor SPECIAL " + dims + " File: " + i->path().string());
            }
        }
    }

    info("Loaded " + std::to_string(brickTypes.size()) + " types of bricks! Of which " + std::to_string(specialBrickTypes) + " were special!");
}
