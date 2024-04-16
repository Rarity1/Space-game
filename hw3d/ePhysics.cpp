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
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

    _ASSERT(devices.size() > 0);

    auto& device = devices.front();
    auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
    auto version = device.getInfo<CL_DEVICE_VERSION>();
    std::ifstream ephy("ePhysics.cl");
    std::string src(std::istreambuf_iterator<char>(ephy), (std::istreambuf_iterator<char>()));
    //cl::Program::Sources sources(src.length(), std::make_pair(src.c_str(), src.length()));

}

void Physics::Update() {
    Retrack();
    for (auto& mUpdate : trackedModels) {
        pSpecCollison(mUpdate);
        if (mUpdate->speed != 0 || mUpdate->gravpull != 0 || mUpdate->pspeed != 0 || mUpdate->updated.load()) {
            mMove(mUpdate);
        }
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
    auto mag = abs(x) + abs(y) + abs(z);
    return {(float)(x/mag), (float)(y/mag) ,(float)(z/mag), 0};
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
      for (auto& m : tModel->tmDist) {
          bool checkdis = false;


          m->ptModel->mPos.posMtx.lock();
          auto postm = m->ptModel->mPos.position;
          m->ptModel->mPos.posMtx.unlock();

          auto dist = fDistance(&posobj, &postm);
          if (dist < 150) {
              m->mtx.lock();
              m->direction = fDirection(&posobj, &postm);
              m->distance = dist;
              m->mtx.unlock();
          }
          else {
              timer->mtx.lock();
              temptime += timer->time;
              timer->mtx.unlock();
              if (temptime > 2.0 / urat) {
                  m->mtx.lock();
                  m->distance = dist;
                  m->mtx.unlock();
              }

          }
      }
  }
      
}



void Physics::ProcCollide(Physics::eResource* obj, eResource::tmCollide* tmdist) {
    eResource::tmCollide* tmdist1 = nullptr;
    auto obj2 = tmdist->ptModel;
    for (auto& tmd1 : tmdist->ptModel->tmDist) {
        if (tmd1->ptModel == obj) {
            tmdist1 = tmd1;
        }
    }
    
    if (tmdist1 != nullptr)
    {
        obj2->mPos.posMtx.lock();
        auto obj2pos = obj2->mPos.position;
        obj2->mPos.posMtx.unlock();

        std::vector<int> cindex{};
        std::vector<int> cindex2{};

        std::vector<XMFLOAT3> tscdat{};
        std::vector<XMFLOAT3> tscdat1{};

        std::vector<std::vector<int>> Coll{};

        auto& tmdat1 = obj2->model->uData->bdata;
        auto& tmdat = obj->model->uData->bdata;

        std::vector<CollideS> sd;
        for (auto& b2 : tmdat1) {
            for (auto& b : tmdat) {
                    if (b.sphere.Intersects(b2.sphere)) {
                        sd.emplace_back(CollideS{ .Index1 = b.bIndex, .Index2 = b2.bIndex });
                    }
                
            }
        }

        std::vector<CollideS*> coll;
        for (auto& c : sd) {
            auto& bn = tmdat[c.Index1];
            auto& bn2 = tmdat1[c.Index2];
            auto b1sph = bn.smallsphere;
            b1sph.Center = AddXMFLOAT3(b1sph.Center, obj->mPos.position);
            auto b2sph = bn2.sphere;
            b2sph.Center = AddXMFLOAT3(b2sph.Center, obj2pos);

            if (b1sph.Intersects(b2sph)) {
                auto dir = fDirection(&b2sph.Center, &b1sph.Center);
                auto dis = fDistance(&b1sph.Center, &b2sph.Center);
                c.speed = (b1sph.Radius + b2sph.Radius) - dis;
                c.dir = dir;
                coll.emplace_back(&c);
            }
        }
        if (std::size(coll) > 0) {
            obj->pDir = coll[0]->dir;
            obj->pspeed = coll[0]->speed;
        }
    }
}


void Physics::pSpecCollison(eResource* obj) {
    obj->mPos.posMtx.lock();
    for (auto& m : obj->tmDist) {
        m->ptModel->mPos.posMtx.lock();
        auto msphere = m->ptModel->model->uData->Sphere;
        m->ptModel->mPos.posMtx.unlock();
        if (msphere.Intersects(obj->model->uData->Sphere)) {
            ProcCollide(obj, m);
        }
    }
    obj->mPos.posMtx.unlock();
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
bool Physics::triCollide(ReadX3D::pCollision* tri, ReadX3D::pCollision* tri2, XMFLOAT3* tripos, XMFLOAT3* tri2pos) {
    //return DirectX::TriangleTests::Intersects(tri10, tri11, tri12, tri20, tri21, tri22);
    return false;
}



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

}
