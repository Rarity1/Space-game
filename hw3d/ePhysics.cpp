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
    for (auto& mUpdate : trackedModels) {
        mMove(mUpdate);
        pSpecCollison(mUpdate);
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
        for (int i = 0; i < std::size(trackedModels); i++) {
            std::thread th(&Physics::trackDist, this, std::ref(trackedModels[i]));
            distanceThreads[i] = move(th);

            trackedModels[i]->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(ReadX3D::pCollision) * std::size(trackedModels[i]->model->uData->cdata));
            queue.enqueueWriteBuffer(trackedModels[i]->clBuff, CL_TRUE, 0, sizeof(ReadX3D::pCollision) * std::size(trackedModels[i]->model->uData->cdata), trackedModels[i]->model->uData->cdata.data());
            
        }
        queue.finish();
        
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
        if (tmdist->Collision.load() && tmdist1->Collision.load())
            return;
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

        std::vector<CollideS> sd;
        for (auto& b2 : tmdat1) {
            for (auto& b : tmdat) {
                auto sph2 = b2.sphere;
                sph2.Center = AddXMFLOAT3(b2.sphere.Center, { dir.x * dist, dir.y * dist, dir.z * dist });
                    if (b.sphere.Intersects(sph2)) {
                        auto ssph2 = b2.smallsphere;
                        ssph2.Center = sph2.Center;
                       if(ssph2.Intersects(b.sphere))
                            sd.emplace_back(CollideS{ .Index1 = b.bIndex, .Index2 = b2.bIndex });
                    }
                
            }
        }

        auto& objcdata = obj->model->uData->cdata;
        auto& objidata = obj->model->uData->idata;
        auto& obj2cdata = obj2->model->uData->cdata;
        auto& obj2idata = obj2->model->uData->idata;
        if (std::size(sd) > 0) {
            tmdist->Collision.store(true);
            tmdist1->Collision.store(true);
        }




        

        std::vector<bool> tmt;
        tmt.resize(std::size(tmdat));
        std::vector<bool> tcdat;
        tcdat.resize(std::size(objcdata));

        

        std::vector<bool> tmt2;
        tmt2.resize(std::size(tmdat1));
        std::vector<bool> tcdat2;
        tcdat2.resize(std::size(obj2cdata));

        for (auto b : tmt) {
            b = false;
        }
        for (auto b : tcdat) {
            b = false;
        }
        for (auto b : tmt2) {
            b = false;
        }
        for (auto b : tcdat2) {
            b = false;
        }
        for (auto& c : sd) {
            
        }
        std::vector<ReadX3D::pCollision> WModel;
        std::vector<ReadX3D::pCollision> TModel;

        WModel = objcdata;

        TModel = obj2cdata;
        struct RETURNDATA {
            bool coll;
            int Windex;
            int Tindex;
        };

        
        cl::Buffer buffer_C(context, CL_MEM_READ_WRITE, sizeof(RETURNDATA) * (std::size(WModel)));
        cl::Buffer buffer_D(context, CL_MEM_READ_WRITE, sizeof(XMFLOAT3) * 2);
        cl::Buffer buffer_E(context, CL_MEM_READ_ONLY, sizeof(int) * 2);


        XMFLOAT3 Wpos[2];
        Wpos[0] = objpos;
        Wpos[1] = ob2pos;

        int size[2];
        size[0] = std::size(WModel);
        size[1] = std::size(TModel);

        queue.enqueueWriteBuffer(buffer_D, CL_TRUE, 0, sizeof(XMFLOAT3) * 2, Wpos);
        queue.enqueueWriteBuffer(buffer_E, CL_TRUE, 0, sizeof(int) * 2, size);


        collide.setArg(0, obj->clBuff);
        collide.setArg(1, obj2->clBuff);
        collide.setArg(2, buffer_D);
        collide.setArg(3, buffer_E);


        collide.setArg(4, buffer_C);


        queue.enqueueNDRangeKernel(collide, cl::NullRange, cl::NDRange(std::size(WModel)), cl::NullRange);
        queue.finish();

        std::vector<RETURNDATA> retdat;
        retdat.resize(std::size(WModel));
        queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, sizeof(RETURNDATA) * (std::size(WModel)), retdat.data());
        for (auto& r : retdat) {
            if (r.coll) {
                obj->pDir = fDirection(&ob2pos, &objpos);
                obj->pspeed = 0.5;
            }
        }
        queue.finish();
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
    XMStoreFloat4(&both, (XMLoadFloat4(&mUpdate->velDir) * mUpdate->speed) + (XMLoadFloat4(&mUpdate->grav) * mUpdate->gravpull) + (XMLoadFloat4(&mUpdate->pDir) * mUpdate->pspeed));
    mUpdate->pDir = { 0,0,0,0 };
    mUpdate->pspeed = 0;
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
