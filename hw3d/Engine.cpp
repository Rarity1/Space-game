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
    //Move everything between this into a function in Graphics
    pGfx->umodel.lock();

    //begin model tracking. load a gd default world mf
    pGfx->lModels->initResource("cube", 1, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 10,0,138 });
    pGfx->lModels->initResource("untitled", 2, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 0,0,138 });
    pGfx->lModels->initResource("wrld", 4, 1, 8570000000.0 , 0.3, DirectX::XMFLOAT3{ 0,0,0 });
    //wrld is 1:50000

    //stress it out nerd
    for (auto i = 0; i < 1; i++) {
        float p = i * 10;
        pGfx->lModels->initResource("cube", 1, 1, 200, 0.3, DirectX::XMFLOAT3{ 10 + p,0,143 });
    }
    trackedModels[0]->mworld = trackedModels[2];
    trackedModels[1]->mworld = trackedModels[2];
    trackedModels[3]->mworld = trackedModels[2];


    plModel = trackedModels[1];
    pGfx->curCamera.position = plModel->mPos.position;
    
    //end model tracking.
    pGfx->loadModels(trackedModels, true);
    //If all models arent unique this is a waste of space
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

     using namespace DirectX;
     if (move.forward != 0) {
         float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
         float speedchange = move.forward * timer.time;
         DirectX::XMFLOAT4 scale = { 0,0,0,0 };
         auto scalar = DirectX::XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMLoadFloat4(&pGfx->curCamera.rotation) * (fabs(move.forward) / move.forward));
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
        pitch += DirectX::XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.down) {
        pitch += -DirectX::XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.left) {
        yaw += -DirectX::XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.right) {
        yaw += DirectX::XM_PIDIV2 * timer.time;
    }

    if (m_keysPressed.q) {
        roll += DirectX::XM_PIDIV2 * timer.time;
    }
    if (m_keysPressed.e) {
        roll += -DirectX::XM_PIDIV2 * timer.time;
    }


    if(pitch != 0 || yaw != 0 || roll != 0)
    RotateCam(pitch, yaw, roll);
    
}

void Engine::RotateCam(float Pitch, float Yaw, float Roll) {
    auto gravdirect = DirectX::XMQuaternionInverse(XMLoadFloat4(&plModel->grav));
    auto updirect = XMLoadFloat4(&pGfx->curCamera.upDirection);
    auto lookdirect = XMLoadFloat4(&pGfx->curCamera.rotation);
    

    if (Yaw != 0) {
        auto temp = DirectX::XMQuaternionRotationNormal(updirect, Yaw);
        auto left = DirectX::XMQuaternionMultiply(temp, lookdirect);
        
        lookdirect = DirectX::XMQuaternionMultiply(left, DirectX::XMQuaternionConjugate(temp));
        auto qup = DirectX::XMQuaternionMultiply(temp, updirect);
        updirect = DirectX::XMQuaternionMultiply(qup, DirectX::XMQuaternionConjugate(temp));
    }
    if (Roll != 0) {
        auto temp = DirectX::XMQuaternionRotationNormal((lookdirect), Roll);
        auto qup = DirectX::XMQuaternionMultiply(temp, updirect);
        updirect = DirectX::XMQuaternionMultiply(qup, DirectX::XMQuaternionConjugate(temp));
        auto left = DirectX::XMQuaternionMultiply(temp, lookdirect);
        lookdirect = DirectX::XMQuaternionMultiply(left, DirectX::XMQuaternionConjugate((temp)));
    }
    if (Pitch != 0) {
        auto temp = DirectX::XMQuaternionRotationNormal(DirectX::XMQuaternionMultiply(lookdirect, updirect), Pitch);
        auto qup = DirectX::XMQuaternionMultiply(temp, updirect);


        updirect = DirectX::XMQuaternionMultiply(qup, DirectX::XMQuaternionConjugate(temp));

        auto left = DirectX::XMQuaternionMultiply(temp, lookdirect);
        lookdirect = DirectX::XMQuaternionMultiply(left, DirectX::XMQuaternionConjugate((temp)));
    }
    
    
    DirectX::XMStoreFloat4(&pGfx->curCamera.rotation, lookdirect);
    DirectX::XMStoreFloat4(&pGfx->curCamera.upDirection, updirect);
}


DirectX::XMFLOAT3 Engine::rWorld(DirectX::XMFLOAT3 pos1) {
	return  { cWorld.x+pos1.x,cWorld.y+pos1.y,cWorld.z+pos1.z};
}
DirectX::XMFLOAT3 Engine::dWorld(DirectX::XMFLOAT3 pos1) {
    return  { -cWorld.x + pos1.x,-cWorld.y + pos1.y,-cWorld.z + pos1.z };
}
DirectX::XMFLOAT3 Engine::cnWorld(DirectX::XMFLOAT3 pos1) {
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