#include "ePhysics.h"



Physics::Physics(EngineTime& Clock, std::vector<RStorage::eResource>& trackedModels, const int& UpdateRate) :
    GConst(6.67430 * pow(10, -11)),
    timer(Clock),
    trackedModels(trackedModels),
    urate(UpdateRate)
{

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
}


void Physics::Update() {
    std::thread th([this] {
        if (phyxBusy.try_lock()) {
            pSpecReset();
            ticker.incCount();
            //std::for_each(trackedModels.begin(), trackedModels.end(), [this](auto& m) {cGravity(&m); });
            std::vector<std::thread> tmThreads;
            for (auto& m : trackedModels) {
                std::thread th([this, &m] {pSpecCollison(m); });
                tmThreads.emplace_back(move(th));
            }

            std::for_each(tmThreads.begin(), tmThreads.end(), [this](auto& e) { e.join(); });
            std::for_each(trackedModels.begin(), trackedModels.end(), [this](auto& m) {mMove(m); });
            phyxBusy.unlock();
        }
    });
    if (lastPhyxThread.get_id()._Get_underlying_id() != 0) {
        lastPhyxThread.join();
    }
    lastPhyxThread = move(th);
}




void Physics::trackM() {
    QueueMTX.lock();
    for (int i = 0; i < std::size(trackedModels); i++) {
        trackedModels[i].model->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(UpVertNorm) * std::size(trackedModels[i].model->uData->Vertdata));
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
        for (auto v = 0; v < std::size(trackedModels[i].model->uData->Vertdata); v++) {
            queue.enqueueWriteBuffer(trackedModels[i].model->clBuff, CL_FALSE, v*sizeof(UpVertNorm), sizeof(DirectX::XMFLOAT3), &trackedModels[i].model->uData->Vertdata[v].position);
            queue.enqueueWriteBuffer(trackedModels[i].model->clBuff, CL_FALSE, (v * sizeof(UpVertNorm))+ sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3), &trackedModels[i].model->uData->Vertdata[v].normal);
        }
    }
    queue.flush();
    QueueMTX.unlock();


}

void Physics::cGravity(RStorage::eResource* obj) {
    using namespace DirectX;
    if (obj->mworld != nullptr) {
        obj->grav = fDirection(obj->mPos->position, obj->mworld->mPos->position);
        obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(fDistance(obj->mPos->position, obj->mworld->mPos->position), 2))) * timer.Current();

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



