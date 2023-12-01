#include "Engine.h"

Engine::Engine(Graphics* gfx, Keyboard* kbd):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd)
{
}


void Engine::iLoad() {
    phyx = std::make_unique<Physics>(&timer, trackedModels, &updaterate);
    phyx->upDist.store(true);
    pGfx->~Graphics();
    pGfx->LoadPipeline();
    //begin model tracking. load a gd default world mf
    trackedModels.emplace_back(new Physics::eResource{ 0, "cube"});
    trackedModels.emplace_back(new Physics::eResource{ 1, "untitled" });
    //wrld is 1:50000
    trackedModels.emplace_back(new Physics::eResource{ 2, "wrld" , true});
    plModel = trackedModels[1];
    //end model tracking.
    for (auto& m : trackedModels) {
        m->loadedModel.model = new RStorage::bmResource{m->name};
        m->loadedModel.model->umID = m->umID;
        pGfx->loadModels(m->umID, m->loadedModel.model);
        //testing
        if (m->umID == 1) {
            m->loadedModel.position = { 0,0,168 };
            m->mworld = trackedModels[2];
            m->mass = 200;
        }
        else if (m->umID == 2) {
            m->mass = 8570000000;
            m->mass *= 2;
            m->loadedModel.position = { 0,0,0 };
        }
        else if (m->umID == 0) {
            m->loadedModel.position = { 0,20,168 };
            m->mworld = trackedModels[2];
            m->mass = 200;
        }
        m->loadedModel.lastposition = m->loadedModel.position;
        pGfx->SetModelPosition(&m->loadedModel);
    }
    for (auto& m : trackedModels) {
        for (auto& b : trackedModels) {
            if (b != m && ((m->mworld == b->mworld || b == m->mworld)|| b->isWorld)) {
                auto reet = new Physics::eResource::tmCollide{b};
                m->tmDist.emplace_back(reet);
            }
        }
    }
    pGfx->LoadResources();
    if (engInit) {
        engInit = false;
    }
}

std::thread Engine::Update(float frametime)
{
    std::thread th(&Engine::DoStuff, this, frametime);
    return th;
}

void Engine::DoStuff(float frametime) {
    UControls();
    timer.mtx.lock();
    timer.time += frametime / 1000;
    timer.mtx.unlock();
    if (timer.time >= 1.0f / updaterate) {
        cPlayermodel();
        cMPosUpdate();
        UCampos();
        timer.mtx.lock();
        timer.time = 0;
        timer.mtx.unlock();
    }
}


void Engine::cPlayermodel()
{
    auto movespeed = 1.0;
    Movement move;
    if (m_keysPressed.w) {
       move.forward += movespeed * timer.time;
   }if (m_keysPressed.s) {
       move.forward -= movespeed * timer.time;
   }if (m_keysPressed.a) {
       move.left += movespeed * timer.time;
   }if (m_keysPressed.d) {
       move.left -= movespeed * timer.time;
   }

   XMStoreFloat4(&plModel->velDir, XMQuaternionNormalize(XMLoadFloat4(&plModel->velDir) + XMLoadFloat4(&pGfx->curCamera.rotation)));
   plModel->speed += move.forward;
}

void Engine::cMPosUpdate(){
    phyx->Update();
    for (auto& m : trackedModels) {
        //if (m->updated.load()) {
            pGfx->SetModelPosition(&m->loadedModel);
            //m->updated.store(false);
        //}
        
    }
   
}




void Engine::UCampos() {
    //Link the camera position here to whatever you want.
    pGfx->curCamera.position.x = plModel->loadedModel.position.x;
    pGfx->curCamera.position.y = plModel->loadedModel.position.y;
    pGfx->curCamera.position.z = plModel->loadedModel.position.z;

    float pitch = 0;
    float yaw = 0;
    float roll = 0;
    if (m_keysPressed.up) {
        pitch += XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.down) {
        pitch += -XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.left) {
        yaw += -XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.right) {
        yaw += XM_PIDIV2 * timer.time;
    }

    if (m_keysPressed.q) {
        roll += XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.e) {
        roll += -XM_PIDIV2 * timer.time;
    }


    if(pitch != 0 || yaw != 0 || roll != 0)
    RotateCam(pitch, yaw, roll);
    
}

void Engine::RotateCam(float Pitch, float Yaw, float Roll) {
    auto gravdirect = XMQuaternionInverse(XMLoadFloat4(&plModel->grav));
    auto updirect = XMLoadFloat4(&pGfx->curCamera.upDirection);
    auto lookdirect = XMLoadFloat4(&pGfx->curCamera.rotation);
    

    if (Yaw != 0) {
        auto temp = XMQuaternionRotationNormal(updirect, Yaw);
        auto left = XMQuaternionMultiply(temp, lookdirect);
        
        lookdirect = XMQuaternionMultiply(left, XMQuaternionConjugate(temp));
        auto qup = XMQuaternionMultiply(temp, updirect);
        updirect = XMQuaternionMultiply(qup, XMQuaternionConjugate(temp));
    }
    if (Roll != 0) {
        auto temp = XMQuaternionRotationNormal((lookdirect), Roll);
        auto qup = XMQuaternionMultiply(temp, updirect);
        updirect = XMQuaternionMultiply(qup, XMQuaternionConjugate(temp));
        auto left = XMQuaternionMultiply(temp, lookdirect);
        lookdirect = XMQuaternionMultiply(left, XMQuaternionConjugate((temp)));
    }
    if (Pitch != 0) {
        auto temp = XMQuaternionRotationNormal(XMQuaternionMultiply(lookdirect, updirect), Pitch);
        auto qup = XMQuaternionMultiply(temp, updirect);


        updirect = XMQuaternionMultiply(qup, XMQuaternionConjugate(temp));

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
    case 'Q':
        m_keysPressed.q = true;
        break;
    case 'E':
        m_keysPressed.e = true;
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
    case 'Q':
        m_keysPressed.q = false;
        break;
    case 'E':
        m_keysPressed.e = false;
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
        
        for (auto& b : m->tmDist) {
            delete b;
        }
        delete m->loadedModel.model->uData;
        delete m->loadedModel.model;
        delete m;
    }

}