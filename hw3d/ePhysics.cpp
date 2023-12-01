#include "ePhysics.h"



Physics::Physics(mThreadTime* timer, std::vector<Physics::eResource*>& trackedModels, int* UpdateRate) :
    GConst(6.67430 * pow(10, -11)),
    timer(timer),
    trackedModels(trackedModels),
    urate(UpdateRate)
{
}

void Physics::Update() {
    std::vector<std::thread> collthread;
    for (auto& mUpdate : trackedModels) {
        std::thread th(&Physics::trackDist, this, std::ref(mUpdate));
        th.detach();
        
        if (mUpdate->speed != 0 || mUpdate->gravpull != 0 || mUpdate->updated.load()) {
            mUpdate->loadedModel.lastposition = mUpdate->loadedModel.position;
            mMove(mUpdate);
        }
        collthread.emplace_back(std::thread(&Physics::pSpecCollison, this, mUpdate));
    }
    for (auto& t : collthread) {
        t.join();
    }
}

void Physics::cGravity(eResource* obj) {
    if (obj->mworld != nullptr) {
        float distance = -fDistance(&obj->loadedModel.position, &obj->mworld->loadedModel.position);


        XMFLOAT4 change{ 0,0,0,0 };
        change.x = (obj->mworld->loadedModel.position.x - obj->loadedModel.position.x);
        change.y = (obj->mworld->loadedModel.position.y - obj->loadedModel.position.y);
        change.z = (obj->mworld->loadedModel.position.z - obj->loadedModel.position.z);


        XMStoreFloat4(&change, XMQuaternionNormalize(XMLoadFloat4(&change)));
        obj->grav = change;


        timer->mtx.lock();
        obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(distance, 2))) * timer->time;
        timer->mtx.unlock();
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
  for (auto& m : tModel->tmDist) {
                m->mtx.lock();
                m->distance = fDistance(&tModel->loadedModel.position, &m->ptModel->loadedModel.position);
                if (m->distance <= (tModel->loadedModel.model->uData->collradius + m->ptModel->loadedModel.model->uData->collradius)) {
                    m->Collision.store(true);
                    XMStoreFloat4(&m->direction, XMQuaternionNormalize(XMVectorSet(tModel->loadedModel.position.x, tModel->loadedModel.position.y, tModel->loadedModel.position.z, 0)-XMVectorSet(m->ptModel->loadedModel.position.x, m->ptModel->loadedModel.position.y, m->ptModel->loadedModel.position.z, 0)));
                }
                else
                    m->Collision.store(false);
                m->mtx.unlock();
  }
}

void Physics::pSpecCollison(eResource* obj) {
    obj->currentMtx.lock();
    
    for (auto& m : obj->tmDist) {
        std::vector<std::thread> tempthread;
        if (m->Collision.load()) {
            m->mtx.lock();
            for (auto& c : obj->loadedModel.model->uData->cdata) {
                std::thread th(&Physics::RayCastColl, this, &c, &obj->loadedModel.position, m->ptModel, &m->direction);
                th.detach();
            }








            m->mtx.unlock();
        }
    }

    obj->currentMtx.unlock();
}

void Physics::RayCastColl(ReadX3D::pCollision* pos, XMFLOAT3* apos, Physics::eResource* obj, XMFLOAT4* dir) {
    for (auto& c : obj->loadedModel.model->uData->cdata) {
        XMVECTOR tri10 = XMVectorSet(c.verts[0].x + obj->loadedModel.position.x, c.verts[0].y + obj->loadedModel.position.y, c.verts[0].z + obj->loadedModel.position.z, 0);
        XMVECTOR tri11 = XMVectorSet(c.verts[1].x + obj->loadedModel.position.x, c.verts[1].y + obj->loadedModel.position.y, c.verts[1].z + obj->loadedModel.position.z, 0);
        XMVECTOR tri12 = XMVectorSet(c.verts[2].x + obj->loadedModel.position.x, c.verts[2].y + obj->loadedModel.position.y, c.verts[2].z + obj->loadedModel.position.z, 0);
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
    mUpdate->loadedModel.position = { mUpdate->loadedModel.position.x + both.x, mUpdate->loadedModel.position.y + both.y, mUpdate->loadedModel.position.z + both.z };
    mUpdate->loadedModel.which = RStorage::BOTH;
}
Physics::~Physics() {
    upDist.store(false);
    for (auto& t : collisionThreads)
        t.join();
}
