#include "Engine.h"
#include "Exceptions.h"
#include "InputHandler.h"
#include "ObjectTracking.h"
#include "ePhysics.h"
#include <functional>
#include <memory>

Engine::Engine(Input &InputHandler, WRect &WindowRect, HWND &hWnd)
    : storage(std::make_unique<RStorage>()) ,pTracker(std::make_unique<Tracker>(*storage, InputHandler)),
    tracker(*pTracker),
    pGfx(std::make_unique<Graphics>(WindowRect, hWnd, *pTracker)), 
    rGfx(*pGfx), cWorld(0, 0, 0, 0), 
    InputHndlr(InputHandler)

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
    auto instanceID = instance.GetID();
    cTrackedInstance = instance.GetID();
    tracker.MakeInstanceActive(instance);

    //Move these custom lambda functions to their own functions so this isnt so messy.
    //Also figure out a clean way to decouple camera updates from engine tickrate
    auto Camera =
        tracker.initCameraObject(instance, "Main", [] {}, {0,0,0} , {1,0,0,0}, {0,0,1,0});

    Camera->MakeActive();

    plModel = tracker.initPhysObject(
        instance, "untitled",
        [this, Camera] {
          struct Movement {
            float forward = 0.0;
            float backward = 0.0;
            float left = 0.0;
            float right = 0.0;
            bool movestop = false;
          };
          Movement move;

          {
            auto movespeed = 2.0;
            if (InputHndlr.keyboard.KeyIsPressed('W')) {
              move.forward += movespeed;
            }
            if (InputHndlr.keyboard.KeyIsPressed('S')) {
              move.forward -= movespeed;
            }
            if (InputHndlr.keyboard.KeyIsPressed('A')) {
              move.left += movespeed;
            }
            if (InputHndlr.keyboard.KeyIsPressed('D')) {
              move.left -= movespeed;
            }
            if (InputHndlr.keyboard.KeyIsPressed('K')) {
              move.movestop = true;
            }
          }
          if (plModel == nullptr)
            return;
          if (Camera == nullptr)
            return;
          auto &pmodl = *(PhysicsObject *)plModel;
          auto CameraRotation = Camera->cPos.GetRotation();
          auto CameraUpDir = Camera->cPos.GetUpDirection();
          if (!Camera->isFree()) {
            if (move.forward != 0 && !move.movestop) {
              float oldspeed = pmodl.speed <= 0.0001 ? 0 : pmodl.speed;
              FLOAT4 forwardScale = {0, 0, 0, 0};

              {
                using namespace DirectX;

                DirectX::XMStoreFloat4(
                    (XMFLOAT4*)&forwardScale,
                    DirectX::XMVector3Dot(
                        XMLoadFloat3((XMFLOAT3*)&pmodl.velDir),
                        XMLoadFloat4((XMFLOAT4*)&CameraRotation) *
                            (fabs(move.forward) / move.forward)));
              }
              forwardScale.x = fabs(forwardScale.x);

              auto forwardSpeed = move.forward * updateClock.Peek() * 0.1;
              float totalForwardSpeed = pmodl.speed + fabs(forwardSpeed);
              float forwardSpeedScalar =
                  fabs(forwardSpeed * (forwardScale.x) -
                       forwardSpeed * (1 - forwardScale.x)) /
                  totalForwardSpeed;
              float invertedForwardSpeedScalar =
                  fabs(pmodl.speed * (forwardScale.x)) / totalForwardSpeed;
              if (forwardScale.y < 0) {
                pmodl.speed = fabs(pmodl.speed - fabs(forwardSpeed));
              } else {
                pmodl.speed = pmodl.speed + fabs(forwardSpeed);
              }

              // Figure this out
              FLOAT3 Temporarydir;
              {
                using namespace DirectX;
                DirectX::XMStoreFloat3(
                    (XMFLOAT3*)&Temporarydir,
                    XMVector3Normalize(XMLoadFloat4((XMFLOAT4*)&CameraRotation) *
                                           (fabs(move.forward) / move.forward) *
                                           fabs(forwardSpeedScalar) +
                                       XMLoadFloat3((XMFLOAT3*)&pmodl.velDir) *
                                           invertedForwardSpeedScalar));
              }

              pmodl.velDir = *(FLOAT3*)&Temporarydir;
            }
            if (move.left != 0 && !move.movestop) {
              float oldspeed = pmodl.speed <= 0.0001 ? 0 : pmodl.speed;
              FLOAT4 leftScale = {0, 0, 0, 0};

              {
                using namespace DirectX;
                DirectX::XMStoreFloat4(
                    (XMFLOAT4*)&leftScale,
                    DirectX::XMVector3Dot(
                        XMLoadFloat3((XMFLOAT3*)&pmodl.velDir),
                        XMVector3Transform(
                            XMLoadFloat4((XMFLOAT4*)&CameraRotation),
                            XMMatrixRotationAxis(XMLoadFloat4((XMFLOAT4*)&CameraUpDir),
                                                 XMConvertToRadians(90.0f))) *
                            (fabs(move.left) / move.left)));
              }
              leftScale.x = fabs(leftScale.x);

              auto leftSpeed = move.left * updateClock.Peek() * 0.1;
              float totallefftSpeed = pmodl.speed + fabs(leftSpeed);
              float leftSpeedScalar = fabs(leftSpeed * (leftScale.x) -
                                           leftSpeed * (1 - leftScale.x)) /
                                      totallefftSpeed;
              float invertedleftSpeedScalar =
                  fabs(pmodl.speed * (leftScale.x)) / totallefftSpeed;
              if (leftScale.y < 0) {
                pmodl.speed = fabs(pmodl.speed - fabs(leftSpeed));
              } else {
                pmodl.speed = pmodl.speed + fabs(leftSpeed);
              }
              // Figure this out
              FLOAT3 Temporarydir;
              {
                using namespace DirectX;
                DirectX::XMStoreFloat3(
                    (XMFLOAT3*)&Temporarydir,
                    XMVector3Normalize(
                        XMVector3Transform(
                            XMLoadFloat4((XMFLOAT4*)&CameraRotation),
                            XMMatrixRotationAxis(XMLoadFloat4((XMFLOAT4*)&CameraUpDir),
                                                 XMConvertToRadians(90.0f))) *
                            (fabs(move.left) / move.left) *
                            fabs(leftSpeedScalar) +
                        XMLoadFloat3((XMFLOAT3*)&pmodl.velDir) * invertedleftSpeedScalar));
              }

              pmodl.velDir = *(FLOAT3*)&Temporarydir;
            }

            if (move.movestop) {
              pmodl.velDir = {0, 0, 0};
              pmodl.speed = 0;
            }
          }
          updateClock.Mark();
        },
        2, 1, FLOAT3{0, 0, 138}, FLOAT4{0, 0, 0, 1},
        2000.0);
    Camera->LinkTo((RenderedObject*)plModel);
    //Camera->AddParent(plModel);
    tracker.initPhysObject(instance, "cube", []{},1, 1, FLOAT3{ 10,0,138 }, FLOAT4{ 0,0,0,1}, 200);
    tracker.initPhysObject(instance, "wrld", []{},4, 1, FLOAT3{ 0,0,0 }, FLOAT4{ 0,0,0,1}, 8570000000.0);
    //wrld is 1:50000

    for (auto i = 0; i < 20; i++) {
        float p = i * 2;
        tracker.initPhysObject(instance, "untitled", []{},2, 1, FLOAT3{ 12 + p,0,138 }, FLOAT4{ 0,0,0,1}, 200);
    }

    for (auto i = 0; i < 10; i++) {
        float p = i * 2;
        tracker.initPhysObject(instance, "cube", []{},1, 1, FLOAT3{ 12 + p,0,138 }, FLOAT4{ 0,0,0,1}, 200);
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
    InputHndlr.DispatchInputEvents();
    loopVariable.notify_all();
    //Update rendered instances every frame
    rGfx.Update();
    rGfx.RenderFrame();
    tsPrintBuffer::PrintFBuffered();
    return true;
}


