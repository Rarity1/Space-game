#include "Engine.h"
#include "Exceptions.h"
#include "ObjectTracking.h"
#include "ePhysics.h"
#include <functional>
#include <memory>

Engine::Engine(Keyboard &kbd, EngineTime &clock, thRect &WindowRect, HWND &hWnd)
    : Clock(clock), storage(std::make_unique<RStorage>()) ,pTracker(std::make_unique<Tracker>(*storage)),
    tracker(*pTracker),
    pGfx(std::make_unique<Graphics>(WindowRect, hWnd, *pTracker)), 
    rGfx(*pGfx), cWorld(0, 0, 0, 0), 
    kbd(kbd)

{
  // Get tracker id for phyx instance
  phyx = std::make_unique<Physics>(updaterate, tracker);
  tMain = std::make_unique<THREADS>((int)std::thread::hardware_concurrency());
  eRun.store(true);
  LoopThread = std::thread([this] { EngineLoop(); });
}

//Initial load of the engine. 
void Engine::iLoad() {

    rGfx.LoadPipeline();
    //begin model tracking. load a gd default world mf
    //Player model needs to be set.
    //Make a better way of setting player model. 

    //Instance memory is handled by tracker will always be valid as long as tracker is valid
    auto& instance = tracker.initInstance();
    cTrackedInstance = instance.GetID();
    tracker.MakeInstanceActive(instance);
    plModel = tracker.initPhysObject(instance, "untitled", []{},2, 1,DirectX::XMFLOAT3{ 0,0,138 }, DirectX::XMFLOAT4{ 0,0,0,1}, 2000.0);
    tracker.initPhysObject(instance, "cube", []{},1, 1, DirectX::XMFLOAT3{ 10,0,138 }, DirectX::XMFLOAT4{ 0,0,0,1}, 200);
    tracker.initPhysObject(instance, "wrld", []{},4, 1, DirectX::XMFLOAT3{ 0,0,0 }, DirectX::XMFLOAT4{ 0,0,0,1}, 8570000000.0);
    //wrld is 1:50000

    for (auto i = 0; i < 20; i++) {
        float p = i * 2;
        tracker.initPhysObject(instance, "untitled", []{},2, 1, DirectX::XMFLOAT3{ 12 + p,0,138 }, DirectX::XMFLOAT4{ 0,0,0,1}, 200);
    }

    for (auto i = 0; i < 10; i++) {
        float p = i * 2;
        tracker.initPhysObject(instance, "cube", []{},1, 1, DirectX::XMFLOAT3{ 12 + p,0,138 }, DirectX::XMFLOAT4{ 0,0,0,1}, 200);
    }

    //trackedModels[0].mworld = &trackedModels[2];
    //trackedModels[1].mworld = &trackedModels[2];
    //trackedModels[3]->mworld = trackedModels[2];




    //end model tracking. begin resource upload.    
   
    loopMutex.lock();
    pauseLoop = true;
    loopMutex.unlock();

    auto& RenderedObjects = instance.GetRenderObjects();
    std::for_each(RenderedObjects.begin(), RenderedObjects.end(), [this](auto& e) {
        for (auto& o : e.second) {
            tracker.lModel((RenderedObject*)o, e.first);
        }
    });
    rGfx.LoadResources(instance);

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
    
    tsPrintBuffer::PrintFBuffered();
    UControls();
    ucontrolClock.Mark();
    loopVariable.notify_all();
    mAniUpdate();
    //Update rendered instances every frame
    rGfx.Update();
    rGfx.RenderFrame();
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

     auto& pmodl = *(PhysicsObject*)plModel;

     if (freeCamTGL.load()) {
         if (move->forward != 0 || move->left != 0) {
             // Figure this out
             using namespace DirectX;

             XMStoreFloat3(&freeCamPos, XMLoadFloat4(&rGfx.curCamera.rotation) * (float)(move->forward * updateClock.Peek()) + XMLoadFloat3(&freeCamPos));
             XMStoreFloat3(&freeCamPos, XMVector3Transform(XMLoadFloat4(&rGfx.curCamera.rotation), XMMatrixRotationAxis(XMLoadFloat4(&rGfx.curCamera.upDirection), XMConvertToRadians(90.0f))) * (float)(move->left * updateClock.Peek()) + XMLoadFloat3(&freeCamPos));

         }
         if (move->movestop) {
             pmodl.velDir = { 0,0,0 };
             pmodl.speed = 0;
         }
     }
     else {
         if (move->forward != 0 && !move->movestop) {
             float oldspeed = pmodl.speed <= 0.0001 ? 0 : pmodl.speed;
             DirectX::XMFLOAT4 forwardScale = { 0,0,0,0 };

             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&forwardScale, DirectX::XMVector3Dot(XMLoadFloat3(&pmodl.velDir), XMLoadFloat4(&rGfx.curCamera.rotation) * (fabs(move->forward) / move->forward)));
             }
             forwardScale.x = fabs(forwardScale.x);

             auto forwardSpeed = move->forward * updateClock.Peek() * 0.1;
             float totalForwardSpeed = pmodl.speed + fabs(forwardSpeed);
             float forwardSpeedScalar = fabs(forwardSpeed * (forwardScale.x) - forwardSpeed * (1 - forwardScale.x)) / totalForwardSpeed;
             float invertedForwardSpeedScalar = fabs(pmodl.speed * (forwardScale.x)) / totalForwardSpeed;
             if (forwardScale.y < 0) {
                 pmodl.speed = fabs(pmodl.speed - fabs(forwardSpeed));
             }
             else {
                 pmodl.speed = pmodl.speed + fabs(forwardSpeed);

             }

             // Figure this out
             DirectX::XMFLOAT3 Temporarydir;
             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat3(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&rGfx.curCamera.rotation) * (fabs(move->forward) / move->forward) * fabs(forwardSpeedScalar) + XMLoadFloat3(&pmodl.velDir) * invertedForwardSpeedScalar));

             }




             pmodl.velDir = Temporarydir;
         }
         if (move->left != 0 && !move->movestop) {
             float oldspeed = pmodl.speed <= 0.0001 ? 0 : pmodl.speed;
             DirectX::XMFLOAT4 leftScale = { 0,0,0,0 };

             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&leftScale, DirectX::XMVector3Dot(XMLoadFloat3(&pmodl.velDir), XMVector3Transform(XMLoadFloat4(&rGfx.curCamera.rotation), XMMatrixRotationAxis(XMLoadFloat4(&rGfx.curCamera.upDirection), XMConvertToRadians(90.0f))) * (fabs(move->left) / move->left)));
             }
             leftScale.x = fabs(leftScale.x);

             auto leftSpeed = move->left * updateClock.Peek() * 0.1;
             float totallefftSpeed = pmodl.speed + fabs(leftSpeed);
             float leftSpeedScalar = fabs(leftSpeed * (leftScale.x) - leftSpeed * (1 - leftScale.x)) / totallefftSpeed;
             float invertedleftSpeedScalar = fabs(pmodl.speed * (leftScale.x)) / totallefftSpeed;
             if (leftScale.y < 0) {
                 pmodl.speed = fabs(pmodl.speed - fabs(leftSpeed));
             }
             else {
                 pmodl.speed = pmodl.speed + fabs(leftSpeed);
             }
             // Figure this out
             DirectX::XMFLOAT3 Temporarydir;
             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat3(&Temporarydir, XMVector3Normalize(XMVector3Transform(XMLoadFloat4(&rGfx.curCamera.rotation), XMMatrixRotationAxis(XMLoadFloat4(&rGfx.curCamera.upDirection), XMConvertToRadians(90.0f))) * (fabs(move->left) / move->left) * fabs(leftSpeedScalar) + XMLoadFloat3(&pmodl.velDir) * invertedleftSpeedScalar));

             }




             pmodl.velDir = Temporarydir;
         }


         
         if (move->movestop) {
             pmodl.velDir = { 0,0,0 };
             pmodl.speed = 0;
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
    for (auto& Instance : tracker.GetActiveInstances()) {
      auto& Objects = Instance->GetPhysicsObjects();
        std::for_each( Objects.begin(), Objects.end(), [](auto& e) { ((PhysicsObject*)e)->UpdatePosition(); });
    }
}

