#include "ePhysics.h"

//#define INDEX_COUNT 16000

Physics::Physics(EngineTime& Clock, std::vector<RStorage::eResource>& trackedModels, const int& UpdateRate) :
    GConst(6.67430 * pow(10, -11)),
    timer(Clock),
    trackedModels(trackedModels),
    urate(UpdateRate)
{
    coreCount = std::thread::hardware_concurrency();
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    _ASSERT(platforms.size() > 0);
    auto& platform = platforms.front();
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

    _ASSERT(devices.size() > 0);

    auto& device = devices.front();
    auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
    auto version = device.getInfo<CL_DEVICE_VERSION>();


    context = cl::Context{ device };
    std::ifstream phys("OpenCL\\ePhysics.cl");
    std::ifstream sphr("OpenCL\\SphrKern.cl");


    //Replace this with something that works in release
    _ASSERT(phys ? true : false);
    _ASSERT(sphr ? true : false);

    std::string p(std::istreambuf_iterator<char>{phys}, {});
    std::string s(std::istreambuf_iterator<char>{sphr}, {});

    int error = CL_SUCCESS;
    Sphere = cl::Program(context, s);
    FullColl = cl::Program(context, p);


    //Add error checking here
    Sphere.build({ device });
    FullColl.build({ device });


    queue = cl::CommandQueue{ context, device };
    thrds = std::make_unique<THREADS>(coreCount);
    childthrds = std::make_unique<THREADS>(coreCount);

    clGetDeviceInfo(device.get(), CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &clLocalMemSize, 0);

}


void Physics::Update() {

    pSpecReset();
    ticker.incCount();
    //std::for_each(trackedModels.begin(), trackedModels.end(), [this](auto& m) {cGravity(&m); });
    pSpecCollison();
    mMove(trackedModels);
}




void Physics::trackM() {
    QueueMTX.lock();
    for (int i = 0; i < std::size(trackedModels); i++) {
        trackedModels[i].model->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(UpVertNorm) * std::size(trackedModels[i].model->uData->MappedVertices)*3);
        trackedModels[i].clPositionBuff = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(DirectX::XMFLOAT3));

        std::vector<DirectX::XMFLOAT3> WBone;
        std::vector<int> IndexOffset;
        WBone.resize(std::size(trackedModels[i].model->uData->bdata));
        for (auto b = 0; b < std::size(WBone); b++) {
            WBone[b] = trackedModels[i].model->uData->bdata[b].sphere.Center;
        }
        trackedModels[i].model->clBoneBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(DirectX::XMFLOAT3) * std::size(WBone));
        trackedModels[i].model->clIndexBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(INTINDEX) * (std::size(trackedModels[i].model->uData->idata)/3));
        trackedModels[i].model->clIndexMap = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * std::size(trackedModels[i].model->uData->idata));


        std::vector<INTINDEX> Verts(std::size(trackedModels[i].model->uData->idata)/3);

        for (auto r = 0; r < std::size(trackedModels[i].model->uData->idata); r++) {
            Verts[trackedModels[i].model->uData->idata[r].index].Index[r%3] = r;
        }
        std::vector<int> Mapper(trackedModels[i].model->uData->idata.size());
        for (auto m = 0; m < Mapper.size(); m++) {
            Mapper[m] = m;
        }

        queue.enqueueWriteBuffer(trackedModels[i].model->clIndexBuff, CL_FALSE, 0, sizeof(INTINDEX) * Verts.size(), Verts.data());
        queue.enqueueWriteBuffer(trackedModels[i].model->clIndexMap, CL_FALSE, 0, sizeof(int) * Mapper.size(), Mapper.data());

        
        queue.enqueueWriteBuffer(trackedModels[i].model->clBoneBuff, CL_FALSE, 0, sizeof(DirectX::XMFLOAT3) * std::size(WBone), WBone.data());


        for (auto v = 0; v < std::size(trackedModels[i].model->uData->idata); v++) {
            queue.enqueueWriteBuffer(trackedModels[i].model->clBuff, CL_FALSE, v*sizeof(UpVertNorm), sizeof(DirectX::XMFLOAT3), &trackedModels[i].model->uData->MappedVertices[trackedModels[i].model->uData->idata[v].index][v%3].position);
            queue.enqueueWriteBuffer(trackedModels[i].model->clBuff, CL_FALSE, (v * sizeof(UpVertNorm))+ sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3), &trackedModels[i].model->uData->MappedVertices[trackedModels[i].model->uData->idata[v].index][v % 3].normal);
        }
    }
    queue.finish();
    QueueMTX.unlock();


}

