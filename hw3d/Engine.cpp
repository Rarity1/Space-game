#include "Engine.h"

Engine::Engine(Graphics* gfx, Keyboard* kbd):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd)
{
}


void Engine::iLoad() {
    pGfx->~Graphics();
    pGfx->LoadPipeline();
    //begin model tracking.
    trackedModels.emplace_back(new eResource{ 0, "cube"});
    trackedModels.emplace_back(new eResource{ 0, "cube" });

    //end model tracking.
    auto temp = 0.0f;
    for (auto& m : trackedModels) {
        m->loadedModel = new RStorage::bmResource{m->name};
        m->loadedModel->umID = m->umID;
        pGfx->loadModels(m->umID, m->loadedModel);
        pGfx->SetModelPosition(m->loadedModel, m->initPos, {0,temp,0});
        temp += 10;
    }
    pGfx->LoadResources();
    if (engInit) {
        engInit = false;
    }
}

void Engine::Update()
{
	UCampos();
    pGfx->OnUpdate();
}


void Engine::UCampos() {
    XMFLOAT3 temppos = { pGfx->curCamera.position.x, pGfx->curCamera.position.y, pGfx->curCamera.position.z };
    bool tbool = true;
    while (tbool) {
        auto ss = kbd->ReadKey();
        if (ss.has_value()) {
            if (ss->IsRelease()) {
                OnKeyUp(ss->GetCode());
            }
            if (ss->IsPress()) {
                OnKeyDown(ss->GetCode());
            }
        }
        else {
            tbool = false;
        }
        

    }
    //free cam
    XMFLOAT3 move{ 0,0,0 };
    if (m_keysPressed.w) {
        move.y -= 1.0f;
    }if (m_keysPressed.s) {
        move.y += 1.0f;
    }if (m_keysPressed.a) {
        move.x -= 1.0f;
    }if (m_keysPressed.d) {
        move.x += 1.0f;
    }
    if (m_keysPressed.left) {
        pGfx->curCamera.rotation.yaw += 0.1;
    }
   
    if (m_keysPressed.right) {
        pGfx->curCamera.rotation.yaw -= 0.1;
    }
       
    if (m_keysPressed.up) {
        pGfx->curCamera.rotation.pitch += 0.1;
    }
        
    if (m_keysPressed.down) {
        pGfx->curCamera.rotation.pitch -= 0.1;
    }
        
    temppos.x += move.x * -cosf(pGfx->curCamera.rotation.yaw) - move.y * sinf(pGfx->curCamera.rotation.yaw);
    temppos.y += move.x * sinf(pGfx->curCamera.rotation.yaw) - move.y * cosf(pGfx->curCamera.rotation.yaw);
    pGfx->curCamera.position = {temppos.x,temppos.y,temppos.z,0};
    //free cam end

	
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

Engine::~Engine() {
    for (auto& m : trackedModels) {
        delete m->loadedModel;
        delete m;
    }
}