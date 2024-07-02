#include "ePhysics.h"



Physics::Physics(mThreadTime& timer, std::vector<RStorage::eResource*>& trackedModels, int& UpdateRate) :
    GConst(6.67430 * pow(10, -11)),
    timer(timer),
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

void Physics::pSpecCollison(RStorage::eResource* obj) {
    std::vector<int> PhysUp{};
    auto sph1 = obj->model->uData->Sphere;
    sph1.Center = AddXMFLOAT3(sph1.Center, *obj->mPos.position);
    std::vector<cl::CommandQueue*> Queues(std::size(trackedModels), nullptr);
    std::vector<std::vector<void*>> WorkIndi(std::size(trackedModels));


    std::vector<std::vector<RETURNDATA>*> retdatVect(std::size(trackedModels));
    std::vector<cl::Buffer*> retbuffers(std::size(trackedModels));
    std::vector<cl::Buffer*> WorkBuffers(std::size(trackedModels));
    std::vector<cl::Buffer*> IndexBuffers(std::size(trackedModels));


    for (auto m = 0; m < std::size(trackedModels); m++) {
        if (trackedModels[m] != obj) {

            auto sph2 = trackedModels[m]->model->uData->Sphere;
            sph2.Center = AddXMFLOAT3(sph2.Center, *trackedModels[m]->mPos.position);
            if (sph2.Intersects(sph1)) {
                Queues[m] = new cl::CommandQueue{ context, devices.front() };
                retdatVect[m] = new std::vector<RETURNDATA>;
                WorkIndi[m] = ProcCollide(obj, m, *Queues[m], retbuffers[m], WorkBuffers[m], IndexBuffers[m]);
            }

        }
    }

    for (auto i = 0; i < std::size(Queues); i++) {
        if (Queues[i] != nullptr && std::size(WorkIndi[i]) > 0) {
            auto& obj2 = trackedModels[i];

            auto& tQueue = *Queues[i];
            auto& WIVect = WorkIndi[i];
            std::vector<RETURNDATA>& retdat = *retdatVect[i];
            auto& WData = *(std::vector<WORKDATA>*)WIVect[0];
            auto& Indices = *(std::vector<int>*)WIVect[1];
            cl::Kernel collide(program, "coll");
            int wSize = std::size(WData);
            retdat.resize(wSize);

            tQueue.enqueueWriteBuffer(*retbuffers[i], CL_TRUE, 0, wSize * sizeof(RETURNDATA), retdat.data());
            tQueue.enqueueWriteBuffer(*IndexBuffers[i], CL_TRUE, 0, std::size(Indices) * sizeof(int), Indices.data());
            tQueue.enqueueWriteBuffer(*WorkBuffers[i], CL_TRUE, 0, wSize * sizeof(WORKDATA), WData.data());
            tQueue.flush();

            collide.setArg(0, obj->model->clBuff);
            collide.setArg(1, obj2->model->clBuff);
            collide.setArg(2, obj->model->clBoneBuff);
            collide.setArg(3, obj2->model->clBoneBuff);
            collide.setArg(4, obj->model->clIndexBuff);
            collide.setArg(5, obj2->model->clIndexBuff);


            collide.setArg(6, *WorkBuffers[i]);
            collide.setArg(7, *IndexBuffers[i]);
            collide.setArg(8, *retbuffers[i]);

            for (auto i = 0; i < wSize; i++) {
                _ASSERT(tQueue.enqueueNDRangeKernel(collide, cl::NDRange(i, 0, 0), cl::NDRange(1, WData[i].wWorkCount, WData[i].tWorkCount), cl::NullRange) == CL_SUCCESS);
            }
            tQueue.finish();
        }

    }
    using namespace DirectX;
    for (auto i = 0; i < std::size(Queues); i++) {
        if (Queues[i] != nullptr) {
            if (std::size(WorkIndi[i]) > 0) {
                auto& obj2 = trackedModels[i];
                auto& tQueue = *Queues[i];
                auto& retdat = *retdatVect[i];
                auto& WData = *(std::vector<WORKDATA>*)WorkIndi[i][0];
                auto& ind = *(std::vector<int>*)WorkIndi[i][1];
                int wSize = std::size(WData);
                auto& retbuffer = *retbuffers[i];
                auto pee = tQueue.enqueueReadBuffer(retbuffer, CL_TRUE, 0, sizeof(RETURNDATA) * wSize, retdat.data());
                if (pee != 0) {
                    return;
                }
                tQueue.finish();
                XMFLOAT4 move2{ 0,0,0,0 };
                float move1 = 0;
                XMFLOAT4 move3{ 0,0,0,0 };
                int coutn = 0;

                for (auto i = 0; i < std::size(retdat); i++) {
                    if (retdat[i].coll) {
                        if (move1 < retdat[i].dist[0] || move1 == 0) {
                            auto dp = XMVector3Dot(XMLoadFloat4(&retdat[i].dir[1]), XMLoadFloat4(&retdat[i].dir[0]));
                            XMStoreFloat4(&move2, XMVector3Normalize(XMLoadFloat4(&retdat[i].dir[1]) + (XMLoadFloat4(&retdat[i].dir[0]) * dp)));
                            XMStoreFloat4(&move3, XMVector3Normalize(XMLoadFloat4(&retdat[i].dir[0]) + (XMLoadFloat4(&retdat[i].dir[1]) * dp)));
                            move1 = retdat[i].dist[0];

                        }

                        coutn++;
                    }
                }
                retdat.resize(0);
                if (coutn != 0) {
                    auto ob2pos = *obj2->mPos.position;
                    auto objpos = *obj->mPos.position;

                    auto dpos = objpos;
                    auto dpos2 = ob2pos;

                    XMStoreFloat3(&ob2pos, XMLoadFloat3(&ob2pos) + XMLoadFloat4(&obj2->velDir) * obj2->speed);
                    XMStoreFloat3(&objpos, XMLoadFloat3(&objpos) + XMLoadFloat4(&obj->velDir) * obj->speed);

                    float massScal1 = (obj->mass / (obj->mass + obj2->mass));
                    float massScal2 = (obj2->mass / (obj->mass + obj2->mass));
                    massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
                    massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

                    XMStoreFloat3(&objpos, XMLoadFloat3(&objpos) + (XMLoadFloat4(&move2) * (move1 + 0.1)) * massScal2);
                    XMStoreFloat3(&ob2pos, XMLoadFloat3(&ob2pos) + (XMLoadFloat4(&move3) * (move1 + 0.1)) * massScal1);

                    auto objpDir = fDirection(&dpos, &objpos);
                    auto obj2pDir = fDirection(&dpos2, &ob2pos);

                    CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed, obj2->speed, obj->mass, obj2->mass);

                    auto oldpDir = obj->pDir;
                    float oldpspeed = obj->pspeed;
                    XMStoreFloat4(&objpDir, XMLoadFloat4(&oldpDir) * oldpspeed + XMLoadFloat4(&objpDir) * (fDistance(&dpos, &objpos) * massScal2));
                    XMFLOAT4 obdp = { 0,0,0,0 };
                    XMStoreFloat4(&obdp, XMVector3Dot(XMLoadFloat4(&objpDir), XMLoadFloat4(&objpDir)));
                    XMStoreFloat4(&obj->pDir, XMVector3Normalize(XMLoadFloat4(&objpDir)));
                    obj->pspeed = sqrt(obdp.x);



                    auto oldpDir2 = obj2->pDir;
                    float oldpspeed2 = obj2->pspeed;
                    XMStoreFloat4(&obj2pDir, XMLoadFloat4(&oldpDir2) * oldpspeed2 + XMLoadFloat4(&obj2pDir) * (fDistance(&dpos2, &ob2pos) * massScal1));
                    XMFLOAT4 obdp2 = { 0,0,0,0 };
                    XMStoreFloat4(&obdp2, XMVector3Dot(XMLoadFloat4(&obj2pDir), XMLoadFloat4(&obj2pDir)));
                    XMStoreFloat4(&obj2->pDir, XMVector3Normalize(XMLoadFloat4(&obj2pDir)));
                    obj2->pspeed = sqrt(obdp2.x);

                }
                delete retbuffers[i];
                delete WorkBuffers[i];
                delete IndexBuffers[i];
                delete Queues[i];
                delete retdatVect[i];
                delete (std::vector<WORKDATA>*)WorkIndi[i][0];
                delete (std::vector<int>*)WorkIndi[i][1];
                WorkIndi[i].resize(0);
            }
            else {
                delete Queues[i];
                delete retdatVect[i];
            }
        }


    }
}

