#include "Engine.h"

Engine::Engine(Graphics& gfx, Keyboard& kbd, EngineTime& clock):
    Clock(clock),
	  pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd)

{
    
    phyx = std::make_unique<Physics>(updaterate);
    tracker = std::make_unique<Tracker>();
    tMain = std::make_unique<THREADS>((int)std::thread::hardware_concurrency());
    eRun.store(true);
    LoopThread = std::move(std::thread([this] {EngineLoop(); }));

}




//Initial load of the engine. 
void Engine::iLoad() {

    pGfx.LoadPipeline();
    //begin model tracking. load a gd default world mf
    //Player model needs to be set.
    //Make a better way of setting player model. 
    cTrackedInstance = 1;
    //This needs to be processed every frame. EG distance from player or special cases
    renderedInstances.emplace_back(std::pair{cTrackedInstance, &tracker->initInstance(cTrackedInstance)});
    processedInstances = renderedInstances;
    plModel = tracker->initObject(tracker->getInstance(cTrackedInstance), "untitled", 2, 1, 2000.0, 0.01, DirectX::XMFLOAT3{ 0,0,138 });
    tracker->initObject(tracker->getInstance(cTrackedInstance), "cube", 1, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 10,0,138 });
    tracker->initObject(tracker->getInstance(cTrackedInstance), "wrld", 4, 1, 8570000000.0 , 0.3, DirectX::XMFLOAT3{ 0,0,0 });
    //wrld is 1:50000

    for (auto i = 0; i < 20; i++) {
        float p = i * 2;
        tracker->initObject(tracker->getInstance(cTrackedInstance), "untitled", 2, 1, 200, 0.3, DirectX::XMFLOAT3{ 12 + p,0,138 });
    }

    for (auto i = 0; i < 10; i++) {
        float p = i * 2;
        tracker->initObject(tracker->getInstance(cTrackedInstance), "cube", 1, 1, 200, 0.3, DirectX::XMFLOAT3{ 12 + p,0,138 });
    }

    //trackedModels[0].mworld = &trackedModels[2];
    //trackedModels[1].mworld = &trackedModels[2];
    //trackedModels[3]->mworld = trackedModels[2];




    //end model tracking. begin resource upload.    
   
    loopMutex.lock();
    pauseLoop = true;
    loopMutex.unlock();

    auto& instance = tracker->getInstance(cTrackedInstance);
    std::for_each(instance.tmodelLinkedObjects.begin(), instance.tmodelLinkedObjects.end(), [this](auto& e) {
        for (auto& o : e.second) {
            tracker->lModel(*o, e.first);
        }
    });
    phyx->trackM(tracker->getInstance(cTrackedInstance));

    pGfx.LoadResources(tracker->getInstance(cTrackedInstance));

    loopMutex.lock();
    pauseLoop = false;
    loopMutex.unlock();
}

void Engine::EngineLoop() {
    loopLock = std::unique_lock<std::mutex>(loopMutex);
    loopLock.unlock();

    const double urate = 1.0 / updaterate;
    double last = 0;
    while (eRun.load()) {
        loopLock.lock();
        loopVariable.wait(loopLock, [this, &urate, &last] {
            if (Clock.Peek() > urate - last) {
                return true;
            }
            else if(!pauseLoop) {
                std::this_thread::sleep_for(std::chrono::milliseconds((int)((urate - (Clock.Peek() + last))*1000)));
                return true;
            }
            else {
                return false;
            }

        });
        loopLock.unlock();
        last = Clock.Mark();
        last = last >= urate ? last - urate : 0;
        enQueueEngineCommands();
        enQueueExternCommands();
        eventBusSync();

    }
}

bool Engine::Update()
{
    UControls();
    ucontrolClock.Mark();
    loopVariable.notify_all();
    mAniUpdate();
    //Update rendered instances every frame
    pGfx.Update(renderedInstances);
    pGfx.RenderFrame(tracker->getInstance(cTrackedInstance));
    return true;
}


