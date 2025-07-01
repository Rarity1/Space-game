#include "Engine.h"

Engine::Engine(Graphics& gfx, Keyboard& kbd, EngineTime& clock):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd),
    trackedObjects(*gfx.lModels->trackedObjects),
    Clock(clock)
{
    phyx = std::make_unique<Physics>(Clock, trackedObjects, updaterate);
    threads = std::make_unique<THREADS>((int)std::thread::hardware_concurrency());
    pGfx.LoadPipeline();
}


void Engine::iLoad() {
    //Move everything between this into a function in Graphics
    pGfx.umodel.lock();

    //begin model tracking. load a gd default world mf
    pGfx.lModels->initObject("untitled", 2, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 0,0,138 });
    pGfx.lModels->initObject("cube", 1, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 10,0,138 });
    pGfx.lModels->initObject("wrld", 4, 1, 8570000000.0 , 0.3, DirectX::XMFLOAT3{ 0,0,0 });
    //wrld is 1:50000

    for (auto i = 0; i < 1; i++) {
        float p = i * 1;
        pGfx.lModels->initObject("untitled", 2, 1, 200, 0.3, DirectX::XMFLOAT3{ 12 + p,0,138 });
    }

    for (auto i = 0; i < 15; i++) {
        float p = i * 1;
       //pGfx.lModels->initObject("untitled", 1, 1, 200, 0.3, DirectX::XMFLOAT3{ 12 + p,0,138 });
    }

    //trackedModels[0].mworld = &trackedModels[2];
    //trackedModels[1].mworld = &trackedModels[2];
    //trackedModels[3]->mworld = trackedModels[2];

    //Make a better way of setting player model. 
    plModel = &trackedObjects[0];
    
    //end model tracking. begin resource upload.
    pGfx.LoadResources();
    pGfx.umodel.unlock();

    if (engInit) {
        engInit = false;
    }
    eRun.store(true);
    for (auto& m : trackedObjects) {
        pGfx.UpdateModel(&m);
    }
    phyx->trackM();
}

void Engine::Update(double delta)
{
    UControls();
    UCampos();
    lastD += delta;
    if ((1.0 / updaterate) <= lastD) {
        enQueueEngineCommands();
        enQueueExternCommands();
        eventBusSync();
        lastD = 0.0;
    }
    mAniUpdate();
}



 void Engine::cPlayermodel()
 {
     auto movespeed = 1.0;
     bool movestop = false;
     Movement move;
     if (m_keysPressed.FindBuffered(KeysPressed::W)) {
         move.forward += movespeed;
     }if (m_keysPressed.FindBuffered(KeysPressed::S)) {
         move.forward -= movespeed;
     }if (m_keysPressed.FindBuffered(KeysPressed::A)) {
         move.left += movespeed;
     }if (m_keysPressed.FindBuffered(KeysPressed::D)) {
         move.left -= movespeed;
     }
     if (m_keysPressed.FindBuffered(KeysPressed::K)) {
         movestop = true;
     }
     using namespace DirectX;

     if (freeCamTGL.load()) {
         if (move.forward != 0) {
             float speedchange = move.forward * 10 / updaterate;
             // Figure this out
             XMFLOAT4 Temporarydir;
             DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx.curCamera.rotation) * fabs(speedchange)));


             XMStoreFloat3(&freeCamPos, XMLoadFloat4(&Temporarydir) * speedchange + XMLoadFloat3(&freeCamPos));
         }
         if (movestop) {
             plModel->velDir = { 0,0,0,0 };
             plModel->speed = 0;
         }
     }
     else {
         if (move.forward != 0 && !movestop) {
             float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
             float speedchange = move.forward * 1.0 / updaterate;
             DirectX::XMFLOAT4 scale = { 0,0,0,0 };
             auto scalar = DirectX::XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move.forward) / move.forward));
             DirectX::XMStoreFloat4(&scale, scalar);
             scale.x = fabs(scale.x);
             float totalspeed = plModel->speed + fabs(speedchange);

             float speedscal = fabs(speedchange * (scale.x) - speedchange * (1 - scale.x)) / totalspeed;
             float inspeedscal = fabs(plModel->speed * (scale.x)) / totalspeed;
             // Figure this out
             XMFLOAT4 Temporarydir;
             DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move.forward) / move.forward) * fabs(speedscal) + XMLoadFloat4(&plModel->velDir) * inspeedscal));


             if (scale.y < 0) {
                 plModel->speed = fabs(plModel->speed - fabs(speedchange));
             }
             else {
                 plModel->speed = plModel->speed + fabs(speedchange);

             }

             plModel->velDir = Temporarydir;
         }
         else if (movestop) {
             plModel->velDir = { 0,0,0,0 };
             plModel->speed = 0;
         }
     }



 }

