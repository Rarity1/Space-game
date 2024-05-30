#include "ePhysics.h"



Physics::Physics(mThreadTime* timer, std::vector<Physics::eResource*>& trackedModels, int* UpdateRate) :
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

void Physics::Update() {
    Retrack();
    if (!Retracker.load()) {
        for (auto& mUpdate : trackedModels) {
            cGravity(mUpdate);
            pSpecCollison(mUpdate);
            mMove(mUpdate);
        }
    }
    for (auto& mUpdate : trackedModels) {
        pSpecReset(mUpdate);
    }
}

void Physics::Retrack() {
    if (Retracker.load()) {
        upDist.store(false);
        for (auto& t : distanceThreads)
            t.join();
        distanceThreads.resize(std::size(trackedModels));
        upDist.store(true);
        QueueMTX.lock();
        queue.flush();
        for (int i = 0; i < std::size(trackedModels); i++) {
            std::thread th(&Physics::trackDist, this, std::ref(trackedModels[i]));
            distanceThreads[i] = move(th);

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
            queue.enqueueWriteBuffer(trackedModels[i]->clPositionBuff, CL_TRUE, 0, sizeof(XMFLOAT3), &trackedModels[i]->mPos);
            queue.enqueueWriteBuffer(trackedModels[i]->clCollIndBuff, CL_TRUE, 0, sizeof(int) * std::size(trackedModels[i]->model->uData->WeightCIndex), trackedModels[i]->model->uData->WeightCIndex.data());

            
        }
        queue.flush();
        QueueMTX.unlock();

        Retracker.store(false);
    }
}

void Physics::cGravity(eResource* obj) {
    if (obj->mworld != nullptr) {
        float distance = -fDistance(&obj->mPos.position, &obj->mworld->mPos.position);


        XMFLOAT4 change{ 0,0,0,0 };
        change.x = (obj->mworld->mPos.position.x - obj->mPos.position.x);
        change.y = (obj->mworld->mPos.position.y - obj->mPos.position.y);
        change.z = (obj->mworld->mPos.position.z - obj->mPos.position.z);


        XMStoreFloat4(&change, XMQuaternionNormalize(XMLoadFloat4(&change)));
        obj->grav = change;


        obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(distance, 2))) * timer->time;
    }
}
XMFLOAT4 Physics::fDirection(XMFLOAT3* pos1, XMFLOAT3* pos2) {
    auto x = (pos2->x - pos1->x);
    auto y = (pos2->y - pos1->y);
    auto z = (pos2->z - pos1->z);
    auto mag = fDistance(pos1, pos2);
    return {(x/mag), (y/mag), (z/mag), 0};
}