void Physics::pSpecReset() {
    for (auto& m : trackedModels) {
        m->Collision.store(false);

    }
    for (auto& m : tmDist) {
        m->store(false);
    }
}


void Physics::Update() {
    if (Retracker.load())
        Retrack();

    std::vector<int> temp(std::size(trackedModels), 1);


    for (auto m = 0; m < std::size(trackedModels); m++) {
        cGravity(trackedModels[m]);
        auto tf = std::ref(temp[m]);
        //Always modify veldir before speccoll. Always run speccoll before mMove
        std::thread([this, tf, m] {pSpecCollison(trackedModels[m]); tf.get() = 0; }).detach();
    }

    while (VectThreadCheck(temp)) {
        for (auto m = 0; m < std::size(trackedModels); m++) {
            if (temp[m] == 0) {
                mMove(trackedModels[m]);
                temp[m] = 2;
            }
        }

    }
    pSpecReset();
}

void Physics::Retrack() {
    QueueMTX.lock();
    for (int i = 0; i < std::size(trackedModels); i++) {
        trackedModels[i]->model->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(DirectX::XMFLOAT3) * std::size(trackedModels[i]->model->uData->Vertdata));
        trackedModels[i]->clPositionBuff = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(DirectX::XMFLOAT3));

        std::vector<DirectX::XMFLOAT3> WBone;
        std::vector<int> IndexOffset;
        WBone.resize(std::size(trackedModels[i]->model->uData->bdata));
        for (auto b = 0; b < std::size(WBone); b++) {
            WBone[b] = trackedModels[i]->model->uData->bdata[b].sphere.Center;
        }
        trackedModels[i]->model->clBoneBuff = cl::Buffer(context, CL_MEM_READ_ONLY, (sizeof(DirectX::XMFLOAT3) + sizeof(int)*2) * std::size(WBone));
        trackedModels[i]->model->clIndexBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(INTINDEX) * std::size(trackedModels[i]->model->uData->idata)/3);


        std::vector<std::vector<int>> Verts(std::size(trackedModels[i]->model->uData->idata)/3);

        for (auto r = 0; r < std::size(trackedModels[i]->model->uData->idata); r++) {
            Verts[trackedModels[i]->model->uData->FindIndex(r)].emplace_back(r);
        }

        for (auto s = 0; s < std::size(Verts); s++) {
            queue.enqueueWriteBuffer(trackedModels[i]->model->clIndexBuff, CL_FALSE, sizeof(INTINDEX) * s, sizeof(INTINDEX), Verts[s].data());
        }
        queue.enqueueWriteBuffer(trackedModels[i]->model->clBoneBuff, CL_FALSE, 0, sizeof(DirectX::XMFLOAT3) * std::size(WBone), WBone.data());
        for (auto v = 0; v < std::size(trackedModels[i]->model->uData->Vertdata); v++) {
            queue.enqueueWriteBuffer(trackedModels[i]->model->clBuff, CL_FALSE, v*sizeof(DirectX::XMFLOAT3), sizeof(DirectX::XMFLOAT3), &trackedModels[i]->model->uData->Vertdata[v].position);
        }
    }
    queue.flush();
    QueueMTX.unlock();
    //tmDist.resize(std::size(trackedModels));
    for (auto& b : tmDist) {
       // b = new std::atomic<bool>;
    }
    Retracker.store(false);

}

