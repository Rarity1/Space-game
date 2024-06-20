#include "Engine.h"

Engine::Engine(Graphics* gfx, Keyboard* kbd):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd),
    trackedModels(gfx->lModels->initializedModels)
{
    phyx = std::make_unique<Physics>(timer, trackedModels, updaterate);
    pGfx->LoadPipeline();
}


void Engine::iLoad() {
    pGfx->umodel.lock();




    //begin model tracking. load a gd default world mf
    pGfx->lModels->initResource((char)"cube", 0, 1, 200.0, 0.01, XMFLOAT3{ 10,0,138 });
    pGfx->lModels->initResource((char)"untitled1", 1, 1, 200.0, 0.01, XMFLOAT3{ 0,0,138 });
    pGfx->lModels->initResource((char)"wrld", 2, 1, 8570000000.0 * 200, 0.3, XMFLOAT3{ 0,0,0 });
    //wrld is 1:50000

    //stress it out nerd
    for (auto i = 0; i < 1; i++) {
        float p = i * 10;
        pGfx->lModels->initResource((char)"b", 0, 1, 200, 0.3, XMFLOAT3{ 15 + p,0,138 });
    }
    trackedModels[0]->mworld = trackedModels[2];
    trackedModels[1]->mworld = trackedModels[2];

    plModel = trackedModels[1];
    pGfx->curCamera.position = plModel->mPos.position;
    
    //end model tracking.
    pGfx->loadModels(trackedModels, true);
    pGfx->LoadResources(std::size(trackedModels));
    pGfx->umodel.unlock();

    if (engInit) {
        engInit = false;
    }
    eRun.store(true);
    for (auto& m : trackedModels) {
        pGfx->UpdateModel(m);
    }
}

void Engine::Update()
{

    UControls();
    timer.mtx.lock();
    culmtime += timer.time;
    timer.mtx.unlock();

    if (culmtime >= 1.0f / updaterate) {
        cPlayermodel();
        phyx->Update();
        culmtime = 0;
    }
    UCampos();
    mAniUpdate();
}



 void Engine::cPlayermodel()
 {
     auto movespeed = 1.0;
     Movement move;
     if (m_keysPressed.w) {
         move.forward += movespeed;
     }if (m_keysPressed.s) {
         move.forward -= movespeed;
     }if (m_keysPressed.a) {
         move.left += movespeed;
     }if (m_keysPressed.d) {
         move.left -= movespeed;
     }

     if (move.forward != 0) {
         float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
         float speedchange = move.forward * timer.time;
         XMFLOAT4 scale = { 0,0,0,0 };
         auto scalar = XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMLoadFloat4(&pGfx->curCamera.rotation) * (fabs(move.forward) / move.forward));
         DirectX::XMStoreFloat4(&scale, scalar);
         scale.x = fabs(scale.x);
         float totalspeed = plModel->speed + fabs(speedchange);

         float speedscal = fabs(speedchange * (scale.x) - speedchange *(1 - scale.x)) / totalspeed;
         float inspeedscal = fabs(plModel->speed * (scale.x)) / totalspeed;
         // Figure this out
         XMFLOAT4 Temporarydir;
         DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx->curCamera.rotation) * (fabs(move.forward)/move.forward) * fabs(speedscal) + XMLoadFloat4(&plModel->velDir) * inspeedscal));


         if (scale.y < 0) {
             plModel->speed = fabs(plModel->speed - fabs(speedchange));
         }
         else {
             plModel->speed = plModel->speed + fabs(speedchange);

         }
     
         plModel->velDir = Temporarydir;
    }
 }

void Engine::mAniUpdate(){
    pGfx->umodel.lock();
    for (auto& m : trackedModels) {
        //Update if doing animation
            //pGfx->UpdateModel(m->model);
    }
    pGfx->umodel.unlock();
}




void Engine::UCampos() {


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
    
    
    DirectX::XMStoreFloat4(&pGfx->curCamera.rotation, lookdirect);
    DirectX::XMStoreFloat4(&pGfx->curCamera.upDirection, updirect);
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


void Engine::SetModelPosition(RStorage::eResource* model) {

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
    eRun.store(false);
    phyx.reset();


}