Physics::WORKINDI* Physics::ProcCollide(RStorage::eResource& obj, RStorage::eResource& obj2, cl::CommandQueue& tQueue, cl::Buffer*& ReturnBuff, cl::Buffer*& WorkBuff, cl::Buffer*& IndBuff, DirectX::XMFLOAT3& objpos, DirectX::XMFLOAT3& obj2pos, DirectX::XMFLOAT4& dir, float& dist) {

    WORKINDI* Result = new WORKINDI;

    using namespace DirectX;
    XMFLOAT3 pos;
    XMStoreFloat3(&pos, XMLoadFloat4(&dir) * dist);

    {
        auto& objbdata = obj.model->uData->bdata;
        auto& obj2bdata = obj2.model->uData->bdata;
        for (auto& b2 : obj2bdata) {
            for (auto& b : objbdata) {
                auto sph2 = b2.sphere;
                XMStoreFloat3(&sph2.Center, XMLoadFloat3(&b2.sphere.Center) + XMLoadFloat3(&pos));
                if (b.sphere.Intersects(sph2)) {
                    if (std::size(b.Indices) > 0 && std::size(b2.Indices) > 0) {
                        Result->WData.emplace_back(WORKDATA{ .bIndex = {b.bIndex, b2.bIndex}, .Position = pos });
                    }
                }

            }
        }
        auto& objudat = obj.model->uData;
        auto& obj2udat = obj2.model->uData;

        std::vector<int> Indices;
        std::mutex iLock;
        std::vector<std::thread> wDataT(Result->WData.size());

        for (auto i = 0; i < Result->WData.size(); i++) {
            std::thread th([this, &Result, i, &objudat, &obj2udat, &pos, &Indices, &iLock] {
                auto& WData = Result->WData;
                auto& w = WData[i];


                auto& Bone1 = objudat->bdata[w.bIndex[0]];
                auto& Bone2 = obj2udat->bdata[w.bIndex[1]];

                XMFLOAT3 b2pos;
                XMStoreFloat3(&b2pos, XMLoadFloat3(&pos));
                XMFLOAT4 bdirection = fDirection(&Bone1.sphere.Center, &b2pos);

                std::vector<int> WorkIndi1;
                std::vector<int> WorkIndi2;



                for (auto a = 0; a < Bone1.Indices.size(); a++) {
                    auto& index = Bone1.Indices[a];
                    auto& Tri = objudat->FindTri(index);

                    XMFLOAT3 Norm;
                    XMStoreFloat3(&Norm, XMVector3Dot(XMLoadFloat3(&Tri[0]->normal), XMLoadFloat4(&bdirection)));
                    if (Norm.x >= 0) {
                        auto tempbSphere = Bone2.sphere;
                        XMStoreFloat3(&tempbSphere.Center, XMLoadFloat3(&pos) + XMLoadFloat3(&tempbSphere.Center));

                        if (tempbSphere.Intersects(XMLoadFloat3(&Tri[0]->position), XMLoadFloat3(&Tri[1]->position), XMLoadFloat3(&Tri[2]->position))) {

                            WorkIndi1.emplace_back(index);
                            w.wWorkCount++;
                        }
                    }


                }

                XMFLOAT3 bpos;
                XMStoreFloat3(&bpos, -XMLoadFloat3(&pos));
                for (auto a = 0; a < Bone2.Indices.size(); a++) {
                    auto& index = Bone2.Indices[a];
                    auto& Tri = obj2udat->FindTri(index);

                    XMFLOAT3 Norm;
                    XMStoreFloat3(&Norm, XMVector3Dot(XMLoadFloat3(&Tri[0]->normal), -XMLoadFloat4(&bdirection)));
                    if (Norm.x >= 0) {
                        auto tempbSphere = Bone1.sphere;
                        XMStoreFloat3(&tempbSphere.Center, XMLoadFloat3(&tempbSphere.Center) - XMLoadFloat3(&pos));
                        if (tempbSphere.Intersects(XMLoadFloat3(&Tri[0]->position), XMLoadFloat3(&Tri[1]->position), XMLoadFloat3(&Tri[2]->position))) {

                            WorkIndi2.emplace_back(index);
                            w.tWorkCount++;
                        }
                    }

                }
                iLock.lock();
                w.tOffset = Indices.size();
                Indices.append_range(WorkIndi1);
                Indices.append_range(WorkIndi2);
                iLock.unlock();
            });
            wDataT[i] = move(th);
        }


        std::for_each(wDataT.begin(), wDataT.end(), [](auto& t) {
            t.join();
        });

        {
            auto x = Result->WData.begin();
            while (x != Result->WData.end()) {
                if (x->wWorkCount == 0 || x->tWorkCount == 0) {
                    x = Result->WData.erase(x);
                }
                else {
                    x++;
                }
            }
        }
        Result->Indices.append_range(Indices);
        if (Result->WData.size() == 0) {
            delete Result;
            return nullptr;
        }
    }

    WorkBuff = new cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(WORKDATA) * Result->WData.size());
    IndBuff = new cl::Buffer(context, CL_MEM_READ_ONLY, std::size(Result->Indices) * sizeof(int));
    ReturnBuff = new cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * Result->WData.size());

    return Result;
}


