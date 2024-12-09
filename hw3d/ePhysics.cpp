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
    _ASSERT(phys ? true : false);
    std::string str(std::istreambuf_iterator<char>{phys}, {});
    sources.push_back({str.c_str(), str.length()});
    
    program = cl::Program{ context, sources };
    _ASSERT(program.build({ device }) == CL_SUCCESS);
    queue = cl::CommandQueue{ context, device };
    clGetDeviceInfo(device.get(), CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &clLocalMemSize, 0);
    
    pThreads = std::vector<THREADS>(coreCount);
}


void Physics::Update() {
    std::thread th([this] {
        pSpecReset();
        ticker.incCount();
        //std::for_each(trackedModels.begin(), trackedModels.end(), [this](auto& m) {cGravity(&m); });
        pSpecCollison();
        mMove(trackedModels);
    });
    if (lastPhyxThread.get_id()._Get_underlying_id() != 0) {
        lastPhyxThread.join();
    }
    lastPhyxThread = move(th);
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
            Verts[trackedModels[i].model->uData->FindIndex(r)].Index[r%3] = r;
        }
        std::vector<int> Mapper(trackedModels[i].model->uData->idata.size());
        for (auto m = 0; m < Mapper.size(); m++) {
            Mapper[m] = trackedModels[i].model->uData->FindIndex(m);
        }

        queue.enqueueWriteBuffer(trackedModels[i].model->clIndexBuff, CL_FALSE, 0, sizeof(INTINDEX) * Verts.size(), Verts.data());
        queue.enqueueWriteBuffer(trackedModels[i].model->clIndexMap, CL_FALSE, 0, sizeof(int) * Mapper.size(), Mapper.data());

        
        queue.enqueueWriteBuffer(trackedModels[i].model->clBoneBuff, CL_FALSE, 0, sizeof(DirectX::XMFLOAT3) * std::size(WBone), WBone.data());


        auto map = trackedModels[i].model->uData->NormalMap;
        for (auto v = 0; v < std::size(trackedModels[i].model->uData->idata); v++) {
            queue.enqueueWriteBuffer(trackedModels[i].model->clBuff, CL_FALSE, v*sizeof(UpVertNorm), sizeof(DirectX::XMFLOAT3), &trackedModels[i].model->uData->MappedVertices[map[v]][v%3].position);
            queue.enqueueWriteBuffer(trackedModels[i].model->clBuff, CL_FALSE, (v * sizeof(UpVertNorm))+ sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3), &trackedModels[i].model->uData->MappedVertices[map[v]][v % 3].normal);
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
Physics::WORKINDI Physics::ProcCollide(RStorage::eResource& obj, RStorage::eResource& obj2, cl::CommandQueue& tQueue, DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist) {

    WORKINDI Result;

    using namespace DirectX;
    XMFLOAT3 pos2;
    XMStoreFloat3(&pos2, XMLoadFloat4(&dir) * dist);

    auto& objudat = obj.model->uData;
    auto& obj2udat = obj2.model->uData;
    std::vector<WORKDATA> WData;

    auto& objbdata = objudat->bdata;
    auto& obj2bdata = obj2udat->bdata;



    //This can be split up for better performance with large amounts of bones
    for (auto& b2 : obj2bdata) {
        for (auto& b : objbdata) {
            auto sph2 = b2.sphere;
            XMStoreFloat3(&sph2.Center, XMLoadFloat3(&b2.sphere.Center) + XMLoadFloat3(&pos2));
            if (b.sphere.Intersects(sph2)) {
                if (std::size(b.Indices) > 0 && std::size(b2.Indices) > 0) {
                    WData.emplace_back(WORKDATA{ .bIndex = {b.bIndex, b2.bIndex}, .Position = pos2 });
                }
            }

        }
    }



    


    //std::future<std::vector<int>> Indices = std::async(std::launch::async, [&WData, &objbdata, &obj2bdata, &objudat, &obj2udat, &pos2] {

    std::vector<int> Indices;
    std::mutex iLock;

    std::vector<std::future<std::vector<int>>> IndVect(WData.size());
    for (auto i = 0; i < WData.size(); i++) {
        IndVect[i] = std::async(std::launch::async, [&WData, i, &objbdata, &obj2bdata, &objudat, &obj2udat, &pos2] {
            auto& objnMap = objudat->NormalMap;
            auto& obj2nMap = obj2udat->NormalMap;
            auto& objvertices = objudat->MappedVertices;
            auto& obj2vertices = obj2udat->MappedVertices;
            using namespace DirectX;
            std::vector<int> Indices;
            auto& wDat = WData[i];
            auto& Bone1 = objbdata[wDat.bIndex[0]];
            auto& Bone2 = obj2bdata[wDat.bIndex[1]];

            XMFLOAT3 b2relPos;
            XMStoreFloat3(&b2relPos, XMLoadFloat3(&pos2) + XMLoadFloat3(&Bone2.sphere.Center));

            int wWorkCount = 0;
            int tWorkCount = 0;

            auto cBoneDir = fDirection(Bone1.sphere.Center, b2relPos);


            //std::future<std::vector<int>> WorkIndi1 = std::async(std::launch::async, [&Bone1, &objvertices, &objnMap, &b2relPos, &cBoneDir] {
            std::vector<int > WorkIndi1;
            for (auto a = 0; a < Bone1.Indices.size(); a++) {
                auto index = Bone1.Indices[a];
                auto& Tri = objvertices[objnMap[index]];
                XMFLOAT3 Norm;
                XMStoreFloat3(&Norm, XMVector3Dot(XMLoadFloat3(&Tri[0].normal), XMLoadFloat3(&b2relPos)));
                if (Norm.x >= 0) {
                    auto& tempbSphere = Bone1.sphere;
                    //Why does this take so long? How Do I make it faster?
                    //if (tempbSphere.Intersects(XMLoadFloat3(&Tri[0].position) + XMLoadFloat4(&cBoneDir), XMLoadFloat3(&Tri[1].position) + XMLoadFloat4(&cBoneDir), XMLoadFloat3(&Tri[2].position) + XMLoadFloat4(&cBoneDir))) {
                        WorkIndi1.emplace_back(index);
                    //}

                }
            }
            //return WorkIndi1;});



            //std::future<std::vector<int>> WorkIndi2 = std::async(std::launch::async, [&Bone2, &obj2vertices, &obj2nMap, &b2relPos, &cBoneDir] {
                std::vector<int> WorkIndi2;
                for (auto a = 0; a < Bone2.Indices.size(); a++) {
                    auto index = Bone2.Indices[a];
                    auto& Tri = obj2vertices[obj2nMap[index]];
                    XMFLOAT3 Norm;
                    XMStoreFloat3(&Norm, XMVector3Dot(XMLoadFloat3(&Tri[0].normal), -XMLoadFloat3(&b2relPos)));
                    if (Norm.x >= 0) {
                        auto& tempbSphere = Bone2.sphere;

                        //if (tempbSphere.Intersects(XMLoadFloat3(&Tri[0].position) - XMLoadFloat4(&cBoneDir), XMLoadFloat3(&Tri[1].position) - XMLoadFloat4(&cBoneDir), XMLoadFloat3(&Tri[2].position) - XMLoadFloat4(&cBoneDir))) {
                            WorkIndi2.emplace_back(index);
                        //}

                    }
                }
               // return WorkIndi2;});

            std::vector<int> temp = WorkIndi1;
            wDat.wWorkCount = temp.size();
            temp.append_range(WorkIndi2);
            wDat.tWorkCount = temp.size() - wDat.wWorkCount;
            return temp;
        });

    }

    for (auto i = 0; i < IndVect.size(); i++) {
        WData[i].tOffset = Indices.size();
        Indices.append_range(IndVect[i].get());
    }
    //return Indices;});




    
    Result.Indices = Indices;
    std::for_each(WData.begin(), WData.end(), [&Result](auto& e) {
        if (e.wWorkCount != 0 && e.tWorkCount != 0) {
            Result.WData.emplace_back(e);
        }
    });
    
    return Result;
}


