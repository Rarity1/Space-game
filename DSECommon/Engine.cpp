#include "Engine.h"

Engine::Engine(Graphics& gfx, Keyboard& kbd, EngineTime& clock):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd),
    //trackedObjects(gfx.lModels->trackedObjects),
    Clock(clock)
{
    tracker = std::make_unique<Tracker>(*pGfx.rStorage);
    phyx = std::make_unique<Physics>(Clock, updaterate);
    tMain = std::make_unique<THREADS>((int)std::thread::hardware_concurrency());
}

//Initial load of the engine. 
void Engine::iLoad() {

    //Move everything between this into a function in Graphics

    //begin model tracking. load a gd default world mf
    //Player model needs to be set.
    //Make a better way of setting player model. 
    cTrackedInstance = 1;
    tracker->initInstance(cTrackedInstance);
    plModel = tracker->initObject(tracker->getInstance(cTrackedInstance), "untitled", 2, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 0,0,138 });
    tracker->initObject(tracker->getInstance(cTrackedInstance), "cube", 1, 1, 200.0, 0.01, DirectX::XMFLOAT3{ 10,0,138 });
    tracker->initObject(tracker->getInstance(cTrackedInstance), "wrld", 4, 1, 8570000000.0 , 0.3, DirectX::XMFLOAT3{ 0,0,0 });
    //wrld is 1:50000

    for (auto i = 0; i < 1; i++) {
        float p = i * 1;
        tracker->initObject(tracker->getInstance(cTrackedInstance), "untitled", 2, 1, 200, 0.3, DirectX::XMFLOAT3{ 12 + p,0,138 });
    }

    for (auto i = 0; i < 1; i++) {
        float p = i * 1;
       tracker->initObject(tracker->getInstance(cTrackedInstance), "untitled", 1, 1, 200, 0.3, DirectX::XMFLOAT3{ 12 + p,0,138 });
    }

    //trackedModels[0].mworld = &trackedModels[2];
    //trackedModels[1].mworld = &trackedModels[2];
    //trackedModels[3]->mworld = trackedModels[2];




    //end model tracking. begin resource upload.    
    pGfx.LoadPipeline();
    pGfx.LoadResources(tracker->getInstance(cTrackedInstance));

    if (engInit) {
        engInit = false;
    }
    eRun.store(true);
    for (auto& t : tracker->getInstance(cTrackedInstance).tmodelLinkedObjects) {
        for (auto& m : t.second) {
            pGfx.UpdateModel(m);
        }
    }
    phyx->trackM(tracker->getInstance(cTrackedInstance));
}

bool Engine::Update()
{
    UControls();
    UCampos();
    if (Clock.Peek() >= 1.0 / updaterate) {
        engineTimeTaken = Clock.Mark();
        enQueueEngineCommands();
        enQueueExternCommands();
        eventBusSync();
        mAniUpdate();
        pGfx.Update(tracker->getInstance(cTrackedInstance));
        auto peek = Clock.Peek();

        return true;
    }
    return false;
}

void Engine::RenderI()
{
    pGfx.RenderFrame(tracker->getInstance(cTrackedInstance));
}



 void Engine::cPlayermodel()
 {
     std::unique_ptr<Movement> move(std::make_unique<Movement>());

     {
         auto movespeed = 1.0;
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
         if (move->forward != 0) {
             float speedchange = move->forward * 10 / updaterate;
             // Figure this out
             using namespace DirectX;
             XMFLOAT4 Temporarydir;
             DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx.curCamera.rotation) * fabs(speedchange)));


             XMStoreFloat3(&freeCamPos, XMLoadFloat4(&Temporarydir) * speedchange + XMLoadFloat3(&freeCamPos));
         }
         if (move->movestop) {
             plModel->velDir = { 0,0,0,0 };
             plModel->speed = 0;
         }
     }
     else {
         if (move->forward != 0 && !move->movestop) {
             float oldspeed = plModel->speed <= 0.0001 ? 0 : plModel->speed;
             float speedchange = move->forward * 1.0 / updaterate;
             DirectX::XMFLOAT4 scale = { 0,0,0,0 };
             {
                 using namespace DirectX;
                 auto scalar = DirectX::XMVector3Dot(XMLoadFloat4(&plModel->velDir), XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move->forward) / move->forward));
                 DirectX::XMStoreFloat4(&scale, scalar);

             }
             scale.x = fabs(scale.x);
             float totalspeed = plModel->speed + fabs(speedchange);

             float speedscal = fabs(speedchange * (scale.x) - speedchange * (1 - scale.x)) / totalspeed;
             float inspeedscal = fabs(plModel->speed * (scale.x)) / totalspeed;
             // Figure this out
             DirectX::XMFLOAT4 Temporarydir;
             {
                 using namespace DirectX;
                 DirectX::XMStoreFloat4(&Temporarydir, XMVector3Normalize(XMLoadFloat4(&pGfx.curCamera.rotation) * (fabs(move->forward) / move->forward) * fabs(speedscal) + XMLoadFloat4(&plModel->velDir) * inspeedscal));

             }


             if (scale.y < 0) {
                 plModel->speed = fabs(plModel->speed - fabs(speedchange));
             }
             else {
                 plModel->speed = plModel->speed + fabs(speedchange);

             }

             plModel->velDir = Temporarydir;
         }
         else if (move->movestop) {
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




int Engine::eventBusSync()
{
    tMain->gEndWork(eWref);
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
        phyx->Update(tracker->getInstance(cTrackedInstance));
    }, 20);
    queueCommand([this] {
        std::for_each(tracker->getInstance(cTrackedInstance).tmodelLinkedObjects.begin(), tracker->getInstance(cTrackedInstance).tmodelLinkedObjects.end(), [this](auto& e) { 
            for (auto& tObjects : e.second) {
                tObjects->UpdatePosition();
            }
        
        });
    }, 21);
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

    float pitch = 0;
    float yaw = 0;
    float roll = 0;
    {
        //Rotation per second in radians //Currently 15 degrees per second
        double rotationPS = Clock.Peek() * DirectX::XM_PIDIV4/3;

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

void Engine::sPGraphics(Object& model)
{


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

}



Engine::~Engine() {

    eRun.store(false);
}