void Physics::cGravity(RStorage::eResource* obj) {
    using namespace DirectX;
    if (obj->mworld != nullptr) {
        //obj->grav = fDirection(obj->mPos->position, obj->mworld->mPos->position);
        //obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(fDistance(obj->mPos->position, obj->mworld->mPos->position), 2))) * timer.Current();

        float mag = obj->speed + obj->gravpull;
        DirectX::XMFLOAT3 dotpro = { 0,0,0 };
        auto Tempvel = obj->velDir;
        XMStoreFloat4(&Tempvel, XMLoadFloat4(&Tempvel) * obj->speed + XMLoadFloat4(&obj->grav) * obj->gravpull);
        XMStoreFloat3(&dotpro, XMVector3Dot(XMLoadFloat4(&Tempvel), XMLoadFloat4(&Tempvel)));
        obj->speed = sqrt(dotpro.x);
        XMStoreFloat4(&obj->velDir, XMLoadFloat4(&Tempvel) / obj->speed);
    } 
}



int Physics::bIndex(std::vector<int> w, int bInd) {
    int Index = -1;
    for (auto i = 0; i < std::size(w); i++) {
        if (w[i] == bInd) {
            Index = i;
        }
    }
    return Index;
}

//This dont work ree
void Physics::CalProportionalSpeed(DirectX::XMFLOAT4& VelDir1, DirectX::XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2) {
    using namespace DirectX;
    float massScal1 = (Mass1 / (Mass1 + Mass2));
    float massScal2 = (Mass2 / (Mass1 + Mass2));
    massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
    massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

    XMFLOAT4 TempVelD1 = VelDir1;
    float TempVSpeed1 = VSpeed1;

    XMFLOAT4 RelDir;
    XMStoreFloat4(&RelDir, XMLoadFloat4(&VelDir2) * VSpeed2 * -massScal1 + XMLoadFloat4(&VelDir1)*VSpeed1* -massScal2);
    XMFLOAT4 RelDot;
    XMStoreFloat4(&RelDot, XMVector3Dot(XMLoadFloat4(&RelDir), XMLoadFloat4(&RelDir)));
    float mag = sqrt(RelDot.x);


    XMStoreFloat4(&VelDir1, XMVector3Normalize(XMLoadFloat4(&RelDir) * -massScal2));
    VSpeed1 = mag * massScal2;

    XMStoreFloat4(&VelDir2, XMVector3Normalize(XMLoadFloat4(&RelDir) * -massScal1));
    VSpeed2 = mag * massScal1;
}