float Physics::fDistance(XMFLOAT3* pos1, XMFLOAT3* pos2) {
    auto x = pow((pos2->x - pos1->x), 2);
    auto y = pow((pos2->y - pos1->y), 2);
    auto z = pow((pos2->z - pos1->z), 2);
    return sqrt(x + y + z);
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

DirectX::XMFLOAT3 Physics::AddXMFLOAT3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b) {
    DirectX::XMFLOAT3 result{0,0,0};

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

template <typename T> int Physics::sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

void Physics::trackDist(Physics::eResource* tModel) {
  for (auto& m : trackedModels) {
       if (m != tModel) {
           auto reet = new Physics::eResource::tmCollide{ m };
           tModel->tmDist.emplace_back(reet);
       }
  }
  float temptime = 0;
  timer->mtx.lock();
  float urat = *urate;
  timer->mtx.unlock();
  while (upDist.load()) {
      tModel->mPos.posMtx.lock();
      auto posobj = tModel->mPos.position;
      tModel->model->uData->Sphere.Center = tModel->mPos.position;
      tModel->mPos.posMtx.unlock();


      QueueMTX.lock();
      queue.enqueueWriteBuffer(tModel->clPositionBuff, CL_TRUE, 0, sizeof(XMFLOAT3), &tModel->mPos.position);
      QueueMTX.unlock();



      timer->mtx.lock();
      temptime += timer->time;
      timer->mtx.unlock();
          for (auto& m : tModel->tmDist) {
              m->ptModel->mPos.posMtx.lock();
              auto postm = m->ptModel->model->uData->Sphere;
              m->ptModel->mPos.posMtx.unlock();
              m->mtx.lock();
              m->direction = fDirection(&posobj, &postm.Center);
              m->distance = fDistance(&posobj, &postm.Center);
              m->mtx.unlock();

          }
      std::this_thread::sleep_for(std::chrono::nanoseconds(340));
      
  }
  for (auto& r : tModel->tmDist) {
      delete r;
  }
}



void Physics::ProcCollide(Physics::eResource* obj, eResource::tmCollide* tmdist) {
    eResource::tmCollide* tmdist1 = nullptr;
    auto& obj2 = tmdist->ptModel;
    for (auto& tmd1 : tmdist->ptModel->tmDist) {
        if (tmd1->ptModel == obj) {
            tmdist1 = tmd1;
        }
    }
    
    

    if (tmdist1 != nullptr)
    {
        if (tmdist->Collision.load() && tmdist1->Collision.load()) {
            return;
        }
        auto& tmdat1 = obj2->model->uData->bdata;
        auto& tmdat = obj->model->uData->bdata;
        
        obj2->mPos.posMtx.lock();
        auto ob2pos = obj2->mPos.position;
        obj2->mPos.posMtx.unlock();

        obj->mPos.posMtx.lock();
        auto objpos = obj->mPos.position;
        obj->mPos.posMtx.unlock();
        auto dir = fDirection(&objpos, &ob2pos);
        auto dist = fDistance(&ob2pos, &objpos);

        std::vector<WORKDATA> WData;

        std::vector<DirectX::BoundingSphere> tooClose[2];
        for (auto& b2 : tmdat1) {
            for (auto& b : tmdat) {
                auto sph2 = b2.sphere;
                sph2.Center = AddXMFLOAT3(b2.sphere.Center, { dir.x * dist, dir.y * dist, dir.z * dist });
                    if (b.sphere.Intersects(sph2)) {
                        auto ssph2 = b2.smallsphere;
                        ssph2.Center = sph2.Center;
                        if (std::size(b.Indices) > 0 && std::size(b2.Indices) > 0) {
                            WData.emplace_back(WORKDATA{ .bIndex = {b.bIndex, b2.bIndex}, .Position = {{0,0,0}, { dir.x * dist, dir.y * dist, dir.z * dist }} });
                        }
                    }
                
            }
        }

        

        if (std::size(WData) > 0) {
            tmdist->Collision.store(true);
            tmdist1->Collision.store(true);
        }
        else return;

        auto& objcdata = obj->model->uData->cdata;
        auto& objbdata = obj->model->uData->bdata;
        auto& objidata = obj->model->uData->idata;
        auto& objweight = obj->model->uData->weights;
        std::vector<int>& WCollIndex = obj->model->uData->WeightCIndex;
        auto& obj2cdata = obj2->model->uData->cdata;
        auto& obj2bdata = obj2->model->uData->bdata;
        auto& obj2idata = obj2->model->uData->idata;
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

        std::vector<OffsetC> offset(std::size(WData));

        std::vector<int> CalIndex;
        float totmass = obj->mass + obj2->mass;

        {
            for (auto i = 0; i < std::size(WData); i++) {

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
            queue.flush();
            cl::Buffer WorkBuff(context, CL_MEM_READ_ONLY, sizeof(WData) * std::size(WData));
            cl::Buffer offsetbuff(context, CL_MEM_READ_ONLY, std::size(WData) * sizeof(OffsetC));
            cl::Buffer workindices(context, CL_MEM_READ_ONLY, std::size(CalIndex) * sizeof(int));
            queue.enqueueWriteBuffer(WorkBuff, CL_TRUE, 0, std::size(WData) * sizeof(WORKDATA), WData.data());
            queue.enqueueWriteBuffer(offsetbuff, CL_TRUE, 0, std::size(WData) * sizeof(OffsetC), offset.data());
            queue.enqueueWriteBuffer(workindices, CL_TRUE, 0, std::size(CalIndex) * sizeof(int), CalIndex.data());





            std::vector<RETURNDATA> retdat(std::size(WData));
            cl::Buffer ReturnBuff(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * std::size(retdat));
            queue.enqueueWriteBuffer(ReturnBuff, CL_TRUE, 0, sizeof(RETURNDATA) * std::size(retdat), retdat.data());
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




            for (auto i = 0; i < std::size(WData); i++) {
                queue.enqueueNDRangeKernel(collide, cl::NDRange(i, 0, 0), cl::NDRange(1, offset[i].ICount[0], offset[i].ICount[1]), cl::NullRange);
                queue.enqueueReadBuffer(ReturnBuff, CL_TRUE, i * sizeof(RETURNDATA), sizeof(RETURNDATA), &retdat[i]);

            }
            queue.flush();


            QueueMTX.unlock();



            XMFLOAT4 move2{ 0,0,0,0 };
            float move1 = 0;
            XMFLOAT4 move3{ 0,0,0,0 };
            for (auto& r : retdat) {
                if (r.coll) {
                    XMFLOAT4 obbone = { obj->model->uData->bdata[r.index1[0]].sphere.Center.x,obj->model->uData->bdata[r.index1[0]].sphere.Center.y,obj->model->uData->bdata[r.index1[0]].sphere.Center.z, 0};
                    XMFLOAT4 obbone2 = { obj2->model->uData->bdata[r.index1[1]].sphere.Center.x,obj2->model->uData->bdata[r.index1[1]].sphere.Center.y,obj2->model->uData->bdata[r.index1[1]].sphere.Center.z, 0 };
                    XMStoreFloat4(&obbone2, XMLoadFloat4(&obbone2)+(XMLoadFloat4(&dir)*dist));

                    XMFLOAT3 temp = { obbone.x, obbone.y, obbone.z};
                    XMFLOAT3 temp2 = { obbone2.x, obbone2.y, obbone2.z };

                    XMFLOAT4 bonevect = fDirection(&temp, &temp2);

                    XMFLOAT4 Dopt1;
                    XMStoreFloat4(&Dopt1, XMVector4Dot(XMLoadFloat4(&bonevect), XMLoadFloat4(&r.dir[0])));
                    XMFLOAT4 Dopt2;
                    XMStoreFloat4(&Dopt2, XMVector4Dot(XMLoadFloat4(&bonevect), XMLoadFloat4(&r.dir[1])));
                    XMFLOAT4 Dopt3;
                    XMStoreFloat4(&Dopt3, XMVector4Dot(XMLoadFloat4(&bonevect), XMLoadFloat4(&dir)));

                    float val = fabs(Dopt1.x * Dopt2.x * Dopt3.x);

                    if (move1 < val) {
                        move1 = val;
                        XMStoreFloat4(&move2, (XMLoadFloat4(&r.dir[1]) * -r.dist[1]));
                        XMStoreFloat4(&move3, (XMLoadFloat4(&r.dir[0]) * -r.dist[0]));

                    }


                }
            }
            XMStoreFloat4(&obj->pDir, XMLoadFloat4(&obj->pDir) + (XMLoadFloat4(&move2)/2 * -(obj2->mass / totmass)));
            XMStoreFloat4(&obj2->pDir, XMLoadFloat4(&obj2->pDir) + (XMLoadFloat4(&move3)/2 * -(obj->mass / totmass)));
        }

       
        
        
    }
}


void Physics::pSpecCollison(eResource* obj) {
    
    for (auto& m : obj->tmDist) {
        auto& msphere = m->ptModel->model->uData->Sphere;
        if (msphere.Intersects(obj->model->uData->Sphere)) {
            if (!m->Collision.load()) {
                ProcCollide(obj, m);
            }
        }

    }
}

void Physics::pSpecReset(eResource* obj) {
    obj->Collision.store(false);
    for (auto& m : obj->tmDist) {
        m->Collision.store(false);
    }
}



void Physics::mMove(Physics::eResource* mUpdate) {
    mUpdate->mPos.posMtx.lock();
    XMFLOAT4 both = { 0,0,0,0 };
    XMStoreFloat4(&both, (XMLoadFloat4(&mUpdate->velDir) * mUpdate->speed) + (XMLoadFloat4(&mUpdate->grav) * mUpdate->gravpull) + (XMLoadFloat4(&mUpdate->pDir)));
    mUpdate->pDir = {0,0,0,0};
    mUpdate->mPos.lastposition = mUpdate->mPos.position;
    mUpdate->mPos.position = { mUpdate->mPos.position.x + both.x, mUpdate->mPos.position.y + both.y, mUpdate->mPos.position.z + both.z };

    auto& bmodel = mUpdate->model;
    bmodel->cmatrix = XMMatrixTranslation(0, 0, 0) * XMMatrixRotationQuaternion(XMLoadFloat4(&mUpdate->mPos.rotation));
    bmodel->cmatrix *= XMMatrixTranslation(mUpdate->mPos.position.x, mUpdate->mPos.position.y, mUpdate->mPos.position.z);
    bmodel->cmatrix *= XMMatrixRotationQuaternion(XMLoadFloat4(&mUpdate->mPos.orbit));
    mUpdate->mPos.posMtx.unlock();

}
Physics::~Physics() {
    upDist.store(false);
    for (auto& t : distanceThreads)
        t.join();
    for (auto& t : collisionThreads)
        t.join();
    for (auto& m : trackedModels)
        delete m;
    trackedModels = {};
}