void Physics::cGravity(RStorage::eResource* obj) {
    using namespace DirectX;
    if (obj->mworld != nullptr) {
        obj->grav = fDirection(obj->mPos.position, obj->mworld->mPos.position);
        obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(fDistance(obj->mPos.position, obj->mworld->mPos.position), 2))) * timer.time;

        float mag = obj->speed + obj->gravpull;
        DirectX::XMFLOAT3 dotpro = { 0,0,0 };
        auto Tempvel = obj->velDir;
        XMStoreFloat4(&Tempvel,  XMLoadFloat4(&Tempvel)*obj->speed + XMLoadFloat4(&obj->grav)*obj->gravpull);
        XMStoreFloat3(&dotpro, XMVector3Dot( XMLoadFloat4(&Tempvel), XMLoadFloat4(&Tempvel)));
        obj->speed = sqrt(dotpro.x);
        XMStoreFloat4(&obj->velDir, XMLoadFloat4(&Tempvel) / obj->speed);
    } 
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
    return {0,0,0,0};
}
float Physics::fDistance(DirectX::XMFLOAT3* pos1, DirectX::XMFLOAT3* pos2) {
    DirectX::XMFLOAT4 Result{ 0,0,0,0 };
    DirectX::XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    DirectX::XMStoreFloat4(&Result, DirectX::XMVectorSubtract(XMLoadFloat3(pos2), XMLoadFloat3(pos1)));
    XMStoreFloat3(&Dist, DirectX::XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    return d;
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

DirectX::XMFLOAT3 Physics::AddXMFLOAT3(DirectX::XMFLOAT3& a, DirectX::XMFLOAT3& b) {
    DirectX::XMFLOAT3 result{0,0,0};

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

template <typename T> int Physics::sgn(T val) {
    return (T(0) < val) - (val < T(0));
}



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


//Return is Workdata vector then int vector
std::vector <void *> Physics::ProcCollide(RStorage::eResource* obj, int tmindex, cl::CommandQueue& tQueue, cl::Buffer*& ReturnBuff, cl::Buffer*& WorkBuff, cl::Buffer*& IndBuff) {

    std::vector<void*> Result;
    using namespace DirectX;



    auto& obj2 = trackedModels[tmindex];
    auto ob2pos = *obj2->mPos.position;
    auto objpos = *obj->mPos.position;

    XMStoreFloat3(&ob2pos, XMLoadFloat3(&ob2pos) + XMLoadFloat4(&obj2->velDir) * obj2->speed);
    XMStoreFloat3(&objpos, XMLoadFloat3(&objpos) + XMLoadFloat4(&obj->velDir) * obj->speed);


    auto dir = fDirection(&objpos, &ob2pos);
    auto dist = fDistance(&ob2pos, &objpos);
    auto pos = XMFLOAT3{ dir.x * dist, dir.y * dist, dir.z * dist };

    //This is where the crazy happens
    std::vector<WORKDATA>& WData = *(std::vector<WORKDATA>*)Result.emplace_back(new std::vector<WORKDATA>);
    std::vector<int>& Indices = *(std::vector<int>*)Result.emplace_back(new std::vector<int>);

    int wSize = 0;
    {
        auto& objbdata = obj->model->uData->bdata;
        auto& obj2bdata = obj2->model->uData->bdata;
        for (auto& b2 : obj2bdata) {
            for (auto& b : objbdata) {
                auto sph2 = b2.sphere;
                sph2.Center = AddXMFLOAT3(b2.sphere.Center, pos);
                if (b.sphere.Intersects(sph2)) {
                    auto ssph2 = b2.smallsphere;
                    ssph2.Center = sph2.Center;
                    if (std::size(b.Indices) > 0 && std::size(b2.Indices) > 0) {
                        WData.emplace_back(WORKDATA{ .bIndex = {b.bIndex, b2.bIndex}, .Position = {pos.x,pos.y,pos.z} });
                    }
                }

            }
        }
        std::vector<std::vector<int>> WIndices(std::size(WData));
        std::vector<std::vector<int>> TIndices(std::size(WData));

        std::vector<std::thread> Threads(std::size(WData));

        int counter = 0;
        int counter2 = 0;

        auto& objudat = obj->model->uData;
        auto& obj2udat = obj2->model->uData;

        std::for_each(WData.begin(), WData.end(), [this, &counter, &WIndices, &TIndices, &objudat, &obj2udat, &dir](WORKDATA w) {
            auto& Bone1 = objudat->bdata[w.bIndex[0]];
            auto& Bone2 = obj2udat->bdata[w.bIndex[1]];
            XMFLOAT3 b2pos;
            XMStoreFloat3(&b2pos, XMLoadFloat3(&Bone2.sphere.Center) + XMLoadFloat4(&dir));
            XMFLOAT4 bdirection = fDirection(&Bone1.sphere.Center, &b2pos);
            auto& Win = WIndices[counter];
            auto& Tin = TIndices[counter];
            for (auto& ind : Bone1.Indices) {
                auto& Tri = objudat->FindTri(ind);
                XMFLOAT3 Norm;
                XMStoreFloat3(&Norm, XMVector3Dot(XMLoadFloat3(&Tri[1]->normal) + XMLoadFloat3(&Tri[0]->normal) + XMLoadFloat3(&Tri[2]->normal), XMLoadFloat4(&bdirection)));
                if (Norm.x >= 0) {
                    Win.emplace_back(objudat->FindIndex(ind));
                }
            }

            for (auto& ind : Bone2.Indices) {
                auto& Tri = obj2udat->FindTri(ind);
                XMFLOAT3 Norm;
                XMStoreFloat3(&Norm, XMVector3Dot(XMLoadFloat3(&Tri[1]->normal) + XMLoadFloat3(&Tri[0]->normal) + XMLoadFloat3(&Tri[2]->normal), XMLoadFloat4(&bdirection)));
                if (Norm.x <= 0) {
                    Tin.emplace_back(obj2udat->FindIndex(ind));
                }
            }
            counter++; 
        });


        if (std::size(WData) > 0) {
            wSize = std::size(WData);
            for (auto i = 0; i < wSize; i++) {
                WData[i].wWorkCount = std::size(WIndices[i]);
                WData[i].tWorkCount = std::size(TIndices[i]);
                WData[i].tOffset = std::size(Indices);

                Indices.append_range(WIndices[i]);
                Indices.append_range(TIndices[i]);
            }
        }
        else {
            delete Result[0];
            delete Result[1];
            Result.resize(0);
            return Result;
        }
    }

    WorkBuff = new cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(WORKDATA) * wSize);
    IndBuff = new cl::Buffer(context, CL_MEM_READ_ONLY, std::size(Indices) * sizeof(int));
    ReturnBuff = new cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * wSize);

    return Result;
}






void Physics::mMove(RStorage::eResource* mUpdate) {
    using namespace DirectX;
    mUpdate->mPos.posMtx.lock();

    mUpdate->mPos.lastposition = *mUpdate->mPos.position;

    if(mUpdate->pspeed != 0)
    XMStoreFloat3(mUpdate->mPos.position, XMLoadFloat3(mUpdate->mPos.position) + XMLoadFloat4(&mUpdate->pDir)*mUpdate->pspeed);

    XMStoreFloat3(mUpdate->mPos.position, XMLoadFloat3(mUpdate->mPos.position) + XMLoadFloat4(&mUpdate->velDir) * (mUpdate->speed));

    mUpdate->pDir = {0,0,0,0};
    mUpdate->pspeed = 0.0;

    auto& bmodel = mUpdate->model;
    bmodel->cmatrix = XMMatrixTranslation(0, 0, 0) * XMMatrixRotationQuaternion(XMLoadFloat4(&mUpdate->mPos.rotation));
    bmodel->cmatrix *= XMMatrixTranslation(mUpdate->mPos.position->x, mUpdate->mPos.position->y, mUpdate->mPos.position->z);
    mUpdate->mPos.posMtx.unlock();

}
bool Physics::VectThreadCheck(std::vector<int>& BoolV)
{
    bool result = false;
    for (auto& b : BoolV) {
       result = result ? result : b != 0 && b != 2;
    }
    return result;
}
Physics::~Physics() {
    queue.flush();
    for (auto& t : distanceThreads)
        t.join();
    for (auto& t : collisionThreads)
        t.join();
    for (auto& t : tmDist)
        delete t;
}
