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
        pSpecCollison(mUpdate);
        if (mUpdate->speed != 0 || mUpdate->gravpull != 0 || mUpdate->pspeed != 0 || mUpdate->updated.load()) {
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
        for (int i = 0; i < std::size(trackedModels); i++) {
            std::thread th(&Physics::trackDist, this, std::ref(trackedModels[i]));
            distanceThreads[i] = move(th);
        }
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
        
        auto& tmdat1 = obj2->model->uData->bdata;
        auto& tmdat = obj->model->uData->bdata;

        
        auto dir = fDirection(&obj2->mPos.position, &obj->mPos.position);
        auto dist = fDistance(&obj2->mPos.position, &obj->mPos.position);
        std::vector<CollideS> sd;
        for (auto& b2 : tmdat1) {
            for (auto& b : tmdat) {
                auto sph1 = b.sphere;
                sph1.Center = AddXMFLOAT3(b.sphere.Center, { dir.x * dist, dir.y * dist, dir.z * dist });
                    if (sph1.Intersects(b2.sphere)) {
                        auto ssph1 = b.smallsphere;
                        ssph1.Center = sph1.Center;
                       if(ssph1.Intersects(b2.sphere))
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




        std::vector<ReadX3D::pCollision> A;

        std::vector<bool> tmt;
        tmt.resize(std::size(tmdat));
        std::vector<bool> tcdat;
        tcdat.resize(std::size(objcdata));

        for (auto b : tmt) {
            b = false;
        }
        for (auto b : tcdat) {
            b = false;
        }
        for (auto& c : sd) {
            if (!tmt[c.Index1]) {
                tmt[c.Index1] = true;
                auto& bn = tmdat[c.Index1];
                for (auto& v : bn.Indices) {
                    auto& bdat = objidata[v];
                    if (!tcdat[bdat.normal]) {
                        tcdat[bdat.normal] = true;
                        auto& tri = objcdata[bdat.normal];
                        A.emplace_back(tri);
                    }
                    
                }
            }
        }
        if (std::size(A) > 0) {
            XMFLOAT3 B;

            B = obj->mPos.position;
            auto pee = sizeof(ReadX3D::pCollision);
            obj->clBuff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(ReadX3D::pCollision) * std::size(A));
            cl::Buffer buffer_C(context, CL_MEM_WRITE_ONLY, sizeof(ReadX3D::pCollision) * std::size(A));


            queue.enqueueWriteBuffer(obj->clBuff, CL_TRUE, 0, sizeof(ReadX3D::pCollision) * std::size(A), A.data());

            
            collide.setArg(0, obj->clBuff);
            collide.setArg(3, buffer_C);

            queue.enqueueNDRangeKernel(collide, cl::NullRange, cl::NDRange(std::size(A)), cl::NullRange);
            queue.finish();

            std::vector<ReadX3D::pCollision> C;
            queue.enqueueReadBuffer(buffer_C, CL_TRUE, 0, sizeof(ReadX3D::pCollision) * std::size(A), C.data());
        }
        
    }
}


void Physics::pSpecCollison(eResource* obj) {
    obj->mPos.posMtx.lock();
    for (auto& m : obj->tmDist) {
        m->ptModel->mPos.posMtx.lock();
        auto& msphere = m->ptModel->model->uData->Sphere;
        if (msphere.Intersects(obj->model->uData->Sphere)) {
            if (!m->Collision.load()) {
                ProcCollide(obj, m);
            }
        }
        m->ptModel->mPos.posMtx.unlock();
    }
    obj->mPos.posMtx.unlock();
}

void Physics::pSpecReset(eResource* obj) {
    obj->Collision.store(false);
    for (auto& m : obj->tmDist) {
        m->Collision.store(false);
    }
}

/*std::vector<float> Physics::RayCastColl(std::vector<int>& index, std::vector<ReadX3D::pCollision>& cdata, XMFLOAT4& dir, std::vector<XMFLOAT3>& raypos) {
    std::vector<float> olddist{};
    auto normdir = XMVector3Normalize(XMLoadFloat4(&dir));
    for (auto& c : cdata) {
        XMVECTOR tri10 = XMVectorSet(c.verts[0]->position.x, c.verts[0]->position.y, c.verts[0]->position.z, 0);
        XMVECTOR tri11 = XMVectorSet(c.verts[1]->position.x, c.verts[1]->position.y, c.verts[1]->position.z, 0);
        XMVECTOR tri12 = XMVectorSet(c.verts[2]->position.x, c.verts[2]->position.y, c.verts[2]->position.z, 0);
        float dist = 0.0;
        for (auto& r : raypos) {
            auto rp = XMVectorSet(r.x, r.y, r.z, 0);
            if (DirectX::TriangleTests::Intersects(rp, normdir, tri10, tri11, tri12, dist)) {
                olddist.emplace_back(dist);
                index.emplace_back(c.index);
            }
        }

    }
    return olddist;
}*/



//figure it out smh
/*bool Physics::triCollide(ReadX3D::pCollision* tri, ReadX3D::pCollision* tri2, XMFLOAT4 dir, float dist) {
    auto Origin = AddXMFLOAT3(tri->verts[0].position, {dir.x * dist, dir.y * dist, dir.z * dist});
    auto Origin1 = AddXMFLOAT3(tri->verts[1].position, { dir.x * dist, dir.y * dist, dir.z * dist });
    XMVECTOR Direction1;
    XMVECTOR Direction2;
    XMVECTOR Direction3;
    {
        auto D1 = fDirection(&tri->verts[0].position, &tri->verts[1].position);
        auto D2 = fDirection(&tri->verts[0]->position, &tri->verts[2]->position);
        auto D3 = fDirection(&tri->verts[1]->position, &tri->verts[2]->position);
        Direction1 = DirectX::XMVector3Normalize(XMLoadFloat4(&D1));
        Direction2 = DirectX::XMVector3Normalize(XMLoadFloat4(&D2));
        Direction3 = DirectX::XMVector3Normalize(XMLoadFloat4(&D3));

    }
    auto Distance1 = fDistance(&tri->verts[0]->position, &tri->verts[1]->position);
    auto Distance2 = fDistance(&tri->verts[0]->position, &tri->verts[2]->position);
    auto Distance3 = fDistance(&tri->verts[1]->position, &tri->verts[2]->position);

    auto vect1 = XMLoadFloat3(&tri2->verts[0]->position);
    auto vect2 = XMLoadFloat3(&tri2->verts[1]->position);
    auto vect3 = XMLoadFloat3(&tri2->verts[2]->position);

    float ret = 0;

    bool result = false; 
    //if (DirectX::TriangleTests::Intersects(XMLoadFloat3(&Origin), Direction1, vect1, vect2, vect3, ret))
        //result = ret < Distance1 ? true : false;

    if(result)
    return result;
    return result;
}*/



void Physics::mMove(Physics::eResource* mUpdate) {
    mUpdate->mPos.posMtx.lock();
    XMFLOAT4 both = { 0,0,0,0 };
    XMStoreFloat4(&both, (XMLoadFloat4(&mUpdate->velDir) * mUpdate->speed) + (XMLoadFloat4(&mUpdate->grav) * mUpdate->gravpull) + (XMLoadFloat4(&mUpdate->pDir) * mUpdate->pspeed));
    mUpdate->pDir = { 0,0,0,0 };
    mUpdate->pspeed = 0;
    mUpdate->mPos.lastposition = mUpdate->mPos.position;
    mUpdate->mPos.position = { mUpdate->mPos.position.x + both.x, mUpdate->mPos.position.y + both.y, mUpdate->mPos.position.z + both.z };
    mUpdate->which = RStorage::BOTH;
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
