#include "Engine.h"

Engine::Engine(Graphics* gfx, Keyboard* kbd):
	pGfx(gfx),
    cWorld(0,0,0,0),
    kbd(kbd),
    GConst(6.67430 * pow(10, -11))
{
}


void Engine::iLoad() {
    pGfx->~Graphics();
    pGfx->LoadPipeline();
    //begin model tracking. load a gd default world mf
    trackedModels.emplace_back(new eResource{ 0, "cube"});
    trackedModels.emplace_back(new eResource{ 1, "untitled" });
    //wrld is 1:50000
    trackedModels.emplace_back(new eResource{ 2, "wrld" , true});

    //end model tracking.
    for (auto& m : trackedModels) {
        m->loadedModel.model = new RStorage::bmResource{m->name};
        m->loadedModel.model->umID = m->umID;
        pGfx->loadModels(m->umID, m->loadedModel.model);
        auto tempc = 0;
        XMFLOAT3 temp1;
        XMFLOAT3 temp2;
        XMFLOAT3 temp3;
        for (auto& v : m->loadedModel.model->uData->vertexData()) {
            switch (tempc%3) {
            case 0:
                temp1 = v.position;
                break;
            case 1:
                temp2 = v.position;
                break;
            case 2:
                temp3 = v.position;
                XMFLOAT3 tempcoll = { temp1.x + temp2.x + temp3.x / 3, temp1.y + temp2.y + temp3.y / 3, temp1.z + temp2.z + temp3.z / 3 };
                m->collision.emplace_back(pCollision{ tempcoll, fDistance(tempcoll, temp1) });
                break;
            }
            tempc++;
            
        }
        //testing
        if (m->umID == 0) {
            m->loadedModel.position = { 03,10,-200 };
            m->mworld = trackedModels[2];
            m->mass = 10;
        }
        else if (m->umID == 2) {
            m->mass = 8570000000;
        }
        pGfx->SetModelPosition(&m->loadedModel);
    }
    pGfx->LoadResources();
    if (engInit) {
        engInit = false;
    }
}

void Engine::Update(float frametime)
{
    UControls();
    timer += frametime;
    if (timer >= 16) {
        cMPosUpdate();
        UCampos();
        timer = 0;
    }
    pGfx->OnUpdate();
}

void Engine::cPlayermodel()
{


}

void Engine::cMPosUpdate(){
    for (auto& m : trackedModels) {
        if (!m->isWorld && m->mworld != nullptr) {
            m->loadedModel.lastposition = m->loadedModel.position;
            cGravity(m);
            m->loadedModel.position.x += m->speed.x;
            m->loadedModel.position.y += m->speed.y;
            m->loadedModel.position.z += m->speed.z;
            m->loadedModel.which = RStorage::BOTH;
        }
        else if (!m->isWorld) {
            m->loadedModel.lastposition = m->loadedModel.position;
            m->loadedModel.position.x += m->speed.x;
            m->loadedModel.position.y += m->speed.y;
            m->loadedModel.position.z += m->speed.z;
            m->loadedModel.which = RStorage::BOTH;
        }
    pGfx->SetModelPosition(&m->loadedModel);
    }
   
}
void Engine::UCampos() {
    //Link the camera position here to whatever you want.
    pGfx->curCamera.position.x = trackedModels[1]->loadedModel.position.x;
    pGfx->curCamera.position.y = trackedModels[1]->loadedModel.position.y;
    pGfx->curCamera.position.z = trackedModels[1]->loadedModel.position.z;




    XMFLOAT3 temppos{ 0,0,0 };
    //free cam
    Movement move{0,0,0};
    /*if (m_keysPressed.w) {
        move.forward += 1.0f;
    }if (m_keysPressed.s) {
        move.forward -= 1.0f;
    }if (m_keysPressed.a) {
        move.left += 1.0f;
    }if (m_keysPressed.d) {
        move.left -= 1.0f;
    }*/
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
    temppos.x = move.forward * cosf(pGfx->curCamera.rotation.pitch) * cosf(pGfx->curCamera.rotation.yaw);
    temppos.y = move.forward * cosf( pGfx->curCamera.rotation.pitch) * sinf(pGfx->curCamera.rotation.yaw);
    temppos.z = move.forward * sinf(pGfx->curCamera.rotation.pitch);

    temppos.x += move.left * cosf(pGfx->curCamera.rotation.yaw+XM_PIDIV2);
    temppos.y += move.left * sinf(pGfx->curCamera.rotation.yaw + XM_PIDIV2);
    //temppos.z += move.left * (pGfx->curCamera.rotation.pitch);

    if (temppos.x > 1) {
        temppos.x /= 2;
    }
    if (temppos.y > 1) {
        temppos.y /= 2;
    }if (temppos.z > 1) {
        temppos.z /= 2;
    }
    pGfx->curCamera.position.x += temppos.x;
    pGfx->curCamera.position.y += temppos.y;
    pGfx->curCamera.position.z += temppos.z;
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
float Engine::fDistance(XMFLOAT3 pos1, XMFLOAT3 pos2) {
    auto x = pos2.x - pos1.x;
    x *= x;
    auto y = pos2.y - pos1.y;
    y *= y;
    auto z = pos2.z - pos1.z;
    z *= z;
    auto distance = sqrt(x+y+z);
    return distance;
}

void Engine::cGravity(eResource* obj) {
    auto distance = fDistance(obj->loadedModel.position, obj->mworld->loadedModel.position);
    auto gaccel = (GConst * (obj->mworld->mass)) / pow(distance, 2);
    bool complex = false;
    double changex = (obj->mworld->loadedModel.position.x-obj->loadedModel.position.x);
    double changey =  (obj->mworld->loadedModel.position.y -obj->loadedModel.position.y);
    double changez = (obj->mworld->loadedModel.position.z-obj->loadedModel.position.z);
    auto mag = sqrt(pow(changex, 2) + pow(changey, 2) + pow(changez, 2));

    changex /= mag;
    changey /= mag;
    changez /= mag;

    obj->loadedModel.position.x += gaccel*changex;
    obj->loadedModel.position.y += gaccel*changey;
    obj->loadedModel.position.z += gaccel*changez;

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
        delete m->loadedModel.model;
        delete m;
    }
}