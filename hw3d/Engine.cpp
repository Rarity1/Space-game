#include "Engine.h"

Engine::Engine(Graphics* gfx, Keyboard* kbd):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd),
    GConst(6.67430 * pow(10, -11))
{
}


void Engine::iLoad() {
    pGfx->~Graphics();
    pGfx->LoadPipeline();
    //begin model tracking. load a gd default world mf
    trackedModels.emplace_back(new eResource{ 0, "cube"});
    trackedModels.emplace_back(new eResource{ 1, "untitled" });
    //wrld is 1:50000
    trackedModels.emplace_back(new eResource{ 2, "wrld" , true});
    plModel = trackedModels[1];
    //end model tracking.
    for (auto& m : trackedModels) {
        m->loadedModel.model = new RStorage::bmResource{m->name};
        m->loadedModel.model->umID = m->umID;
        pGfx->loadModels(m->umID, m->loadedModel.model);
        auto tempc = 0;
        XMFLOAT3 temp1;
        XMFLOAT3 temp2;
        XMFLOAT3 temp3;
        XMFLOAT3 temp4 = {0,0,0};
        for (auto& v : m->loadedModel.model->uData->vertexData()) {
            switch (tempc%3) {
            case 0:
                temp1 = v.position;
                break;
            case 1:
                temp2 = v.position;
                break;
            case 2:
                temp3 = v.position;
                XMFLOAT3 tempcoll = { temp1.x + temp2.x + temp3.x / 3, temp1.y + temp2.y + temp3.y / 3, temp1.z + temp2.z + temp3.z / 3 };
                m->collision.emplace_back(pCollision{ tempcoll, fDistance(tempcoll, temp1) });
                temp4 = abs(temp3.x) > abs(temp4.x) && abs(temp3.y) > abs(temp4.y) && abs(temp3.z) > abs(temp4.z) ? temp3 : temp4;
                break;
            }

            m->gCollision = fDistance({0,0,0}, temp4);
            tempc++;
            
        }
        //testing
        if (m->umID == 1) {
            m->loadedModel.position = { 0,0,168 };
            m->mworld = trackedModels[2];
            m->mass = 200;
        }
        else if (m->umID == 2) {
            m->mass = 8570000000;
            m->mass *= 200000;
            m->loadedModel.position = { 0,0,0 };
        }
        else if (m->umID == 0) {
            m->loadedModel.position = { 0,10,138 };
            m->mworld = trackedModels[2];
            m->mass = 200;
        }
        m->loadedModel.lastposition = m->loadedModel.position;
        pGfx->SetModelPosition(&m->loadedModel);
    }
    pGfx->LoadResources();
    if (engInit) {
        engInit = false;
    }
}

void Engine::Update(float frametime)
{
    UControls();
    timer += frametime/1000;
    if (timer >= 1/updaterate) {
        
        cPlayermodel();
        eResource* tempproc = nullptr;
        for (auto& m : trackedModels) {
            
            
            cMPosUpdate(m, tempproc);
        }

        UCampos();
        timer = 0;
    }
    pGfx->OnUpdate();
}

void Engine::procGenCollision(eResource* m, eResource* tempres) {
        for (auto& m2 : trackedModels) {
            if (m != m2 && m != tempres) {
                tempres = m2;
                auto temp = m->gCollision;
                temp += m2->gCollision;
                auto fdtemp = fDistance(m->loadedModel.position, m2->loadedModel.position);
                
                if (fdtemp <= temp) {
                    
                    pSpecCollison(m, m2, temp, fdtemp);
                }
            }
        }
}
void Engine::pSpecCollison(eResource* obj1, eResource* obj2, float radialdist, float actualdist) {
    double changex = (obj2->loadedModel.position.x - obj1->loadedModel.position.x);
    double changey = (obj2->loadedModel.position.y - obj1->loadedModel.position.y);
    double changez = (obj2->loadedModel.position.z - obj1->loadedModel.position.z);
    auto mag = sqrt(pow(changex, 2) + pow(changey, 2) + pow(changez, 2));

    changex /= mag;
    changey /= mag;
    changez /= mag;

    auto mass1 = obj1->mass;
    auto mass2 = obj2->mass;

    auto masg = sqrt(pow(mass1, 2) + pow(mass2, 2));

    mass1 /= masg;
    mass2 /= masg;

    auto newdist = radialdist - actualdist;


    obj1->loadedModel.position.x -= mass2 * newdist * changex;
    obj1->loadedModel.position.y -= mass2 * newdist * changey;
    obj1->loadedModel.position.z -= mass2 * newdist * changez;
    obj2->loadedModel.position.x += mass1 * newdist * changex;
    obj2->loadedModel.position.y += mass1 * newdist * changex;
    obj2->loadedModel.position.z += mass1 * newdist * changex;
}


