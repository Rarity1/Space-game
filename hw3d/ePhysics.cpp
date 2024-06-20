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

    auto test = program.build({ device });
    _ASSERT(test == CL_SUCCESS);

    
    queue = cl::CommandQueue{ context, device };

    collide = cl::Kernel(program, "coll");
    cl_ulong size;
    clGetDeviceInfo(device.get(), CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &size, 0);
}

void Physics::pSpecCollison(RStorage::eResource* obj) {
    std::vector<int> PhysUp{};
    auto sph1 = obj->model->uData->Sphere;
    sph1.Center = AddXMFLOAT3(sph1.Center, *obj->mPos.position);
    for (auto m = 0; m < std::size(trackedModels); m++) {
        if (trackedModels[m] != obj) {
            auto sph2 = trackedModels[m]->model->uData->Sphere;
            sph2.Center = AddXMFLOAT3(sph2.Center, *trackedModels[m]->mPos.position);
            if (sph2.Intersects(sph1)) {
                ProcCollide(obj,m);
            }

        }
    }

    QueueMTX.lock();

    queue.flush();
    QueueMTX.unlock();



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



    for (auto& mUpdate : trackedModels) {
        cGravity(mUpdate);
        //Always modify veldir before speccoll. Always run speccoll before mMove
        pSpecCollison(mUpdate);
        mMove(mUpdate);
    }
    pSpecReset();
}

void Physics::Retrack() {
    QueueMTX.lock();
    for (int i = 0; i < std::size(trackedModels); i++) {
        trackedModels[i]->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(ReadX3D::pCollision) * std::size(trackedModels[i]->model->uData->cdata));
        trackedModels[i]->clPositionBuff = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(XMFLOAT3));
        
        std::vector<XMFLOAT3> WBone;
        WBone.resize(std::size(trackedModels[i]->model->uData->bdata));
        for (auto b = 0; b < std::size(WBone); b++) {
            WBone[b] = trackedModels[i]->model->uData->bdata[b].sphere.Center;
        }

        trackedModels[i]->clBoneBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(XMFLOAT3) * std::size(WBone));
        trackedModels[i]->clCollIndBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(int) * std::size(trackedModels[i]->model->uData->WeightCIndex));

        queue.enqueueWriteBuffer(trackedModels[i]->clBoneBuff, CL_TRUE, 0, sizeof(XMFLOAT3) * std::size(WBone), WBone.data());
        queue.enqueueWriteBuffer(trackedModels[i]->clBuff, CL_TRUE, 0, sizeof(ReadX3D::pCollision) * std::size(trackedModels[i]->model->uData->cdata), trackedModels[i]->model->uData->cdata.data());
        queue.enqueueWriteBuffer(trackedModels[i]->clCollIndBuff, CL_TRUE, 0, sizeof(int) * std::size(trackedModels[i]->model->uData->WeightCIndex), trackedModels[i]->model->uData->WeightCIndex.data());
    }
    queue.flush();
    QueueMTX.unlock();
    tmDist.resize(std::size(trackedModels));
    for (auto& b : tmDist) {
        b = new std::atomic<bool>;
    }
    Retracker.store(false);

}

void Physics::cGravity(RStorage::eResource* obj) {
    if (obj->mworld != nullptr) {
        float distance = fDistance(obj->mPos.position, obj->mworld->mPos.position);



        obj->grav = fDirection(obj->mPos.position, obj->mworld->mPos.position);
        obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(distance, 2))) * timer.time;
    } 
}