//Moved most memory access to stack but execution time for high index count still attrocious 
Physics::WORKINDI Physics::ProcCollide(RStorage::eResource& obj, RStorage::eResource& obj2,  DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist) {
    WORKINDI Result;
    using namespace DirectX;
    XMFLOAT3 pos2;
    XMStoreFloat3(&pos2, XMLoadFloat4(&dir) * dist);

    auto &objudat = obj.model->uData;
    auto &obj2udat = obj2.model->uData;
    auto &objbdata = objudat->bdata;
    auto &obj2bdata = obj2udat->bdata;
    auto MappedVert1 = objudat->MappedVertices;
    auto MappedVert2 = obj2udat->MappedVertices;

    std::vector<std::vector<int>> WData;

    //This can be split up for better performance with large amounts of bones
    for (auto& b2 : obj2bdata) {
        for (auto& b : objbdata) {
            auto sph2 = b2.sphere;
            XMStoreFloat3(&sph2.Center, XMLoadFloat3(&b2.sphere.Center) + XMLoadFloat3(&pos2));
            if (b.sphere.Intersects(sph2)) {
                if (std::size(b.Indices) > 0 && std::size(b2.Indices) > 0) {
                    WData.emplace_back(std::vector<int>{b.bIndex, b2.bIndex});
                }
            }

        }
    }

    if (WData.size() == 0) return Result;


    std::mutex iLock;

    //std::vector<SPHR> CollSp(WData.size());



    
    int indexCount1 = 0;
    int indexCount2 = 0;
    std::vector<std::vector<int>> b1ind(WData.size(), {});
    std::vector<std::vector<int>> b2ind(WData.size(), {});
    std::vector<UINT> offset1(WData.size());
    std::vector<UINT> offset2(WData.size());
    //std::vector<std::vector<int>> ISize(WData.size(), std::vector<int>(2));
    std::vector<std::atomic<short int>> objCheck1(objudat->idata.size());
    std::vector<std::atomic<short int>> objCheck2(obj2udat->idata.size());


    std::vector<THREADS::WRef> refs;
    refs.resize(WData.size());

    for (auto i = 0; i < WData.size(); i++) {
        auto& wDat = WData[i];
        std::function<void()> tFunc{ [MappedVert1, MappedVert2, wDat, i, &objudat, &obj2udat, &pos2, &b1ind, &b2ind, &objCheck1, &objCheck2]() {
            auto objidat = objudat->idata;
            auto obj2idat = obj2udat->idata;
            
            auto Bone1 = objudat->bdata[wDat[0]].Indices;
            auto Bone2 = obj2udat->bdata[wDat[1]].Indices;

            auto tempbSphere = objudat->bdata[wDat[0]].sphere;
            auto tempbSphere2 = obj2udat->bdata[wDat[1]].sphere;

            SPHR CollSp;
            using namespace DirectX;
            XMStoreFloat3(&CollSp.Center[0], XMLoadFloat3(&tempbSphere.Center) - XMLoadFloat3(&pos2));
            XMStoreFloat3(&CollSp.Center[1], XMLoadFloat3(&tempbSphere2.Center) + XMLoadFloat3(&pos2));
            CollSp.Radius[0] = tempbSphere.Radius;
            CollSp.Radius[1] = tempbSphere2.Radius;

            std::vector<int> bt1;
            bt1.reserve(Bone1.size());
            XMFLOAT4 bdirection = fDirection(tempbSphere.Center, CollSp.Center[1]);

            for (auto b = 0; b < Bone1.size(); b++) {
                if (objCheck1[Bone1[b]].load() != 1) {
                    auto ree = XMVector3Dot(XMLoadFloat4(&bdirection), XMLoadFloat3(&MappedVert1[objidat[Bone1[b]].index][0].normal));
                    if (XMVector3GreaterOrEqual(ree, XMVectorZero())) {
                        bt1.push_back(Bone1[b]);
                    }
                    objCheck1[Bone1[b]].store(1);
                }

            }




            std::vector<int> bt2;
            bt2.reserve(Bone2.size());
            XMFLOAT4 ibdirection = fDirection(tempbSphere2.Center, CollSp.Center[0]);
            for (auto b = 0; b < Bone2.size(); b++) {
                if (objCheck2[Bone2[b]].load() != 1) {
                    auto ib = XMLoadFloat4(&ibdirection);
                    auto bind = Bone2[b];
                    auto index = obj2idat[bind].index;
                    auto norm = XMLoadFloat3(&MappedVert2[index][0].normal);

                    auto ree = XMVector3Dot(ib, norm);
                    if (XMVector3GreaterOrEqual(ree, XMVectorZero())) {
                        bt2.push_back(Bone2[b]);
                    }
                    objCheck2[Bone2[b]].store(1);
                }


            }



            b2ind[i] = bt2;
            b1ind[i] = bt1;


        } };
       refs[i] = (childthrds.get()->gPushWork(tFunc));
    }
    for (auto& i : refs) {
        childthrds.get()->gEndWork(i);
    }

    std::vector<int> objind(objudat->MappedVertices.size(), 0);
    std::vector<int> obj2ind(obj2udat->MappedVertices.size(), 0);
    std::vector<int > WorkIndi1;
    std::vector<int> WorkIndi2;
    WorkIndi1.reserve(objudat->idata.size());
    
    WorkIndi2.reserve(obj2udat->idata.size());
    
    
        for (auto& b : b1ind) {
        for (auto& w : b) {
            if (objind[objudat->idata[w].index] == 0) {
                WorkIndi1.push_back(objudat->idata[w].index);
                objind[objudat->idata[w].index] = 1;
            }
        }
    }
    for (auto& b : b2ind) {
        for (auto& w : b) {
            if (obj2ind[obj2udat->idata[w].index] == 0) {
                WorkIndi2.push_back(obj2udat->idata[w].index);
                obj2ind[obj2udat->idata[w].index] = 1;
            }
        }
    }
    


    Result.Position = pos2;
    Result.Indices.append_range(WorkIndi1);
    Result.wWorkCount = WorkIndi1.size();
    Result.Indices.append_range(WorkIndi2);
    Result.tWorkCount = WorkIndi2.size();
    return Result;
}