void Engine::cPlayermodel()
{
    auto movespeed = 20.0;
    Movement move;
    if (m_keysPressed.w) {
       move.forward += movespeed * timer;
   }if (m_keysPressed.s) {
       move.forward -= movespeed * timer;
   }if (m_keysPressed.a) {
       move.left += movespeed * timer;
   }if (m_keysPressed.d) {
       move.left -= movespeed * timer;
   }

   auto changex = move.forward * pGfx->curCamera.rotation.x;// (move.forward * cosf(pGfx->curCamera.rotation.pitch) * cosf(pGfx->curCamera.rotation.yaw)) + move.left * cosf(pGfx->curCamera.rotation.yaw + XM_PIDIV2);
   auto changey = move.forward * pGfx->curCamera.rotation.y;// move.forward* cosf(pGfx->curCamera.rotation.pitch)* sinf(pGfx->curCamera.rotation.yaw) + move.left * sinf(pGfx->curCamera.rotation.yaw + XM_PIDIV2);
   auto changez = move.forward * pGfx->curCamera.rotation.z;// move.forward* sinf(pGfx->curCamera.rotation.pitch);

   plModel->loadedModel.position.x += changex;
   plModel->loadedModel.position.y += changey;
   plModel->loadedModel.position.z += changez;
}

void Engine::cMPosUpdate(eResource* mUpdate, eResource* tempres){
    cGravity(mUpdate);
    if (mUpdate->speed != 0 || mUpdate->gravpull != 0) {
        mUpdate->loadedModel.lastposition = mUpdate->loadedModel.position;
        mMove(mUpdate);
    }
    procGenCollision(mUpdate, tempres);      
    pGfx->SetModelPosition(&mUpdate->loadedModel);
}

void Engine::mMove(eResource* mUpdate) {
    XMFLOAT4 vel = { mUpdate->velDir.x* mUpdate->speed ,mUpdate->velDir.y * mUpdate->speed, mUpdate->velDir.z* mUpdate->speed, mUpdate->velDir.w * mUpdate->speed };
    XMFLOAT4 gravm = { mUpdate->grav.x * mUpdate->gravpull,mUpdate->grav.y * mUpdate->gravpull,mUpdate->grav.z * mUpdate->gravpull,mUpdate->grav.w * mUpdate->gravpull };
    XMFLOAT4 both = { 0,0,0,0 };
    XMStoreFloat4(&both, XMLoadFloat4(&vel) + XMLoadFloat4(&gravm));
    
    
    mUpdate->loadedModel.position = { mUpdate->loadedModel.position.x + both.x, mUpdate->loadedModel.position.y + both.y, mUpdate->loadedModel.position.z + both.z };


    mUpdate->loadedModel.which = RStorage::BOTH;
}



void Engine::UCampos() {
    //Link the camera position here to whatever you want.
    pGfx->curCamera.position.x = plModel->loadedModel.position.x;
    pGfx->curCamera.position.y = plModel->loadedModel.position.y;
    pGfx->curCamera.position.z = plModel->loadedModel.position.z;


    float pitch = 0;
    float yaw = 0;
    if (m_keysPressed.up) {
        pitch += XM_PIDIV2 * timer;
    }
    if (m_keysPressed.down) {
        pitch += -XM_PIDIV2 * timer;
    }
    if (m_keysPressed.left) {
        yaw += XM_PIDIV2 * timer;
    }
    if (m_keysPressed.right) {
        yaw += -XM_PIDIV2 * timer;
    }


    if(pitch != 0 || yaw != 0 || m_keysPressed.k)
    RotateCam(pitch, yaw);
    
}