void Physics::pSpecCollison() {

    //Create looping thread pool before this ever starts then queue this work onto each work thread
    for (auto i = 0; i < trackedModels.size(); i++) {
        pThreads[i % coreCount].tPushWork(std::function<void()>([this, i] {
            using namespace DirectX;
            auto& obj = trackedModels[i];
            XMFLOAT3 Pos1;
            obj.mPos.posMtx.lock();
            XMStoreFloat3(&Pos1, XMLoadFloat3(obj.mPos.position));
            auto sph1 = obj.model->uData->Sphere;
            obj.mPos.posMtx.unlock();


            std::vector<collstruct> wCollModels;
            {
                std::vector<collstruct> LCollModels;


                for (auto m = 0; m < std::size(trackedModels); m++) {
                    if (trackedModels[m] != obj) {
                        XMFLOAT3 Pos2;
                        trackedModels[m].mPos.posMtx.lock();
                        XMStoreFloat3(&Pos2, XMLoadFloat3(trackedModels[m].mPos.position));
                        auto sph2 = trackedModels[m].model->uData->Sphere;
                        trackedModels[m].mPos.posMtx.unlock();
                        auto tdist = fDirection(Pos1, Pos2);
                        XMStoreFloat3(&sph2.Center, XMLoadFloat4(&tdist)*fDistance(Pos1, Pos2));
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

            }


            if(devices.size() < 1) return;

            std::vector<cl::CommandQueue> Queues(wCollModels.size(), cl::CommandQueue{ context, devices.front() });

            std::vector<std::thread> tThreads(wCollModels.size());
            for (auto i = 0; i < wCollModels.size(); i++) {
                auto& obj2 = *wCollModels[i].obj2;
                XMFLOAT3 Pos2;
                obj2.mPos.posMtx.lock();
                XMStoreFloat3(&Pos2, XMLoadFloat3(obj2.mPos.position));
                auto sph2 = obj2.model->uData->Sphere;
                obj2.mPos.posMtx.unlock();
                auto dir = fDirection(Pos1, Pos2);
                auto dist = fDistance(Pos1, Pos2);
                auto& tQueue = Queues[i];
                WORKINDI WorkIndi = ProcCollide(obj, obj2, tQueue, Pos1, Pos2, dir, dist);
                if (WorkIndi.WData.size() != 0) {
                    cl::Buffer WorkBuffer(context, CL_MEM_READ_ONLY, sizeof(WORKDATA) * WorkIndi.WData.size());
                    cl::Buffer IndexBuffer(context, CL_MEM_READ_ONLY, WorkIndi.Indices.size() * sizeof(int));
                    cl::Buffer retbuffer(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * WorkIndi.WData.size());

                    std::vector<RETURNDATA> retdat;
                    auto& WData = WorkIndi.WData;
                    auto& Indices = WorkIndi.Indices;
                    cl::Kernel collide(program, "coll");
                    int wSize = std::size(WData);
                    retdat.resize(wSize);

                    tQueue.enqueueWriteBuffer(WorkBuffer, CL_FALSE, 0, wSize * sizeof(WORKDATA), WData.data());
                    tQueue.enqueueWriteBuffer(IndexBuffer, CL_FALSE, 0, std::size(Indices) * sizeof(int), Indices.data());
                    tQueue.enqueueWriteBuffer(retbuffer, CL_FALSE, 0, wSize * sizeof(RETURNDATA), retdat.data());

                    collide.setArg(0, obj.model->clBuff);
                    collide.setArg(1, obj2.model->clBuff);
                    collide.setArg(2, obj.model->clBoneBuff);
                    collide.setArg(3, obj2.model->clBoneBuff);
                    collide.setArg(4, obj.model->clIndexBuff);
                    collide.setArg(5, obj2.model->clIndexBuff);
                    collide.setArg(6, obj.model->clIndexMap);
                    collide.setArg(7, obj2.model->clIndexMap);
                    collide.setArg(8, WorkBuffer);
                    collide.setArg(9, IndexBuffer);
                    collide.setArg(10, retbuffer);
                    tQueue.flush();

                    for (auto a = 0; a < wSize; a++) {
                        _ASSERT(tQueue.enqueueNDRangeKernel(collide, cl::NDRange(a, 0, 0), cl::NDRange(1, WData[a].wWorkCount, WData[a].tWorkCount), cl::NullRange) == CL_SUCCESS);
                        //tQueue.enqueueNDRangeKernel(collide, cl::NDRange(a, 0, 0), cl::NDRange(0, WData[a].wWorkCount, WData[a].tWorkCount), cl::NullRange);
                    }
                    tQueue.finish();



                    /*
                    for (auto r = 0; r < wSize; r++) {
                        //_ASSERT(tQueue.enqueueReadBuffer(retbuffer, CL_FALSE, sizeof(RETURNDATA) * r, sizeof(RETURNDATA), &retdat[r]) == CL_SUCCESS);
                        //tQueue.enqueueReadBuffer(retbuffer, CL_FALSE, sizeof(RETURNDATA) * r, sizeof(RETURNDATA), &retdat[r]);
                    }
                    */
                    tQueue.enqueueReadBuffer(retbuffer, CL_FALSE, 0, sizeof(RETURNDATA) * wSize, retdat.data());

                    XMFLOAT4 MoveD1{ 0,0,0,0 };
                    XMFLOAT4 MoveD2{ 0,0,0,0 };
                    int coutn = 0;
                    for (auto r = 0; r < std::size(retdat); r++) {
                        if (retdat[r].coll) {

                            XMFLOAT3 Mag{ 0,0,0 };
                            XMStoreFloat3(&Mag, XMVector3Dot(XMLoadFloat4(&MoveD1), XMLoadFloat4(&MoveD1)));
                            if (Mag.x < retdat[r].dist[0]) {
                                XMStoreFloat4(&MoveD1, XMLoadFloat3(&retdat[r].dir[0]) * retdat[r].dist[0]);
                                XMStoreFloat4(&MoveD2, -XMLoadFloat3(&retdat[r].dir[0]) * retdat[r].dist[0]);

                            }


                            coutn++;
                        }
                    }
                    if (coutn != 0) {

                        float massScal1 = (obj.mass / (obj.mass + obj2.mass));
                        float massScal2 = (obj2.mass / (obj.mass + obj2.mass));
                        massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
                        massScal2 = massScal2 < 0.00001 ? 0 : massScal2;


                        XMFLOAT3 obj2pDir;
                        XMStoreFloat3(&obj2pDir, XMLoadFloat3(&Pos2) + (XMLoadFloat4(&MoveD1)) * massScal1);
                        XMFLOAT3 objpDir;
                        XMStoreFloat3(&objpDir, XMLoadFloat3(&Pos1) + (XMLoadFloat4(&MoveD2)) * massScal2);

                        //Fixxx thissss
                        //CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed, obj2.speed, obj->mass, obj2.mass);
                        obj.CollisionUp(fDirection(Pos1, objpDir), fDistance(Pos1, objpDir));
                        obj2.CollisionUp(fDirection(Pos2, obj2pDir), fDistance(Pos2, obj2pDir));

                    }
                }
            }
            Queues.resize(0);
        }));
        
    }

    THREADS::tEndWork(pThreads);

}

void Physics::pSpecReset() {
    //Do stuff here to free memory when Collision calculations are over.
    cmMtx.lock();
    CollModels.resize(0);
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

        auto& bmodel = mUpdate.model;
        bmodel->cmatrix = XMMatrixTranslation(0, 0, 0) * XMMatrixRotationQuaternion(XMLoadFloat4(&mUpdate.mPos.rotation));
        bmodel->cmatrix *= XMMatrixTranslation(mUpdate.mPos.position->x, mUpdate.mPos.position->y, mUpdate.mPos.position->z);
        mUpdate.mPos.posMtx.unlock();
    });

}

Physics::~Physics() {
    if (lastPhyxThread.get_id()._Get_underlying_id() != 0) {
        lastPhyxThread.join();
    }
;

}