void Physics::pSpecCollison() {

    std::vector<THREADS::WRef> refs(trackedModels.size());
    for (auto h = 0; h < trackedModels.size(); h++) {
      std::function<void()> work{[this, h]() {
            using namespace DirectX;
            auto& obj = trackedModels[h];
            XMFLOAT3 Pos1;
            obj.mPos.posMtx.lock();
            XMStoreFloat3(&Pos1, XMLoadFloat3(obj.mPos.position));
            auto sph1 = obj.model->uData->Sphere;
            obj.mPos.posMtx.unlock();


            std::vector<collstruct> wCollModels;
            {
                std::vector<collstruct> LCollModels;
                LCollModels.reserve(trackedModels.size());


                for (auto m = 0; m < std::size(trackedModels); m++) {
                    if (trackedModels[m] != obj) {
                        XMFLOAT3 Pos2;
                        trackedModels[m].mPos.posMtx.lock();
                        XMStoreFloat3(&Pos2, XMLoadFloat3(trackedModels[m].mPos.position));
                        auto sph2 = trackedModels[m].model->uData->Sphere;
                        trackedModels[m].mPos.posMtx.unlock();
                        auto tdist = fDirection(Pos1, Pos2);
                        XMStoreFloat3(&sph2.Center, XMLoadFloat4(&tdist) * fDistance(Pos1, Pos2));
                        if (sph2.Intersects(sph1)) {
                            LCollModels.emplace_back(collstruct(&obj, &trackedModels[m]));
                        }
                    }
                }
                std::vector<bool> lCheck(LCollModels.size(), true);
                cmMtx.lock();
                if (CollModels.size() != 0) {
                    std::for_each(CollModels.begin(), CollModels.end(), [&lCheck, &LCollModels](auto& c) {
                        for (auto l = 0; l < LCollModels.size(); l++) {
                            if (lCheck[l]) {
                                if (LCollModels[l] == c) lCheck[l] = false;
                            }
                        }
                    });
                    CollModels.reserve(lCheck.size());
                    for (auto i = 0; i < lCheck.size(); i++) {
                        if (lCheck[i]) {
                            CollModels.emplace_back(LCollModels[i]);
                            wCollModels.emplace_back(LCollModels[i]);
                        }
                    }
                    CollModels.shrink_to_fit();

                }
                else {
                    CollModels.append_range(LCollModels);
                    wCollModels = LCollModels;
                }
                cmMtx.unlock();

            }



            //We need more queues. Found limit maybe?
            auto tQueue = cl::CommandQueue{ context, devices.front() };

            std::vector<std::thread> tThreads(wCollModels.size());
            std::vector<cl::Buffer> rbuffers(wCollModels.size());
            std::vector < std::vector<int>> wSizes(wCollModels.size(), std::vector<int>(2));
            for (auto i = 0; i < wCollModels.size(); i++) {
                auto& obj2 = *wCollModels[i].obj2;
                XMFLOAT3 Pos2;
                obj2.mPos.posMtx.lock();
                XMStoreFloat3(&Pos2, XMLoadFloat3(obj2.mPos.position));
                auto sph2 = obj2.model->uData->Sphere;
                obj2.mPos.posMtx.unlock();
                auto dir = fDirection(Pos1, Pos2);
                auto dist = fDistance(Pos1, Pos2);
                WORKINDI WorkIndi = ProcCollide(obj, obj2, Pos1, Pos2, dir, dist);
                if (WorkIndi.wWorkCount != 0 && WorkIndi.tWorkCount != 0) {
                    wSizes[i][0] = WorkIndi.wWorkCount;
                    wSizes[i][1] = WorkIndi.tWorkCount;
                    cl::Buffer PositionBuffer(context, CL_MEM_READ_ONLY, sizeof(XMFLOAT3));
                    cl::Buffer IndexBuffer(context, CL_MEM_READ_ONLY, WorkIndi.Indices.size() * sizeof(int));
                    auto& Indices = WorkIndi.Indices;
                    cl::Kernel collide(FullColl, "coll");
                    std::vector<RETURNDATA> retdat(wSizes[i][0]);

                    rbuffers[i] = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * retdat.size());
                    tQueue.enqueueWriteBuffer(PositionBuffer, CL_FALSE, 0, sizeof(XMFLOAT3), &WorkIndi.Position);
                    tQueue.enqueueWriteBuffer(IndexBuffer, CL_FALSE, 0, std::size(Indices) * sizeof(int), Indices.data());
                    tQueue.enqueueWriteBuffer(rbuffers[i], CL_FALSE, 0, sizeof(RETURNDATA) * retdat.size(), retdat.data());


                    collide.setArg(0, obj.model->clBuff);
                    collide.setArg(1, obj2.model->clBuff);
                    collide.setArg(2, obj.model->clIndexBuff);
                    collide.setArg(3, obj2.model->clIndexBuff);
                    //collide.setArg(4, obj.model->clIndexMap);
                    //collide.setArg(5, obj2.model->clIndexMap);
                    collide.setArg(4, PositionBuffer);
                    collide.setArg(5, IndexBuffer);
                    collide.setArg(6, rbuffers[i]);

                    size_t wsize[2] = { WorkIndi.wWorkCount, WorkIndi.tWorkCount };
                    size_t offsize[2] = { 0, WorkIndi.wWorkCount };
                    int errore = 0;
                    tQueue.finish();



                    errore = clEnqueueNDRangeKernel(tQueue.get(), collide.get(), 2, offsize, wsize, nullptr, 0, NULL, NULL);
                    _ASSERT(errore == CL_SUCCESS);


                }
            }
            tQueue.finish();

            for (auto i = 0; i < wCollModels.size(); i++) {
                if (wSizes[i][0] != 0 && wSizes[i][1] != 0) {
                    auto& obj2 = *wCollModels[i].obj2;

                    XMFLOAT3 Pos2;
                    obj2.mPos.posMtx.lock();
                    XMStoreFloat3(&Pos2, XMLoadFloat3(obj2.mPos.position));
                    auto sph2 = obj2.model->uData->Sphere;
                    obj2.mPos.posMtx.unlock();

                    std::vector<RETURNDATA> retdat(wSizes[i][0]);
                    int errore = 0;
                    {
                        auto& retbuffer = rbuffers[i];

                        errore = tQueue.enqueueReadBuffer(retbuffer, CL_TRUE, 0, sizeof(RETURNDATA) * retdat.size(), retdat.data());
                        _ASSERT(errore == CL_SUCCESS);
                    }

                    XMFLOAT3 MoveD1{ 0,0,0};
                    XMFLOAT3 MoveD2{ 0,0,0};
                    XMFLOAT3 Zero{ 0,0,0};

                    int coutn = 0;
                    for (auto r = 0; r < std::size(retdat); r++) {
                        if (retdat[r].coll) {

                            XMStoreFloat3(&MoveD1, (XMLoadFloat3(&MoveD1) + (XMLoadFloat3(&retdat[r].dir[0]) * retdat[r].dist[0])));
                            XMStoreFloat3(&MoveD2, (XMLoadFloat3(&MoveD2) + (XMLoadFloat3(&retdat[r].dir[1]) * retdat[r].dist[1])));

                            coutn++;
                        }
                    }
                    if (coutn != 0) {
                        XMStoreFloat3(&MoveD1, XMLoadFloat3(&MoveD1)/coutn);
                        XMStoreFloat3(&MoveD2, XMLoadFloat3(&MoveD2) / coutn);

                        float massScal1 = (obj.mass / (obj.mass + obj2.mass));
                        float massScal2 = (obj2.mass / (obj.mass + obj2.mass));
                        massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
                        massScal2 = massScal2 < 0.00001 ? 0 : massScal2;


                        XMFLOAT3 obj2pDir;
                        XMStoreFloat3(&obj2pDir, XMLoadFloat3(&Pos2) + (XMLoadFloat3(&MoveD1)) * massScal1);
                        XMFLOAT3 objpDir;
                        XMStoreFloat3(&objpDir, XMLoadFloat3(&Pos1) + (XMLoadFloat3(&MoveD2)) * massScal2);

                        //Fixxx thissss
                        //CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed, obj2.speed, obj->mass, obj2.mass);
                        obj.CollisionUp(fDirection(Pos1, objpDir), fDistance(Pos1, objpDir));
                        obj2.CollisionUp(fDirection(Pos2, obj2pDir), fDistance(Pos2, obj2pDir));

                    }
                }
            }
        }};
       refs[h] = thrds.get()->gPushWork(work);
    }
    for (auto& t : refs) {
        //_ASSERT(t.uWid != 0);
        thrds.get()->gEndWork(t);
    }
}