void Engine::mAniUpdate(){
    using namespace DirectX;
    pGfx.umodel.lock();

    /*
        if (m_keysPressed.FindBuffered(KeysPressed::K)) {

        XMFLOAT4 up(1,0,0, 0);
        auto temp = DirectX::XMQuaternionRotationAxis(XMVector4Normalize(XMLoadFloat4(&up)), 10 * Clock.Current());
        auto left = DirectX::XMQuaternionMultiply(temp, XMLoadFloat4(&trackedObjects[1].mPos.rotation));


        XMStoreFloat4(&trackedObjects[1].mPos.rotation, left);


    }
    */


    pGfx.umodel.unlock();
}




int Engine::eventBusSync()
{
    threads.get()->gEndWork(eWref);
    return 0;
}

//Max priority is 65535
void Engine::queueCommand(std::function<void()> Function, int Priority)
{
    if (QueueThreads[Priority].inUse) {
        queueCommand(Function, Priority+1);
    }
    else {
        QueueThreads[Priority] = Event(Priority, Function, true);
    }


}

void Engine::enQueueEngineCommands()
{
    queueCommand(std::function<void()>([this] {this->cPlayermodel(); }), 0);
    //this->cPlayermodel();
    queueCommand(std::function<void()>([this] {phyx->Update(); }), 20);
    //phyx->Update();
    queueCommand(std::function<void()>([this] { std::for_each(trackedObjects.begin(), trackedObjects.end(), [this](auto& e) { sPGraphics(e); }); }), 21);

    std::vector<std::function<void()>> wFs;

    for (auto& q : QueueThreads) {
        wFs.emplace_back(q.second.wFunc);
        q.second.inUse = false;
    }

    eWref = threads.get()->gPushWork(wFs);
}

void Engine::enQueueExternCommands()
{
}

void Engine::UCampos() {
    //Please add a threadsafe way to update position
    if (pGfx.curCamera.position == nullptr) {
        plModel->mPos.posMtx.lock();
        pGfx.curCamera.position = plModel->mPos.position;
        pGfx.curCamera.posMtx = &plModel->mPos.posMtx;
        plModel->mPos.posMtx.unlock();
    }

    //Toggle Freecam
    if (m_keysPressed.FindBuffered(KeysPressed::J) && inputDelay.Peek() > 0.5) {
        inputDelay.Mark();
        if (freeCamTGL.load()) {
            freeCamTGL.store(false);
            plModel->mPos.posMtx.lock();
            pGfx.curCamera.position = plModel->mPos.position;
            pGfx.curCamera.posMtx = &plModel->mPos.posMtx;
            plModel->mPos.posMtx.unlock();
        }
        else {
            freeCamTGL.store(true);
            plModel->mPos.posMtx.lock();

            freeCamPos = *plModel->mPos.position;
            plModel->mPos.posMtx.unlock();

            pGfx.curCamera.position = &freeCamPos;
            pGfx.curCamera.posMtx = &freeCamMTX;
        }
    }

    double delta = Clock.Current();
    float pitch = 0;
    float yaw = 0;
    float roll = 0;
    if (m_keysPressed.FindBuffered(KeysPressed::up)) {
        pitch += DirectX::XM_PIDIV2 * delta;
    }
    if (m_keysPressed.FindBuffered(KeysPressed::down)) {
        pitch += -DirectX::XM_PIDIV2 * delta;
    }
    if (m_keysPressed.FindBuffered(KeysPressed::left)) {
        yaw += -DirectX::XM_PIDIV2 * delta;
    }
    if (m_keysPressed.FindBuffered(KeysPressed::right)) {
        yaw += DirectX::XM_PIDIV2 * delta;
    }

    if (m_keysPressed.FindBuffered(KeysPressed::Q)) {
        roll += DirectX::XM_PIDIV2 * delta;
    }
    if (m_keysPressed.FindBuffered(KeysPressed::E)) {
        roll += -DirectX::XM_PIDIV2 * delta;
    }

    if(pitch != 0 || yaw != 0 || roll != 0)
    RotateCam(pitch, yaw, roll);


}

