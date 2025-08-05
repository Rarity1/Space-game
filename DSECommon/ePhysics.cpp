#include "ePhysics.h"


//#pragma OPENCL EXTENSION cl_khr_d3d11_sharing : enable


Physics::Physics(EngineTime& Clock, const int& UpdateRate) :
    GConst(6.67430 * pow(10, -11)),
    timer(Clock),
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

    Sphere = cl::Program(context, s);
    FullColl = cl::Program(context, p);


    //Add error checking here
    //Sphere.build({ device });
    auto error = FullColl.build({ device });
    _ASSERT(error == CL_SUCCESS);


    queue = cl::CommandQueue{ context, device };
    
    //Write something that can automatically assign child processess to new THREADS instance
    tMain = std::make_unique<THREADS>(coreCount);
    tProcCollide = std::make_unique<THREADS>(coreCount);
    tProcCollideSub = std::make_unique<THREADS>(coreCount);

    clGetDeviceInfo(device.get(), CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &clLocalMemSize, 0);

}


void Physics::Update(std::list<Object>& trackedObjects) {
    //DebugStream dbgStream;
    //std::streambuf* oldBuf = std::cout.rdbuf(&dbgStream);
    
    CollModels.reserve((trackedObjects.size() ^ 2) / 2);
    //std::for_each(trackedObjects.begin(), trackedObjects.end(), [this](auto& m) {cGravity(&m); });
    pCollison(trackedObjects);
    pSpecReset();

    DebugMTX.lock();
    std::cout << "Current Tick: " + std::to_string(ticker.cGet()) << std::endl;
    DebugMTX.unlock();

    ticker.incCount();
    //std::cout.rdbuf(oldBuf);
}




void Physics::trackM(std::list<Object>& trackedObjects) {
    QueueMTX.lock();
    auto tO = trackedObjects.begin();
    for (int i = 0; i < std::size(trackedObjects); i++) {
        tO->model->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(std::array<ReadXML::Vertex, 3>) * tO->model->uData->MappedVertices.size());
        tO->clPositionBuff = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(DirectX::XMFLOAT3));

        std::vector<DirectX::XMFLOAT3> WBone;
        std::vector<int> IndexOffset;
        WBone.resize(std::size(tO->model->uData->bdata));
        for (auto b = 0; b < std::size(WBone); b++) {
            WBone[b] = tO->model->uData->bdata[b].sphere.Center;
        }
        tO->model->clBoneBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(DirectX::XMFLOAT3) * std::size(WBone));

        queue.enqueueWriteBuffer(tO->model->clBoneBuff, CL_FALSE, 0, sizeof(DirectX::XMFLOAT3) * std::size(WBone), WBone.data());

        queue.enqueueWriteBuffer(tO->model->clBuff, CL_FALSE, 0, sizeof(std::array<ReadXML::Vertex, 3>) * tO->model->uData->MappedVertices.size(), tO->model->uData->MappedVertices.data());

        tO++;
    }
    queue.finish();
    QueueMTX.unlock();


}