XMFLOAT4 Physics::fDirection(XMFLOAT3* pos1, XMFLOAT3* pos2) {
    XMFLOAT4 Result{ 0,0,0,0 };
    XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    XMStoreFloat4(&Result, XMLoadFloat3(pos2) - XMLoadFloat3(pos1));
    XMStoreFloat3(&Dist, XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
    d = sqrt(Dist.x);
    if (d > 0) {
        XMStoreFloat4(&Result, XMLoadFloat4(&Result) / d);
        return Result;
    }
    return {0,0,0,0};
}
float Physics::fDistance(XMFLOAT3* pos1, XMFLOAT3* pos2) {
    XMFLOAT4 Result{ 0,0,0,0 };
    XMFLOAT3 Dist{ 0,0,0 };
    float d = 0;
    XMStoreFloat4(&Result, XMLoadFloat3(pos2) - XMLoadFloat3(pos1));
    XMStoreFloat3(&Dist, XMVector3Dot(XMLoadFloat4(&Result), XMLoadFloat4(&Result)));
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



void Physics::CalProportionalSpeed(XMFLOAT4& VelDir1, XMFLOAT4& VelDir2, float& VSpeed1, float& VSpeed2, float& Mass1, float& Mass2) {

    float massScal1 = (Mass1 / (Mass1 + Mass2));
    float massScal2 = (Mass2 / (Mass1 + Mass2));
    massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
    massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

    XMFLOAT4 TempVelD1 = VelDir1;
    float TempVSpeed1 = VSpeed1;

    XMFLOAT4 RelDir;
    XMStoreFloat4(&RelDir, XMVector3Normalize(XMLoadFloat4(&VelDir2)+ XMLoadFloat4(&VelDir1)));
    XMFLOAT4 RelDot;
    XMStoreFloat4(&RelDot, XMVector3Dot(XMLoadFloat4(&RelDir), XMLoadFloat4(&RelDir)));



    XMStoreFloat4(&VelDir1, XMVector3Normalize((XMLoadFloat4(&RelDir)/XMLoadFloat4(&RelDot)) * massScal2));
    VSpeed1 = ((VSpeed1 * RelDot.x) + (VSpeed2 * RelDot.x)) * massScal2;

    XMStoreFloat4(&VelDir2, XMVector3Normalize((XMLoadFloat4(&RelDir) / XMLoadFloat4(&RelDot)) * massScal1));
    VSpeed2 = ((TempVSpeed1 * RelDot.x) + (VSpeed2 * RelDot.x)) * massScal1;
}



void Physics::ProcCollide(RStorage::eResource* obj, int tmindex) {




    auto& obj2 = trackedModels[tmindex];

    if (tmDist[tmindex]->load()) {
        return;
    }
    auto& tmdat1 = obj2->model->uData->bdata;
    auto& tmdat = obj->model->uData->bdata;
    auto ob2pos = *obj2->mPos.position;
    auto objpos = *obj->mPos.position;

    auto dpos = objpos;
    auto dpos2 = ob2pos;

    XMStoreFloat3(&ob2pos, XMLoadFloat3(&ob2pos) + XMLoadFloat4(&obj2->velDir) * obj2->speed);
    XMStoreFloat3(&objpos, XMLoadFloat3(&objpos) + XMLoadFloat4(&obj->velDir) * obj->speed);


    auto dir = fDirection(&objpos, &ob2pos);
    auto dist = fDistance(&ob2pos, &objpos);
    auto pos = XMFLOAT3{ dir.x * dist, dir.y * dist, dir.z * dist };

    std::vector<WORKDATA> WData;
    int wSize = 0;
    {
        for (auto& b2 : tmdat1) {
            for (auto& b : tmdat) {
                auto sph2 = b2.sphere;
                sph2.Center = AddXMFLOAT3(b2.sphere.Center, pos);
                if (b.sphere.Intersects(sph2)) {
                    auto ssph2 = b2.smallsphere;
                    ssph2.Center = sph2.Center;
                    if (std::size(b.Indices) > 0 && std::size(b2.Indices) > 0) {
                        WData.emplace_back(WORKDATA{ .bIndex = {b.bIndex, b2.bIndex}, .Position = {{0,0,0}, pos} });
                    }
                }

            }
        }

        if (std::size(WData) > 0) {
            wSize = std::size(WData);
            tmDist[tmindex]->store(true);
        }
        else return;
    }
    std::vector<RETURNDATA> retdat(wSize);
    std::vector<OffsetC> offset(wSize);



    auto& objcdata = obj->model->uData->cdata;
    auto& objbdata = obj->model->uData->bdata;
    auto& objweight = obj->model->uData->weights;
    std::vector<int>& WCollIndex = obj->model->uData->WeightCIndex;
    auto& obj2cdata = obj2->model->uData->cdata;
    auto& obj2bdata = obj2->model->uData->bdata;
    auto& obj2weight = obj2->model->uData->weights;
    std::vector<int>& TCollIndex = obj2->model->uData->WeightCIndex;

    std::vector<bool> tmt(std::size(tmdat), false);
    std::vector<bool> tmt2(std::size(tmdat1), false);


    int size[2];
    size[0] = std::size(objbdata);
    size[1] = std::size(obj2bdata);


    std::vector<int> WBool(std::size(objbdata), -1);
    std::vector<int> TBool(std::size(obj2bdata), -1);
    int WMAX = 0;
    int TMAX = 0;

    int cinSize = 0;
    
        std::vector<int> CalIndex;
        float totmass = obj->mass + obj2->mass;


        for (auto i = 0; i < wSize; i++) {

            auto& c = WData[i];
            WMAX = std::size(objbdata[c.bIndex[0]].Indices) > WMAX ? std::size(objbdata[c.bIndex[0]].Indices) : WMAX;
            TMAX = std::size(obj2bdata[c.bIndex[1]].Indices) > TMAX ? std::size(obj2bdata[c.bIndex[1]].Indices) : TMAX;
            if (WBool[c.bIndex[0]] == -1) {
                offset[i].ICount[0] = std::size(objbdata[c.bIndex[0]].Indices);
                offset[i].Offset[0] = std::size(CalIndex);
                CalIndex.append_range(objbdata[c.bIndex[0]].Indices);
                WBool[c.bIndex[0]] = offset[i].Offset[0];
            }
            else {
                offset[i].ICount[0] = std::size(objbdata[c.bIndex[0]].Indices);
                offset[i].Offset[0] = WBool[c.bIndex[0]];
            }


            if (TBool[c.bIndex[1]] == -1) {
                offset[i].ICount[1] = std::size(obj2bdata[c.bIndex[1]].Indices);
                offset[i].Offset[1] = std::size(CalIndex);
                CalIndex.append_range(obj2bdata[c.bIndex[1]].Indices);
                TBool[c.bIndex[1]] = offset[i].Offset[1];

            }
            else {
                offset[i].ICount[1] = std::size(obj2bdata[c.bIndex[1]].Indices);
                offset[i].Offset[1] = TBool[c.bIndex[1]];
            }

        }



    QueueMTX.lock();
    //queue.enqueueWriteBuffer(obj->clPositionBuff, CL_TRUE, 0, sizeof(XMFLOAT3), obj->mPos.position);
    //queue.enqueueWriteBuffer(obj2->clPositionBuff, CL_TRUE, 0, sizeof(XMFLOAT3), obj2->mPos.position);


    cl::Buffer WorkBuff(context, CL_MEM_READ_ONLY, sizeof(WData) * wSize);
    cl::Buffer offsetbuff(context, CL_MEM_READ_ONLY, wSize * sizeof(OffsetC));
    cl::Buffer workindices(context, CL_MEM_READ_ONLY, std::size(CalIndex) * sizeof(int));

    queue.enqueueWriteBuffer(WorkBuff, CL_TRUE, 0, wSize * sizeof(WORKDATA), WData.data());
    queue.enqueueWriteBuffer(offsetbuff, CL_TRUE, 0, wSize * sizeof(OffsetC), offset.data());
    queue.enqueueWriteBuffer(workindices, CL_TRUE, 0, std::size(CalIndex) * sizeof(int), CalIndex.data());


    cl::Buffer ReturnBuff(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * wSize);
    queue.enqueueWriteBuffer(ReturnBuff, CL_TRUE, 0, sizeof(RETURNDATA) * wSize, retdat.data());
    queue.flush();

    collide.setArg(0, obj->clBuff);
    collide.setArg(1, obj2->clBuff);
    collide.setArg(2, obj->clBoneBuff);
    collide.setArg(3, obj2->clBoneBuff);
    collide.setArg(4, WorkBuff);
    collide.setArg(5, obj->clCollIndBuff);
    collide.setArg(6, obj2->clCollIndBuff);
    collide.setArg(7, ReturnBuff);
    collide.setArg(8, offsetbuff);
    collide.setArg(9, workindices);
    queue.flush();



    for (auto i = 0; i < wSize; i++) {
        queue.enqueueNDRangeKernel(collide, cl::NDRange(i, 0, 0), cl::NDRange(1, offset[i].ICount[0], offset[i].ICount[1]), cl::NullRange);
        _ASSERT(queue.enqueueReadBuffer(ReturnBuff, CL_TRUE, i * sizeof(RETURNDATA), sizeof(RETURNDATA), &retdat[i]) == CL_SUCCESS);

    }

    queue.flush();
    QueueMTX.unlock();



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

    if (coutn != 0) {
        float massScal1 = (obj->mass / (obj->mass + obj2->mass));
        float massScal2 = (obj2->mass / (obj->mass + obj2->mass));
        massScal1 = massScal1 < 0.00001 ? 0 : massScal1;
        massScal2 = massScal2 < 0.00001 ? 0 : massScal2;

        XMStoreFloat3(&objpos, XMLoadFloat3(&objpos) + (XMLoadFloat4(&move2) * (move1 + 0.1)) * massScal2);
        XMStoreFloat3(&ob2pos, XMLoadFloat3(&ob2pos) + (XMLoadFloat4(&move3) * (move1 + 0.1)) * massScal1);

        auto objpDir = fDirection(&dpos, &objpos);
        auto obj2pDir = fDirection(&dpos2, &ob2pos);

        CalProportionalSpeed(obj->velDir, obj2->velDir, obj->speed, obj2->speed, obj->mass, obj2->mass);
        obj->speed = obj->speed - obj2->friction * massScal2 < 0 ? 0 : obj->speed - obj2->friction * massScal2;
        obj2->speed = obj2->speed - obj->friction * massScal1 < 0 ? 0 : obj2->speed - obj->friction * massScal1;

        XMStoreFloat4(&obj->pDir, XMVector4Normalize(XMLoadFloat4(&obj->pDir) + XMLoadFloat4(&objpDir) * massScal2));
        XMStoreFloat4(&obj2->pDir, XMVector4Normalize(XMLoadFloat4(&obj2->pDir) + XMLoadFloat4(&obj2pDir) * massScal1));

        obj->pspeed += fDistance(&dpos, &objpos) * massScal2;
        obj2->pspeed += fDistance(&dpos2, &ob2pos) * massScal1;

    }
}





void Physics::mMove(RStorage::eResource* mUpdate) {
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
Physics::~Physics() {
    queue.flush();
    for (auto& t : distanceThreads)
        t.join();
    for (auto& t : collisionThreads)
        t.join();
    for (auto& t : tmDist)
        delete t;
}
