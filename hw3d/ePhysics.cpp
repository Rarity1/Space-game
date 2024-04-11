#include "ePhysics.h"



Physics::Physics(mThreadTime* timer, std::vector<Physics::eResource*>& trackedModels, int* UpdateRate) :
    GConst(6.67430 * pow(10, -11)),
    timer(timer),
    trackedModels(trackedModels),
    urate(UpdateRate)
{

}

void Physics::Update() {
    Retrack();
    std::vector<std::thread> collthread;
    for (auto& mUpdate : trackedModels) {
        if (mUpdate->speed != 0 || mUpdate->gravpull != 0 || mUpdate->updated.load()) {
            
            mUpdate->mPos.lastposition = mUpdate->mPos.position;
            mMove(mUpdate);
        }
        mMove(mUpdate);
    }
    for (auto& t : collthread) {
        t.join();
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

float Physics::fDistance(XMFLOAT3* pos1, XMFLOAT3* pos2) {
    auto x = pos2->x - pos1->x;
    auto y = pos2->y - pos1->y;
    auto z = pos2->z - pos1->z;
    return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
}

DirectX::XMFLOAT3 Physics::AddXMFLOAT3(DirectX::XMFLOAT3 a, DirectX::XMFLOAT3 b) {
    DirectX::XMFLOAT3 result{0,0,0};

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

void Physics::trackDist(Physics::eResource* tModel) {
  for (auto& m : trackedModels) {
       auto reet = new Physics::eResource::tmCollide{ m };
       if(m != tModel)
       tModel->tmDist.emplace_back(reet);
  }
  tModel->distlock.lock();
  while(upDist)
      for (auto& m : tModel->tmDist) {
          m->mtx.lock();
          m->distance = fDistance(&tModel->mPos.position, &m->ptModel->mPos.position);
          if (m->distance <= (tModel->model->uData->collradius + m->ptModel->model->uData->collradius)) {
              m->Collision.store(true);
              XMStoreFloat4(&m->direction, XMQuaternionNormalize(XMVectorSet(tModel->mPos.position.x, tModel->mPos.position.y, tModel->mPos.position.z, 0) - XMVectorSet(m->ptModel->mPos.position.x, m->ptModel->mPos.position.y, m->ptModel->mPos.position.z, 0)));
          }
          else
              m->Collision.store(false);
          m->mtx.unlock();
      }
  tModel->distlock.unlock();
}

void Physics::pSpecCollison(eResource* obj) {
    obj->currentMtx.lock();
    std::vector<std::thread> threads;
    for (auto& m : obj->tmDist) {
        std::vector<std::thread> tempthread;
        if (m->Collision.load()) {
            m->mtx.lock();
            



            //Skeleton coll here?







            m->mtx.unlock();
        }
    }
    for (auto& t : threads) {
        t.join();
    }
    obj->currentMtx.unlock();
}

void Physics::RayCastColl(ReadX3D::pCollision* pos, XMFLOAT3* apos, Physics::eResource* obj, XMFLOAT4* dir) {
    for (auto& c : obj->model->uData->cdata) {
        XMVECTOR tri10 = XMVectorSet(c.verts[0].x + obj->mPos.position.x, c.verts[0].y + obj->mPos.position.y, c.verts[0].z + obj->mPos.position.z, 0);
        XMVECTOR tri11 = XMVectorSet(c.verts[1].x + obj->mPos.position.x, c.verts[1].y + obj->mPos.position.y, c.verts[1].z + obj->mPos.position.z, 0);
        XMVECTOR tri12 = XMVectorSet(c.verts[2].x + obj->mPos.position.x, c.verts[2].y + obj->mPos.position.y, c.verts[2].z + obj->mPos.position.z, 0);
        XMVECTOR position = XMVectorSet(pos->pos.x + apos->x, pos->pos.y + apos->y, pos->pos.z + apos->z, 0);
        float dist = c.radius + pos->radius;
        if (DirectX::TriangleTests::Intersects(position, XMLoadFloat4(dir), tri10, tri11, tri12, dist)) {
            auto pee = 0;
        }
    }
}

bool Physics::triCollide(ReadX3D::pCollision* tri, ReadX3D::pCollision* tri2, XMFLOAT3* tripos, XMFLOAT3* tri2pos) {

    XMVECTOR tri10 = XMVectorSet(tri->verts[0].x + tripos->x, tri->verts[0].y + tripos->y, tri->verts[0].z + tripos->z, 0);
    XMVECTOR tri11 = XMVectorSet(tri->verts[1].x + tripos->x, tri->verts[1].y + tripos->y, tri->verts[1].z + tripos->z, 0);
    XMVECTOR tri12 = XMVectorSet(tri->verts[2].x + tripos->x, tri->verts[2].y + tripos->y, tri->verts[2].z + tripos->z, 0);
    XMVECTOR tri20 = XMVectorSet(tri2->verts[0].x + tri2pos->x, tri2->verts[0].y + tri2pos->y, tri2->verts[0].z + tri2pos->z, 0);
    XMVECTOR tri21 = XMVectorSet(tri2->verts[1].x + tri2pos->x, tri2->verts[1].y + tri2pos->y, tri2->verts[1].z + tri2pos->z, 0);
    XMVECTOR tri22 = XMVectorSet(tri2->verts[2].x + tri2pos->x, tri2->verts[2].y + tri2pos->y, tri2->verts[2].z + tri2pos->z, 0);
    
    return DirectX::TriangleTests::Intersects(tri10,tri11,tri12,tri20,tri21,tri22);
}
void Physics::mMove(Physics::eResource* mUpdate) {
    
    mUpdate->speed += mUpdate->gravpull;
    XMFLOAT4 both = { 0,0,0,0 };
    XMStoreFloat4(&both, (XMLoadFloat4(&mUpdate->velDir) * mUpdate->speed) + (XMLoadFloat4(&mUpdate->grav) * mUpdate->gravpull));
    mUpdate->mPos.posMtx.lock();
    mUpdate->mPos.position = { mUpdate->mPos.position.x + both.x, mUpdate->mPos.position.y + both.y, mUpdate->mPos.position.z + both.z };
    mUpdate->mPos.posMtx.unlock();
    mUpdate->which = RStorage::BOTH;
}
Physics::~Physics() {
    upDist.store(false);
    for (auto& t : distanceThreads)
        t.join();
    for (auto& t : collisionThreads)
        t.join();

}