//Calculate instances based on playermodel position ?
 void Engine::cPlayermodel()
 {
     std::unique_ptr<Movement> move(std::make_unique<Movement>());

     {
         auto movespeed = 2.0;
         if (m_keysPressed.FindBuffered(KeysPressed::W)) {
             move->forward += movespeed;
         }if (m_keysPressed.FindBuffered(KeysPressed::S)) {
             move->forward -= movespeed;
         }if (m_keysPressed.FindBuffered(KeysPressed::A)) {
             move->left += movespeed;
         }if (m_keysPressed.FindBuffered(KeysPressed::D)) {
             move->left -= movespeed;
         }
         if (m_keysPressed.FindBuffered(KeysPressed::K)) {
             move->movestop = true;
         }
     }



     if (freeCamTGL.load()) {
         if (move->forward != 0 || move->left != 0) {
             // Figure this out
             using namespace DirectX;

             XMStoreFloat3(&freeCamPos, XMLoadFloat4(&pGfx.curCamera.rotation) * (float)(move->forward * updateClock.Peek()) + XMLoadFloat3(&freeCamPos));
             XMStoreFloat3(&freeCamPos, XMVector3Transform(XMLoadFloat4(&pGfx.curCamera.rotation), XMMatrixRotationAxis(XMLoadFloat4(&pGfx.curCamera.upDirection), XMConvertToRadians(90.0f))) * (float)(move->left * updateClock.Peek()) + XMLoadFloat3(&freeCamPos));

         }
         if (move->movestop) {
             plModel->velDir = { 0,0,0,0 };
             plModel->speed = 0;
         }
     }
     else {
         if (move->forward != 0 && !move->movestop) {
             float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
             DirectX::XMFLOAT4 forwardScale = { 0,0,0,0 };

             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&forwardScale, DirectX::XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move->forward) / move->forward)));
             }
             forwardScale.x = fabs(forwardScale.x);

             auto forwardSpeed = move->forward * updateClock.Peek() * 0.1;
             float totalForwardSpeed = plModel->speed + fabs(forwardSpeed);
             float forwardSpeedScalar = fabs(forwardSpeed * (forwardScale.x) - forwardSpeed * (1 - forwardScale.x)) / totalForwardSpeed;
             float invertedForwardSpeedScalar = fabs(plModel->speed * (forwardScale.x)) / totalForwardSpeed;
             if (forwardScale.y < 0) {
                 plModel->speed = fabs(plModel->speed - fabs(forwardSpeed));
             }
             else {
                 plModel->speed = plModel->speed + fabs(forwardSpeed);

             }

             // Figure this out
             DirectX::XMFLOAT4 Temporarydir;
             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move->forward) / move->forward) * fabs(forwardSpeedScalar) + XMLoadFloat4(&plModel->velDir) * invertedForwardSpeedScalar));

             }




             plModel->velDir = Temporarydir;
         }
         if (move->left != 0 && !move->movestop) {
             float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
             DirectX::XMFLOAT4 leftScale = { 0,0,0,0 };

             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&leftScale, DirectX::XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMVector3Transform(XMLoadFloat4(&pGfx.curCamera.rotation), XMMatrixRotationAxis(XMLoadFloat4(&pGfx.curCamera.upDirection), XMConvertToRadians(90.0f))) * (fabs(move->left) / move->left)));
             }
             leftScale.x = fabs(leftScale.x);

             auto leftSpeed = move->left * updateClock.Peek() * 0.1;
             float totallefftSpeed = plModel->speed + fabs(leftSpeed);
             float leftSpeedScalar = fabs(leftSpeed * (leftScale.x) - leftSpeed * (1 - leftScale.x)) / totallefftSpeed;
             float invertedleftSpeedScalar = fabs(plModel->speed * (leftScale.x)) / totallefftSpeed;
             if (leftScale.y < 0) {
                 plModel->speed = fabs(plModel->speed - fabs(leftSpeed));
             }
             else {
                 plModel->speed = plModel->speed + fabs(leftSpeed);
             }
             // Figure this out
             DirectX::XMFLOAT4 Temporarydir;
             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMVector3Transform(XMLoadFloat4(&pGfx.curCamera.rotation), XMMatrixRotationAxis(XMLoadFloat4(&pGfx.curCamera.upDirection), XMConvertToRadians(90.0f))) * (fabs(move->left) / move->left) * fabs(leftSpeedScalar) + XMLoadFloat4(&plModel->velDir) * invertedleftSpeedScalar));

             }




             plModel->velDir = Temporarydir;
         }


         
         if (move->movestop) {
             plModel->velDir = { 0,0,0,0 };
             plModel->speed = 0;
         }
     }



 }