void Physics::pSpecReset() {
    //Do stuff here to free memory when Collision calculations are over.
    cmMtx.lock();
    CollModels.resize(0);
    CollModels.shrink_to_fit();
    cmMtx.unlock();
}




float Physics::fDistance(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2) {
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, DirectX::XMVectorSubtract(XMLoadFloat3(&pos2), XMLoadFloat3(&pos1)));
    DirectX::XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    return d;
}

DirectX::XMFLOAT4 Physics::fDirection(DirectX::XMFLOAT3& pos1, DirectX::XMFLOAT3& pos2) {
    using namespace DirectX;
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, XMLoadFloat3(&pos2) - XMLoadFloat3(&pos1));
    XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    if (d > 0) {
        XMStoreFloat4(&Result, XMLoadFloat4(&Result) / d);
        return Result;
    }
    return { 0,0,0,0 };
}


void Physics::mMove(std::vector<RStorage::eResource>& trackedModels) {
    std::for_each(trackedModels.begin(), trackedModels.end(), [](auto& mUpdate) {
        using namespace DirectX;
        mUpdate.mPos.posMtx.lock();
        mUpdate.mPos.lastposition = *mUpdate.mPos.position;
        if (mUpdate.CollCheck()) {
            auto pDir = mUpdate.CollDir();
            XMStoreFloat3(mUpdate.mPos.position, XMLoadFloat3(mUpdate.mPos.position) + XMLoadFloat4(&pDir));
            mUpdate.CollReset();
        }
        XMStoreFloat3(mUpdate.mPos.position, XMLoadFloat3(mUpdate.mPos.position) + XMLoadFloat4(&mUpdate.velDir) * (mUpdate.speed));

        //This needs to be put in graphics. Physics should not be handling graphics updates
        mUpdate.cmatrix = XMMatrixTranslation(0, 0, 0) * XMMatrixRotationQuaternion(XMLoadFloat4(&mUpdate.mPos.rotation));
        mUpdate.cmatrix *= XMMatrixTranslation(mUpdate.mPos.position->x, mUpdate.mPos.position->y, mUpdate.mPos.position->z);
        mUpdate.mPos.posMtx.unlock();
    });

}

Physics::~Physics() {
    thrds.get()->gEndWork(lastWref);
}