//Calculate instances based on playermodel position ?

void Engine::mAniUpdate(){
    using namespace DirectX;

    /*
        if (m_keysPressed.FindBuffered(KeysPressed::K)) {

        XMFLOAT4 up(1,0,0, 0);
        auto temp = DirectX::XMQuaternionRotationAxis(XMVector4Normalize(XMLoadFloat4((XMFLOAT4*)&up)), 10 * Clock.Current());
        auto left = DirectX::XMQuaternionMultiply(temp, XMLoadFloat4((XMFLOAT4*)&trackedObjects[1].mPos.rotation));


        XMStoreFloat4((XMFLOAT4*)&trackedObjects[1].mPos.rotation, left);


    }
    */
}

void Engine::updateInstances()
{
    for (auto& Instance : tracker.GetActiveInstances()) {
      Instance->InstanceObject->UpdateChildren();
    }
}

void Engine::uPhysics()
{

    phyx->Update();

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
    }, 0);
    queueCommand([this] {
        uPhysics();
    }, 20);
    queueCommand([this] {
        updateInstances();
        }, 21);
    //queueCommand([this] {rGfx.Update(tracker.trackedObjects);}, 65535);
    eWref = tMain->gPushWork(getCQueue());
}

void Engine::enQueueExternCommands()
{
}



FLOAT3 Engine::rWorld(FLOAT3 pos1) {
	return  { cWorld.x+pos1.x,cWorld.y+pos1.y,cWorld.z+pos1.z};
}
FLOAT3 Engine::dWorld(FLOAT3 pos1) {
    return  { -cWorld.x + pos1.x,-cWorld.y + pos1.y,-cWorld.z + pos1.z };
}
FLOAT3 Engine::cnWorld(FLOAT3 pos1) {
    auto tworld = { cWorld.x - nWorld.x, cWorld.x - nWorld.x, cWorld.x - nWorld.x};
    return  { -cWorld.x + pos1.x,-cWorld.y + pos1.y,-cWorld.z + pos1.z };
}



Engine::~Engine() {
    eRun.store(false);
    loopVariable.notify_one();
    LoopThread.join();
    //loopLock.release();
}