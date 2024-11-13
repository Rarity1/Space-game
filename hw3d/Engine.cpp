#include "Engine.h"

Engine::Engine(Graphics& gfx, Keyboard& kbd, EngineTime& clock):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd),
    trackedModels(gfx.lModels->initializedModels),
    Clock(clock)
{
    phyx = std::make_unique<Physics>(Clock, trackedModels, updaterate);
    pGfx.LoadPipeline();
}


void Engine::iLoad() {
    eventQueue.resize(updaterate);
    //Move everything between this into a function in Graphics
    pGfx.umodel.lock();

    //begin model tracking. load a gd default world mf
    pGfx.lModels->initResource("cube", 1, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 10,0,138 });
    pGfx.lModels->initResource("untitled", 2, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 0,0,138 });
    pGfx.lModels->initResource("wrld", 4, 1, 8570000000.0 , 0.3, DirectX::XMFLOAT3{ 0,0,0 });
    //wrld is 1:50000

    //stress it out nerd
    for (auto i = 0; i < 1; i++) {
        float p = i * 10;
        //pGfx.lModels->initResource("cube", 1, 1, 200, 0.3, DirectX::XMFLOAT3{ 10 + p,0,143 });
    }
    //trackedModels[0].mworld = &trackedModels[2];
    //trackedModels[1].mworld = &trackedModels[2];
    //trackedModels[3]->mworld = trackedModels[2];


    plModel = &trackedModels[1];
    
    //end model tracking.
    //If all models arent unique this is a waste of space
    pGfx.LoadResources(std::size(trackedModels));
    pGfx.umodel.unlock();

    if (engInit) {
        engInit = false;
    }
    eRun.store(true);
    for (auto& m : trackedModels) {
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
    //mAniUpdate();
}



 void Engine::cPlayermodel()
 {
     auto movespeed = 1.0;
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

     using namespace DirectX;
     if (move.forward != 0) {
         float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
         float speedchange = move.forward * 1.0 / updaterate;
         DirectX::XMFLOAT4 scale = { 0,0,0,0 };
         auto scalar = DirectX::XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move.forward) / move.forward));
         DirectX::XMStoreFloat4(&scale, scalar);
         scale.x = fabs(scale.x);
         float totalspeed = plModel->speed + fabs(speedchange);

         float speedscal = fabs(speedchange * (scale.x) - speedchange *(1 - scale.x)) / totalspeed;
         float inspeedscal = fabs(plModel->speed * (scale.x)) / totalspeed;
         // Figure this out
         XMFLOAT4 Temporarydir;
         DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move.forward)/move.forward) * fabs(speedscal) + XMLoadFloat4(&plModel->velDir) * inspeedscal));


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
    pGfx.umodel.lock();
    for (auto& m : trackedModels) {
        //Update if doing animation
            //pGfx.UpdateModel(m->model);
    }
    pGfx.umodel.unlock();
}




int Engine::eventBusSync()
{
    std::thread th([this] {
        if (evBusLock.try_lock()) {
            QueueTLock.lock();
            std::for_each(QueueThreads.begin(), QueueThreads.end(), [](auto& thread) {
                thread.join();
            });
            QueueThreads.resize(0);


            QueueLock.lock();
            for (auto& e : eventQueue) {
                if (e != nullptr) {
                    e->evnt();
                    delete e;
                    e = nullptr;
                }
            }
            queueCount = 0;
            QueueLock.unlock();
            QueueTLock.unlock();
            evBusLock.unlock();
        }
    });

    if (oldBusThread.get_id()._Get_underlying_id() != 0) {
        oldBusThread.join();
    }
    oldBusThread = move(th);

    return 0;
}

void Engine::queueCommand(std::function<void()> Function, int Priority)
{
    std::thread th([this, Function, Priority] {
        bool queued = false;
        int x = Priority;
        if (x >= updaterate) {
            x = 0;
        }
        while (!queued) {
            if (queueCount < updaterate) {
                QueueLock.lock();
                if (eventQueue[x] == nullptr) {
                    eventQueue[x] = new Event(Function);
                    queued = true;
                    queueCount++;
                }
                else {
                    x++;
                }
                QueueLock.unlock();
            }
            else return;
        }
    });
    QueueTLock.lock();
    QueueThreads.emplace_back(move(th));
    QueueTLock.unlock();

}

void Engine::enQueueEngineCommands()
{
    queueCommand(std::function<void()>([this] {this->cPlayermodel(); }), 0);
    queueCommand(std::function<void()>([this] {phyx->Update(); }), 20);
}

void Engine::enQueueExternCommands()
{
}

void Engine::UCampos() {


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

    //Please add a threadsafe way to update position
    if (pGfx.curCamera.position == nullptr) {
        plModel->mPos->posMtx.lock();
        pGfx.curCamera.position = plModel->mPos->position;
        pGfx.curCamera.posMtx = &plModel->mPos->posMtx;
        plModel->mPos->posMtx.unlock();
    }
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
void Engine::OnKeyDown(unsigned char key)
{
    switch (key)
    {
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


void Engine::SetModelPosition(RStorage::eResource* model) {

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
    if (oldBusThread.get_id()._Get_underlying_id() != 0) {
        oldBusThread.join();
    }
    for (auto& e : eventQueue) {

        if (e != nullptr)
            delete e;
    }
    eRun.store(false);
}