void Physics::cGravity(Object* obj) {
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


Physics::WORKINDI Physics::ProcCollide(Object& obj, Object& obj2,  DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist) {
    WORKINDI Result;
    using namespace DirectX;
    XMFLOAT3 pos2;
    XMStoreFloat3(&pos2, XMLoadFloat4(&dir) * dist);

    auto &objudat = obj.model->uData;
    auto &obj2udat = obj2.model->uData;
    auto &objbdata = objudat->bdata;
    auto &obj2bdata = obj2udat->bdata;
    auto& MappedVert1 = objudat->MappedVertices;
    auto& MappedVert2 = obj2udat->MappedVertices;

    std::vector<std::array<uint16_t, 2>> WData;
    WData.reserve(obj2bdata.size() * objbdata.size());
    
    for (auto& b2 : obj2bdata) {
        BoundingSphere sph2 = b2.sphere;
        XMStoreFloat3(&sph2.Center, XMLoadFloat3(&sph2.Center) + XMLoadFloat3(&pos2));
        for (auto& b : objbdata) {
            if (b.sphere.Intersects(sph2)) {
                WData.emplace_back(std::array<uint16_t, 2>{b.bIndex, b2.bIndex});
            }

        }
    }
 
   

    if (WData.size() == 0) return Result;


    
    int indexCount1 = 0;
    int indexCount2 = 0;
    std::vector<std::vector<int>> b1ind(WData.size(), {});
    std::vector<std::vector<int>> b2ind(WData.size(), {});
    std::vector<UINT> offset1(WData.size());
    std::vector<UINT> offset2(WData.size());
    std::vector<std::atomic<bool>> objCheck1(objudat->MappedVertices.size());
    std::vector<std::atomic<bool>> objCheck2(obj2udat->MappedVertices.size());


    
    std::vector<THREADS::WRef> refs1(WData.size());
    std::vector<THREADS::WRef> refs2(WData.size());


    for (auto i = 0; i < WData.size(); i++) {
        std::array<DirectX::BoundingSphere, 2> CollSp;

        
        using namespace DirectX;
        CollSp[0] = objudat->bdata[WData[i][0]].sphere;
        CollSp[1] = obj2udat->bdata[WData[i][1]].sphere;
        XMStoreFloat3(&CollSp[0].Center, XMLoadFloat3(&CollSp[0].Center) - XMLoadFloat3(&pos2));

        XMStoreFloat3(&CollSp[1].Center, XMLoadFloat3(&CollSp[1].Center) + XMLoadFloat3(&pos2));

        b1ind[i].reserve(objudat->bdata[WData[i][0]].Indices.size());
        b2ind[i].reserve(obj2udat->bdata[WData[i][1]].Indices.size());


        refs1[i] = tProcCollideSub->gPushWork([&objudat, &objCheck1, &WData, &b1ind, i, &CollSp, &MappedVert1]() {
            XMFLOAT4 bdirection = fDirection(objudat->bdata[WData[i][0]].sphere.Center, CollSp[1].Center);
            std::for_each(objudat->bdata[WData[i][0]].Indices.begin(), objudat->bdata[WData[i][0]].Indices.end(), [&objCheck1, &b1ind, i, &objudat, &bdirection, &MappedVert1](auto& e) {
                if (!objCheck1[objudat->mIndex[e]].load()) {
                    if (XMVector3GreaterOrEqual(XMVector3Dot(XMLoadFloat4(&bdirection), XMLoadFloat3(&MappedVert1[objudat->mIndex[e]][0].normal)), XMVectorZero())) {
                        objCheck1[objudat->mIndex[e]].store(true);
                        b1ind[i].emplace_back(objudat->mIndex[e]);
                    }

                }
            });
        });

        



        refs2[i] = tProcCollideSub->gPushWork([&obj2udat, &objCheck2, &WData, &b2ind, i, &CollSp, &MappedVert2]() {
            XMFLOAT4 ibdirection = fDirection(obj2udat->bdata[WData[i][1]].sphere.Center, CollSp[0].Center);

            std::for_each(obj2udat->bdata[WData[i][1]].Indices.begin(), obj2udat->bdata[WData[i][1]].Indices.end(), [&objCheck2, &b2ind, i, &obj2udat, ibdirection, &MappedVert2](auto& e) {
                if (!objCheck2[obj2udat->mIndex[e]].load()) {
                    if (XMVector3GreaterOrEqual(XMVector3Dot(XMLoadFloat4(&ibdirection), XMLoadFloat3(&MappedVert2[obj2udat->mIndex[e]][0].normal)), XMVectorZero())) {
                        objCheck2[obj2udat->mIndex[e]].store(true);
                        b2ind[i].emplace_back(obj2udat->mIndex[e]);
                    }
                }
            });
            
        });

    }

    std::vector<int> objind(objudat->MappedVertices.size(), 0);
    std::vector<int> obj2ind(obj2udat->MappedVertices.size(), 0);
    std::vector<int > WorkIndi1;
    std::vector<int> WorkIndi2;
    WorkIndi1.reserve(objudat->MappedVertices.size());
    
    WorkIndi2.reserve(obj2udat->MappedVertices.size());
    

    for (auto i = 0; i < b1ind.size(); i++) {
        tProcCollideSub->gEndWork(refs1[i]);
        WorkIndi1.append_range(b1ind[i]);
    }

    for (auto i = 0; i < b2ind.size(); i++) {
        tProcCollideSub->gEndWork(refs2[i]);
        WorkIndi2.append_range(b2ind[i]);
    }
    


    Result.Position = { pos2.x, pos2.y, pos2.z };
    Result.Indices.append_range(WorkIndi1);
    Result.wWorkCount = WorkIndi1.size();
    Result.Indices.append_range(WorkIndi2);
    Result.tWorkCount = WorkIndi2.size();
    return Result;
}

void Physics::pCollison(std::list<Object>& trackedObjects) {

    std::vector<THREADS::WRef> refs(trackedObjects.size());
    auto tO = trackedObjects.begin();
    for (auto h = 0; h < refs.size(); h++) {
        refs[h] = tMain->gPushWork([this, h, tO, &trackedObjects](){
            using namespace DirectX;
            auto& obj = *tO;
            XMFLOAT3 Pos1;
            obj.mPos.posMtx.lock();
            XMStoreFloat3(&Pos1, XMLoadFloat3(obj.mPos.position));
            auto sph1 = obj.model->uData->Sphere;
            obj.mPos.posMtx.unlock();


            std::vector<collstruct> wCollModels;
            std::vector<collstruct> LCollModels;
            LCollModels.reserve(trackedObjects.size());

            for (auto& m : trackedObjects) {
                if (m != obj) {
                    XMFLOAT3 Pos2;
                    m.mPos.posMtx.lock();
                    XMStoreFloat3(&Pos2, XMLoadFloat3(m.mPos.position));
                    auto sph2 = m.model->uData->Sphere;
                    m.mPos.posMtx.unlock();
                    auto tdist = fDirection(Pos1, Pos2);
                    XMStoreFloat3(&sph2.Center, XMLoadFloat4(&tdist) * fDistance(Pos1, Pos2));
                    if (sph2.Intersects(sph1)) {
                        LCollModels.emplace_back(collstruct(&obj, &m));
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
                for (auto i = 0; i < lCheck.size(); i++) {
                    if (lCheck[i]) {
                        CollModels.emplace_back(LCollModels[i]);
                        wCollModels.emplace_back(LCollModels[i]);
                    }
                }
            }
            else {
                CollModels.append_range(LCollModels);
                wCollModels = LCollModels;
            }
            cmMtx.unlock();




            std::vector<std::thread> tThreads(wCollModels.size());
            std::vector < std::array<int,2>> wSizes(wCollModels.size(), std::array<int, 2>{0,0});

            std::vector<THREADS::WRef> refs;
            refs.resize(wCollModels.size());
            std::vector<std::vector<RETURNDATA>> retdata(wCollModels.size());

  

            for (auto i = 0; i < wCollModels.size(); i++) {

                refs[i] = tProcCollide->gPushWork([this, &wCollModels, i, &wSizes, &obj, &Pos1, &retdata]() {
                    XMFLOAT3 MoveD1{ 0,0,0 };
                    XMFLOAT3 MoveD2{ 0,0,0 };
                    XMFLOAT3 Zero{ 0,0,0 };
                    auto tQueue = cl::CommandQueue{ context, devices.front() };
                    _ASSERT(wCollModels.size() > 0);

                    auto& obj2 = *wCollModels[i].obj2;
                    XMFLOAT3 Pos2;
                    BoundingSphere sph2;
                    obj2.mPos.posMtx.lock();
                    sph2 = obj2.model->uData->Sphere;
                    Pos2 = *obj2.mPos.position;
                    obj2.mPos.posMtx.unlock();
                    auto dir = fDirection(Pos1, Pos2);
                    auto dist = fDistance(Pos1, Pos2);

                    WORKINDI WorkIndi = ProcCollide(obj, obj2, Pos1, Pos2, dir, dist);

                    if (WorkIndi.wWorkCount != 0 && WorkIndi.tWorkCount != 0) {
                        wSizes[i][0] = WorkIndi.wWorkCount;
                        wSizes[i][1] = WorkIndi.tWorkCount;

                        retdata[i].resize(wSizes[i][0]);



                        cl::Buffer PositionBuffer(context, CL_MEM_READ_ONLY, sizeof(XMFLOAT3));
                        cl::Buffer IndexBuffer(context, CL_MEM_READ_ONLY, WorkIndi.Indices.size() * sizeof(int));

                        cl::Kernel kerns(FullColl, "coll");

                        tQueue.enqueueWriteBuffer(PositionBuffer, CL_FALSE, 0, sizeof(XMFLOAT3), &WorkIndi.Position);
                        tQueue.enqueueWriteBuffer(IndexBuffer, CL_FALSE, 0, WorkIndi.Indices.size() * sizeof(cl_int), WorkIndi.Indices.data());
                        size_t retdatSize = sizeof(RETURNDATA) * retdata[i].size();
                        cl::Buffer rbuffer(context, CL_MEM_READ_WRITE, retdatSize);
                        cl_float fillbuff = 0.0;
                        tQueue.enqueueFillBuffer(rbuffer, fillbuff, 0, retdatSize);
                        tQueue.finish();

                        kerns.setArg(0, obj.model->clBuff);
                        kerns.setArg(1, obj2.model->clBuff);
                        kerns.setArg(2, PositionBuffer);
                        kerns.setArg(3, IndexBuffer);
                        kerns.setArg(4, rbuffer);




                        size_t wsize[2] = { WorkIndi.wWorkCount, WorkIndi.tWorkCount };
                        size_t offsize[2] = { 0, WorkIndi.wWorkCount };
                        auto error = CL_SUCCESS;

                        error = clEnqueueNDRangeKernel(tQueue.get(), kerns.get(), 2, offsize, wsize, nullptr, 0, NULL, NULL);
                        _ASSERT(error == CL_SUCCESS);

                        tQueue.finish();
                        error = tQueue.enqueueReadBuffer(rbuffer, CL_TRUE, 0, sizeof(RETURNDATA) * retdata[i].size(), retdata[i].data());
                        _ASSERT(error == CL_SUCCESS);
                        tQueue.finish();
                        auto& retdat = retdata[i];
                        auto& obj2 = *wCollModels[i].obj2;

                        XMFLOAT3 Pos2;
                        obj2.mPos.posMtx.lock();
                        XMStoreFloat3(&Pos2, XMLoadFloat3(obj2.mPos.position));
                        auto sph2 = obj2.model->uData->Sphere;
                        obj2.mPos.posMtx.unlock();

                        auto dir = fDirection(Pos1, Pos2);
                        auto dist = fDistance(Pos1, Pos2);
                        XMFLOAT3 RelPos;
                        XMStoreFloat3(&RelPos, XMLoadFloat4(&dir)* dist);

                        int coutn = 0;
                        for (auto& r : retdat) {
                            if (r.dist[0] != 0 || r.dist[1] != 0) {

                                XMStoreFloat3(&MoveD1, (XMLoadFloat3(&MoveD1) + (XMLoadFloat3(&obj.model->uData->MappedVertices[r.index[0]][0].normal) * r.dist[0])));
                                XMStoreFloat3(&MoveD2, (XMLoadFloat3(&MoveD2) + (XMLoadFloat3(&obj2.model->uData->MappedVertices[r.index[1]][0].normal) * r.dist[1])));
                                /*
                                                        DebugMTX.lock();
                                std::cout << "Model: " + obj.model->name + " Vert: " + std::to_string(r.index[0]) + " Pos: {" + std::to_string(obj.model->uData->MappedVertices[r.index[0]][r.index[0] % 3].position.x) + ", " +
                                    std::to_string(obj.model->uData->MappedVertices[r.index[0]][r.index[0] % 3].position.y) +", " + std::to_string(obj.model->uData->MappedVertices[r.index[0]][r.index[0] % 3].position.z) + "} " << std::endl;
                                std::cout << "Normal: {" + std::to_string(obj.model->uData->MappedVertices[r.index[0]][0].normal.x) + ", " +
                                    std::to_string(obj.model->uData->MappedVertices[r.index[0]][0].normal.y) + ", " + std::to_string(obj.model->uData->MappedVertices[r.index[0]][0].normal.z) + "} " << std::endl;
                                auto objvert0 = obj.model->uData->MappedVertices[r.index[0]][0].position;
                                auto objvert1 = obj.model->uData->MappedVertices[r.index[0]][1].position;
                                auto objvert2 = obj.model->uData->MappedVertices[r.index[0]][2].position;

                                std::cout << "Mod1 Vert0 pos': {" + std::to_string(objvert0.x) + ", " +
                                    std::to_string(objvert0.y) + ", " + std::to_string(objvert0.z) + "} " << std::endl;
                                std::cout << "Mod1 Vert0 pos': {" + std::to_string(objvert1.x) + ", " +
                                    std::to_string(objvert1.y) + ", " + std::to_string(objvert1.z) + "} " << std::endl;
                                std::cout << "Mod1 Vert0 pos': {" + std::to_string(objvert2.x) + ", " +
                                    std::to_string(objvert2.y) + ", " + std::to_string(objvert2.z) + "} " << std::endl;

                                std::cout << "Model2: " + obj2.model->name + " Vert: " + std::to_string(r.index[1]) + " Pos: {" + std::to_string(obj2.model->uData->MappedVertices[r.index[1]][r.index[1] % 3].position.x) + ", " +
                                    std::to_string(obj2.model->uData->MappedVertices[r.index[1]][r.index[1] % 3].position.y) + ", " + std::to_string(obj2.model->uData->MappedVertices[r.index[1]][r.index[1] % 3].position.z) + "} " << std::endl;
                                auto obj2vert0 = obj2.model->uData->MappedVertices[r.index[1]][0].position;
                                auto obj2vert1 = obj2.model->uData->MappedVertices[r.index[1]][1].position;
                                auto obj2vert2 = obj2.model->uData->MappedVertices[r.index[1]][2].position;

                                std::cout << "Mod2 Vert0 pos': {" + std::to_string(obj2vert0.x) + ", " +
                                    std::to_string(obj2vert0.y) + ", " + std::to_string(obj2vert0.z) + "} " << std::endl;
                                std::cout << "Mod2 Vert0 pos': {" + std::to_string(obj2vert1.x) + ", " +
                                    std::to_string(obj2vert1.y) + ", " + std::to_string(obj2vert1.z) + "} " << std::endl;
                                std::cout << "Mod2 Vert0 pos': {" + std::to_string(obj2vert2.x) + ", " +
                                    std::to_string(obj2vert2.y) + ", " + std::to_string(obj2vert2.z) + "} " << std::endl;
                                std::cout << "Mod 2 rel pos: {" + std::to_string(RelPos.x) + ", " +
                                    std::to_string(RelPos.y) + ", " + std::to_string(RelPos.z) + "} " << std::endl;
                                std::cout << "Normal: {" + std::to_string(obj2.model->uData->MappedVertices[r.index[1]][0].normal.x) + ", " +
                                    std::to_string(obj2.model->uData->MappedVertices[r.index[1]][0].normal.y) + ", " + std::to_string(obj2.model->uData->MappedVertices[r.index[1]][0].normal.z) + "} " << std::endl;



                                DebugMTX.unlock();
                                */

                                coutn++;
                            }
                        }
                        if (coutn != 0) {
                            float massScal1 = (obj.mass / (obj.mass + obj2.mass));
                            float massScal2 = (obj2.mass / (obj.mass + obj2.mass));
                            massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
                            massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

                            XMStoreFloat3(&MoveD1, (XMLoadFloat3(&MoveD1) / coutn) * massScal2);
                            XMStoreFloat3(&MoveD2, (XMLoadFloat3(&MoveD2) / coutn) * massScal1);

                            XMFLOAT3 Zero(0, 0, 0);

                            //Fixxx thissss
                            //CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed, obj2.speed, obj->mass, obj2.mass);
                            obj.Move(fDirection(Zero, MoveD2), fDistance(Zero, MoveD2));
                            obj2.Move(fDirection(Zero, MoveD1), fDistance(Zero, MoveD1));

                        }
                    }


                });


            }
            tProcCollide->gEndWork(refs);







            for (auto i = 0; i < retdata.size(); i++) {
               
            }


        })
        ;
        tO++;
    };
    tMain->gEndWork(refs);
}

void Physics::pSpecReset() {
    //Do stuff here to free memory when Collision calculations are over.
    cmMtx.lock();
    CollModels.resize(0);
    //CollModels.shrink_to_fit();
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