void Engine::uPhysics()
{

    phyx->Update();

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
    queueCommand([this] {
        cPlayermodel();
    }, 0);
    queueCommand([this] {
        uPhysics();
    }, 20);
    queueCommand([this] {
        uPosInstances();
        }, 21);
    //queueCommand([this] {rGfx.Update(tracker.trackedObjects);}, 65535);
    eWref = tMain->gPushWork(getCQueue());
}

void Engine::enQueueExternCommands()
{
}

void Engine::UCampos() {
    auto& pmodl = *(PhysicsObject*)plModel;
    //Please add a threadsafe way to update position
    if (rGfx.curCamera.position == nullptr) {
        rGfx.curCamera.position = pmodl.mPos.GetPtr();
    }

    //Toggle Freecam
    if (m_keysPressed.FindBuffered(KeysPressed::J) && inputDelay.Peek() > 0.5) {
        inputDelay.Mark();
        if (freeCamTGL.load()) {
            freeCamTGL.store(false);
            rGfx.curCamera.position = pmodl.mPos.GetPtr();
        }
        else {
            freeCamTGL.store(true);
            freeCamPos = pmodl.mPos.Get();
            rGfx.curCamera.position = &freeCamPos;
            //rGfx.curCamera.posMtx = &freeCamMTX;
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
    auto updirect = XMLoadFloat4(&rGfx.curCamera.upDirection);
    auto lookdirect = XMLoadFloat4(&rGfx.curCamera.rotation);
    
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
    
    
    DirectX::XMStoreFloat4(&rGfx.curCamera.rotation, lookdirect);
    DirectX::XMStoreFloat4(&rGfx.curCamera.upDirection, updirect);
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