void Engine::sPGraphics(RStorage::eResource& model)
{
    pGfx.umodel.lock();


    using namespace DirectX;
    model.mPos.posMtx.lock();
    model.mPos.lastposition = *model.mPos.position;
    auto pDir = model.CollDir();
    XMStoreFloat3(model.mPos.position, XMLoadFloat3(model.mPos.position) + XMLoadFloat4(&pDir));
    model.CollReset();
    XMStoreFloat3(model.mPos.position, XMLoadFloat3(model.mPos.position) + XMLoadFloat4(&model.velDir) * (model.speed));
    model.mPos.posMtx.unlock();


    XMStoreFloat4x4(&model.cmatrix, XMMatrixRotationQuaternion(XMLoadFloat4(&model.mPos.rotation)) * XMMatrixTranslation(model.mPos.position->x, model.mPos.position->y, model.mPos.position->z));
    pGfx.umodel.unlock();

}

void Engine::RotateCam(float Pitch, float Yaw, float Roll) {
    auto gravdirect = DirectX::XMQuaternionInverse(XMLoadFloat4(&plModel->grav));
    auto updirect = XMLoadFloat4(&pGfx.curCamera.upDirection);
    auto lookdirect = XMLoadFloat4(&pGfx.curCamera.rotation);
    

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
    
    
    DirectX::XMStoreFloat4(&pGfx.curCamera.rotation, lookdirect);
    DirectX::XMStoreFloat4(&pGfx.curCamera.upDirection, updirect);
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


//Rewrite this to accept a keybinding config. eg KeyPressed::Action instead of KeyPressed::Key
void Engine::OnKeyDown(unsigned char key)
{
    switch (key)
    {
    case 'J':
        m_keysPressed.Set(KeysPressed::J);
        break;
    case 'K':
        m_keysPressed.Set(KeysPressed::K);
        break;
    case 'W':
        m_keysPressed.Set(KeysPressed::W);
        break;
    case 'A':
        m_keysPressed.Set(KeysPressed::A);
        break;
    case 'S':
        m_keysPressed.Set(KeysPressed::S);
        break;
    case 'D':
        m_keysPressed.Set(KeysPressed::D);
        break;
    case 'Q':
        m_keysPressed.Set(KeysPressed::Q);
        break;
    case 'E':
        m_keysPressed.Set(KeysPressed::E);
        break;
    case VK_LEFT:
        m_keysPressed.Set(KeysPressed::left);
        break;
    case VK_RIGHT:
        m_keysPressed.Set(KeysPressed::right);
        break;
    case VK_UP:
        m_keysPressed.Set(KeysPressed::up);
        break;
    case VK_DOWN:
        m_keysPressed.Set(KeysPressed::down);
        break;
    }
}

void Engine::OnKeyUp(unsigned char key)
{
    switch (key)
    {
    case 'J':
        m_keysPressed.Set(KeysPressed::J, false);
        break;
    case 'K':
        m_keysPressed.Set(KeysPressed::K, false);
        break;
    case 'W':
        m_keysPressed.Set(KeysPressed::W, false);
        break;
    case 'A':
        m_keysPressed.Set(KeysPressed::A, false);
        break;
    case 'S':
        m_keysPressed.Set(KeysPressed::S, false);
        break;
    case 'D':
        m_keysPressed.Set(KeysPressed::D, false);
        break;
    case 'Q':
        m_keysPressed.Set(KeysPressed::Q, false);
        break;
    case 'E':
        m_keysPressed.Set(KeysPressed::E, false);
        break;
    case VK_LEFT:
        m_keysPressed.Set(KeysPressed::left, false);
        break;
    case VK_RIGHT:
        m_keysPressed.Set(KeysPressed::right, false);
        break;
    case VK_UP:
        m_keysPressed.Set(KeysPressed::up, false);
        break;
    case VK_DOWN:
        m_keysPressed.Set(KeysPressed::down, false);
        break;
    }
}


void Engine::UControls() {
    while (auto ss = kbd.ReadKey()) {

        if (ss->IsRelease()) {
            OnKeyUp(ss->GetCode());
        }
        if (ss->IsPress()) {
            OnKeyDown(ss->GetCode());
        }
    }

}



Engine::~Engine() {
    //_ASSERT(eWref.uWid != 0);
    //threads.get()->gEndWork(eWref);

    eRun.store(false);
}