//Fix memory leaks plz. 
void Physics::pSpecCollison(RStorage::eResource& obj) {

    using namespace DirectX;
    XMFLOAT3 Pos1;
    obj.mPos->posMtx.lock();
    XMStoreFloat3(&Pos1, XMLoadFloat3(obj.mPos->position));
    auto sph1 = obj.model->uData->Sphere;
    obj.mPos->posMtx.unlock();
    

    std::vector<collstruct> wCollModels;
    {
        std::vector<collstruct> LCollModels;


        for (auto m = 0; m < std::size(trackedModels); m++) {
            if (trackedModels[m] != obj) {
                XMFLOAT3 Pos2;
                trackedModels[m].mPos->posMtx.lock();
                XMStoreFloat3(&Pos2, XMLoadFloat3(trackedModels[m].mPos->position));
                auto sph2 = trackedModels[m].model->uData->Sphere;
                trackedModels[m].mPos->posMtx.unlock();
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



    std::vector<cl::CommandQueue*> Queues(wCollModels.size(), nullptr);
    std::vector<WORKINDI*> WorkIndi(wCollModels.size());
    std::vector<std::vector<RETURNDATA>*> retdatVect(wCollModels.size(), nullptr);
    std::vector<cl::Buffer*> retbuffers(wCollModels.size());
    std::vector<cl::Buffer*> WorkBuffers(wCollModels.size());
    std::vector<cl::Buffer*> IndexBuffers(wCollModels.size());
    {
        std::vector<std::thread> tThreads(wCollModels.size());
        for (auto i = 0; i < wCollModels.size(); i++) {
            std::thread th([this, &obj, &retbuffers, &WorkBuffers, &IndexBuffers, &wCollModels, i, &Pos1, &Queues, &WorkIndi] {
                auto& obj2 = *wCollModels[i].obj2;
                XMFLOAT3 Pos2;
                obj2.mPos->posMtx.lock();
                XMStoreFloat3(&Pos2, XMLoadFloat3(obj2.mPos->position));
                auto sph2 = obj2.model->uData->Sphere;
                obj2.mPos->posMtx.unlock();
                auto dir = fDirection(&Pos1, &Pos2);
                auto dist = fDistance(&Pos1, &Pos2);
                Queues[i] = new cl::CommandQueue{ context, devices.front() };
                WorkIndi[i] = ProcCollide(obj, obj2, *Queues[i], retbuffers[i], WorkBuffers[i], IndexBuffers[i], Pos1, Pos2, dir, dist);
            });
            tThreads[i] = move(th);

        }
        std::for_each(tThreads.begin(), tThreads.end(), [this](auto& t) {
            t.join();
        });

    }

    


    for (auto i = 0; i < std::size(Queues); i++) {
        if (Queues[i] != nullptr && WorkIndi[i] != nullptr) {
            auto& obj2 = *wCollModels[i].obj2;

            auto& tQueue = *Queues[i];
            auto& WIVect = *WorkIndi[i];
            retdatVect[i] = new std::vector<RETURNDATA>;
            std::vector<RETURNDATA>& retdat = *retdatVect[i];
            auto& WData = WIVect.WData;
            auto& Indices = WIVect.Indices;
            cl::Kernel collide(program, "coll");
            int wSize = std::size(WData);
            retdat.resize(wSize);

            tQueue.enqueueWriteBuffer(*retbuffers[i], CL_FALSE, 0, wSize * sizeof(RETURNDATA), retdat.data());
            tQueue.enqueueWriteBuffer(*IndexBuffers[i], CL_FALSE, 0, std::size(Indices) * sizeof(int), Indices.data());
            tQueue.enqueueWriteBuffer(*WorkBuffers[i], CL_FALSE, 0, wSize * sizeof(WORKDATA), WData.data());

            collide.setArg(0, obj.model->clBuff);
            collide.setArg(1, obj2.model->clBuff);
            collide.setArg(2, obj.model->clBoneBuff);
            collide.setArg(3, obj2.model->clBoneBuff);
            collide.setArg(4, obj.model->clIndexBuff);
            collide.setArg(5, obj2.model->clIndexBuff);
            collide.setArg(6, obj.model->clIndexMap);
            collide.setArg(7, obj2.model->clIndexMap);
            collide.setArg(8, *WorkBuffers[i]);
            collide.setArg(9, *IndexBuffers[i]);
            collide.setArg(10, *retbuffers[i]);
            tQueue.flush();


            for (auto i = 0; i < wSize; i++) {
                _ASSERT(tQueue.enqueueNDRangeKernel(collide, cl::NDRange(i, 0, 0), cl::NDRange(1, WData[i].wWorkCount, WData[i].tWorkCount), cl::NullRange) == CL_SUCCESS);
                //tQueue.enqueueNDRangeKernel(collide, cl::NDRange(i, 0, 0), cl::NDRange(1, WData[i].wWorkCount, WData[i].tWorkCount), cl::NullRange);
            }
        }
    }


    for (auto i = 0; i < std::size(Queues); i++) {
        if (Queues[i] != nullptr) {
            if (WorkIndi[i] != nullptr) {
                auto& obj2 = *wCollModels[i].obj2;
                auto& tQueue = *Queues[i];
                auto& retdat = *retdatVect[i];
                auto& WData = WorkIndi[i]->WData;
                auto& ind = WorkIndi[i]->Indices;
                int wSize = std::size(WData);
                auto& retbuffer = *retbuffers[i];

                    for (auto r = 0; r < wSize; r++) {
                        _ASSERT(tQueue.enqueueReadBuffer(retbuffer, CL_FALSE, sizeof(RETURNDATA) * r, sizeof(RETURNDATA), &retdat[r]) == CL_SUCCESS);
                        //tQueue.enqueueReadBuffer(retbuffer, CL_FALSE, sizeof(RETURNDATA) * r, sizeof(RETURNDATA), &retdat[r]);
                    }
                    XMFLOAT4 MoveD1{ 0,0,0,0 };
                    XMFLOAT4 MoveD2{ 0,0,0,0 };
                    int coutn = 0;
                    for (auto r = 0; r < std::size(retdat); r++) {
                        if (retdat[r].coll) {

                            XMFLOAT3 Mag{0,0,0};
                            XMStoreFloat3(&Mag, XMVector3Dot(XMLoadFloat4(&MoveD1), XMLoadFloat4(&MoveD1)));
                            if (Mag.x < retdat[r].dist[0]) {
                                XMStoreFloat4(&MoveD1, XMLoadFloat3(&retdat[r].dir[0]) * retdat[r].dist[0]);
                                //XMStoreFloat4(&MoveD2, XMLoadFloat3(&retdat[r].dir[1]) * retdat[r].dist[1]);

                            }

                            coutn++;
                        }
                    }
                    if (coutn != 0) {
                        XMFLOAT3 Pos2;
                        //XMStoreFloat3(&futurePos2, XMLoadFloat3(obj2.mPos->position) + (XMLoadFloat4(&obj2.velDir) * obj2.speed));
                        XMStoreFloat3(&Pos2, XMLoadFloat3(obj2.mPos->position));

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
                        obj.CollisionUp(fDirection(&Pos1, &objpDir), fDistance(&Pos1, &objpDir));
                        obj2.CollisionUp(fDirection(&Pos2, &obj2pDir), fDistance(&Pos2, &obj2pDir));

                    }
            }
        }

    }


    for (auto i = 0; i < std::size(Queues); i++) {
        if (Queues[i] != nullptr) {
            delete retbuffers[i];
            delete Queues[i];
            delete retdatVect[i];
            delete WorkIndi[i];
            delete WorkBuffers[i];
            delete IndexBuffers[i];

        }


    }
}

void Physics::pSpecReset() {
    //Do stuff here to free memory when Collision calculations are over.
    cmMtx.lock();
    CollModels.resize(0);
    cmMtx.unlock();
}




float Physics::fDistance(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2) {
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, DirectX::XMVectorSubtract(XMLoadFloat3(pos2), XMLoadFloat3(pos1)));
    DirectX::XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    return d;
}

DirectX::XMFLOAT4 Physics::fDirection(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2) {
    using namespace DirectX;
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, XMLoadFloat3(pos2) - XMLoadFloat3(pos1));
    XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    if (d > 0) {
        XMStoreFloat4(&Result, XMLoadFloat4(&Result) / d);
        return Result;
    }
    return { 0,0,0,0 };
}


void Physics::mMove(RStorage::eResource& mUpdate) {
    using namespace DirectX;
    mUpdate.mPos->posMtx.lock();

    mUpdate.mPos->lastposition = *mUpdate.mPos->position;

    if (mUpdate.CollCheck()) {
        auto pDir = mUpdate.CollDir();
        XMStoreFloat3(mUpdate.mPos->position, XMLoadFloat3(mUpdate.mPos->position) + XMLoadFloat4(&pDir));
        mUpdate.CollReset();
    }

    XMStoreFloat3(mUpdate.mPos->position, XMLoadFloat3(mUpdate.mPos->position) + XMLoadFloat4(&mUpdate.velDir) * (mUpdate.speed));


    auto& bmodel = mUpdate.model;
    bmodel->cmatrix = XMMatrixTranslation(0, 0, 0) * XMMatrixRotationQuaternion(XMLoadFloat4(&mUpdate.mPos->rotation));
    bmodel->cmatrix *= XMMatrixTranslation(mUpdate.mPos->position->x, mUpdate.mPos->position->y, mUpdate.mPos->position->z);
    mUpdate.mPos->posMtx.unlock();

}

Physics::~Physics() {
    if (lastPhyxThread.get_id()._Get_underlying_id() != 0) {
        lastPhyxThread.join();
    }
    for (auto& t : distanceThreads)
        t.join();
    for (auto& t : collisionThreads)
        t.join();
}