void Engine::mAniUpdate(){
    using namespace DirectX;

    /*
        if (m_keysPressed.FindBuffered(KeysPressed::K)) {

        XMFLOAT4 up(1,0,0, 0);
        auto temp = DirectX::XMQuaternionRotationAxis(XMVector4Normalize(XMLoadFloat4(&up)), 10 * Clock.Current());
        auto left = DirectX::XMQuaternionMultiply(temp, XMLoadFloat4(&trackedObjects[1].mPos.rotation));


        XMStoreFloat4(&trackedObjects[1].mPos.rotation, left);


    }
    */

}

void Engine::uPosInstances()
{
    for (auto& Instance : processedInstances) {
        std::for_each(Instance.second->tmodelLinkedObjects.begin(), Instance.second->tmodelLinkedObjects.end(), [this](auto& e) { for (auto& tObjects : e.second) { tObjects->UpdatePosition(); }});
    }
}

void Engine::uPhysics()
{

    phyx->Update(tracker->getInstance(cTrackedInstance));

}




int Engine::eventBusSync()
{
    tMain->gEndWork(eWref);
    updateClock.Mark();

    return 0;
}

//Max priority is 65535
void Engine::queueCommand(std::function<void()> Function, unsigned short Priority)
{
    QueueList.emplace_back(Event(Priority, Function));
    ++queueCount;
}

std::vector<std::function<void()>>& Engine::getCQueue()
{
    QueueList.sort([](auto& first, auto& second) {
        return first.wPriority > second.wPriority;
    });
    wFunctions.resize(0);
    for (auto& q : QueueList) {
        wFunctions.emplace_back(std::move(q.wFunc));
    }
    QueueList.resize(0);
    return wFunctions;
}

void Engine::enQueueEngineCommands()
{
    queueCommand(std::move([this] {
        cPlayermodel();
    }), 0);
    queueCommand(std::move([this] {
        uPhysics();
    }), 20);
    queueCommand(std::move([this] {
        uPosInstances();
        }), 21);
    //queueCommand([this] {pGfx.Update(tracker->trackedObjects);}, 65535);
    eWref = tMain->gPushWork(getCQueue());
}

void Engine::enQueueExternCommands()
{
}

void Engine::UCampos() {
    //Please add a threadsafe way to update position
    if (pGfx.curCamera.position == nullptr) {
        plModel->mPos.posMtx.lock();
        pGfx.curCamera.position = plModel->mPos.position.get();
        pGfx.curCamera.posMtx = &plModel->mPos.posMtx;
        plModel->mPos.posMtx.unlock();
    }

    //Toggle Freecam
    if (m_keysPressed.FindBuffered(KeysPressed::J) && inputDelay.Peek() > 0.5) {
        inputDelay.Mark();
        if (freeCamTGL.load()) {
            freeCamTGL.store(false);
            plModel->mPos.posMtx.lock();
            pGfx.curCamera.position = plModel->mPos.position.get();
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

    float pitch = 0;
    float yaw = 0;
    float roll = 0;
    {
        //Rotation per second in radians //Currently 90 degrees per second
        double rotationPS = ucontrolClock.Peek() * DirectX::XM_PIDIV2;

        if (m_keysPressed.FindBuffered(KeysPressed::up)) {
            pitch += rotationPS;
        }
        if (m_keysPressed.FindBuffered(KeysPressed::down)) {
            pitch += -rotationPS;
        }
        if (m_keysPressed.FindBuffered(KeysPressed::left)) {
            yaw += -rotationPS;
        }
        if (m_keysPressed.FindBuffered(KeysPressed::right)) {
            yaw += rotationPS;
        }

        if (m_keysPressed.FindBuffered(KeysPressed::Q)) {
            roll += rotationPS;
        }
        if (m_keysPressed.FindBuffered(KeysPressed::E)) {
            roll += -rotationPS;
        }
    }


    if(pitch != 0 || yaw != 0 || roll != 0)
    RotateCam(pitch, yaw, roll);


}


void Engine::RotateCam(float Pitch, float Yaw, float Roll) {
    auto updirect = XMLoadFloat4(&pGfx.curCamera.upDirection);
    auto lookdirect = XMLoadFloat4(&pGfx.curCamera.rotation);
    
    //auto gravdirect = DirectX::XMQuaternionInverse(XMLoadFloat4(&plModel->grav));
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
    UCampos();
}



Engine::~Engine() {
    eRun.store(false);
    loopVariable.notify_one();
    LoopThread.join();
    //loopLock.release();
}