void Engine::RotateCam(float Pitch, float Yaw, float Roll) {
    if (m_keysPressed.k)
        XMStoreFloat4(&pGfx->curCamera.upDirection, XMQuaternionInverse(XMLoadFloat4(&plModel->grav)));
    auto updirect = XMLoadFloat4(&pGfx->curCamera.upDirection);
    auto lookdirect = XMLoadFloat4(&pGfx->curCamera.rotation);

    if (Yaw != 0) {
        auto temp = XMQuaternionRotationNormal(XMQuaternionConjugate(updirect), Yaw);
        auto qup = XMQuaternionMultiply(temp, lookdirect);
        lookdirect = XMQuaternionMultiply(qup, XMQuaternionConjugate(temp));
    }
    if (Pitch != 0) {
        auto temp = XMQuaternionRotationNormal(XMQuaternionMultiply(lookdirect, updirect), Pitch);
        auto qup = XMQuaternionMultiply(temp, updirect);


        updirect = XMQuaternionMultiply(qup, XMQuaternionConjugate((temp)));
        auto left = XMQuaternionMultiply(temp, lookdirect);
        lookdirect = XMQuaternionMultiply(left, XMQuaternionConjugate((temp)));
    }
    
    XMStoreFloat4(&pGfx->curCamera.rotation, lookdirect);
    XMStoreFloat4(&pGfx->curCamera.upDirection, updirect);
}


XMFLOAT3 Engine::rWorld(XMFLOAT3 pos1) {
	return  { cWorld.x+pos1.x,cWorld.y+pos1.y,cWorld.z+pos1.z};
}
XMFLOAT3 Engine::dWorld(XMFLOAT3 pos1) {
    return  { -cWorld.x + pos1.x,-cWorld.y + pos1.y,-cWorld.z + pos1.z };
}
XMFLOAT3 Engine::cnWorld(XMFLOAT3 pos1) {
    auto tworld = { cWorld.x - nWorld.x, cWorld.x - nWorld.x, cWorld.x - nWorld.x};
    return  { -cWorld.x + pos1.x,-cWorld.y + pos1.y,-cWorld.z + pos1.z };
}
void Engine::OnKeyDown(unsigned char key)
{
    switch (key)
    {
    case 'K':
        m_keysPressed.k = true;
        break;
    case 'W':
        m_keysPressed.w = true;
        break;
    case 'A':
        m_keysPressed.a = true;
        break;
    case 'S':
        m_keysPressed.s = true;
        break;
    case 'D':
        m_keysPressed.d = true;
        break;
    case VK_LEFT:
        m_keysPressed.left = true;
        break;
    case VK_RIGHT:
        m_keysPressed.right = true;
        break;
    case VK_UP:
        m_keysPressed.up = true;
        break;
    case VK_DOWN:
        m_keysPressed.down = true;
        break;
    }
}

void Engine::OnKeyUp(unsigned char key)
{
    switch (key)
    {
    case 'K':
        m_keysPressed.k = false;
        break;
    case 'W':
        m_keysPressed.w = false;
        break;
    case 'A':
        m_keysPressed.a = false;
        break;
    case 'S':
        m_keysPressed.s = false;
        break;
    case 'D':
        m_keysPressed.d = false;
        break;
    case VK_LEFT:
        m_keysPressed.left = false;
        break;
    case VK_RIGHT:
        m_keysPressed.right = false;
        break;
    case VK_UP:
        m_keysPressed.up = false;
        break;
    case VK_DOWN:
        m_keysPressed.down = false;
        break;
    }


}
float Engine::fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2) {
    auto x = pos2.x - pos1.x;
    auto y = pos2.y - pos1.y;
    auto z = pos2.z - pos1.z;
    return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
}

void Engine::cGravity(eResource* obj) {
    if (obj->mworld != nullptr) {
        float distance = -fDistance(obj->loadedModel.position, obj->mworld->loadedModel.position);


        XMFLOAT4 change{0,0,0,0};
         change.x = (obj->mworld->loadedModel.position.x - obj->loadedModel.position.x);
         change.y = (obj->mworld->loadedModel.position.y - obj->loadedModel.position.y);
         change.z = (obj->mworld->loadedModel.position.z - obj->loadedModel.position.z);


        XMStoreFloat4(&change, XMQuaternionNormalize(XMLoadFloat4(&change)));
        obj->grav = change;
        


        obj->gravpull = sqrt(((GConst * (obj->mworld->mass)) / pow(distance, 2))) * timer;
    }
}

void Engine::UControls() {
    while (auto ss = kbd->ReadKey()) {

        if (ss->IsRelease()) {
            OnKeyUp(ss->GetCode());
        }
        if (ss->IsPress()) {
            OnKeyDown(ss->GetCode());
        }

    }
}



Engine::~Engine() {
    for (auto& m : trackedModels) {
        delete m->loadedModel.model;
        delete m;
    }
}