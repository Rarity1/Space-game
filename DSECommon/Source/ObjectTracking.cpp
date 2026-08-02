#include "ObjectTracking.h"
#include "InputHandler.h"
#include "ePhysics.h"
#include <DirectXMath.h>
#include <functional>
#include <memory>




void Tracker::lModel(RenderedObject* obj, umID umID) noexcept
{
  auto& ob = *obj;
	ob.model = storage.loadModel(umID);
	ob.model->curTexture = storage.getTexture(ob.model->name);
	ob.loadedModel = true;
}


Tracker::Instance::Instance(InstID instanceID, Tracker& Parent,  std::function<void()> func):
instanceID(instanceID),
pTracker(Parent){
   InstanceObject = Parent.initOriginObject(this, func);
};

RStorage::bmResource& Tracker::GetModel(umID umID)
{
	return storage.GetModel(umID);
}

Tracker::Instance& Tracker::initInstance(InstID instanceID, std::function<void()> func)
{
  if(Instances.contains(instanceID)){
    return initInstance(instanceID + 1);
  }else{
    Instances.insert({instanceID, std::make_unique<Instance>(instanceID, *this, func)});
    return *Instances[instanceID];
  }
}

Tracker::Instance& Tracker::getInstance(InstID instanceID)
{
	if (!Instances.contains(instanceID)) {
		return initInstance(instanceID);
	}
	return *Instances[instanceID];
}

void Tracker::MakeInstanceActive(Instance &inst) {
  if (isInstanceActive.contains(inst.instanceID)) {
    if (isInstanceActive[inst.instanceID] != &inst.Active) {
      isInstanceActive[inst.instanceID] = &inst.Active;
      inst.Active.store(true);
      ActiveInstances.emplace_back(&inst);
    }
    inst.Active.store(true);
    ActiveInstances.emplace_back(&inst);

    return;
  }
  inst.Active.store(true);
  ActiveInstances.emplace_back(&inst);
  isInstanceActive.insert({inst.instanceID, &inst.Active});
}

Object *Tracker::initOriginObject(Instance* oInstance,
                            std::function<void()> func) {
  auto uOID = idTracker.AllocID();
  oInstance->uniqueOrigin =  std::make_unique<Object>(uOID, func, oInstance);
  return oInstance->uniqueOrigin.get();
}

Object *Tracker::initObject(Instance &oInstance, std::string Name,
                            std::function<void()> func) {
  auto uOID = idTracker.AllocID();
  TrackedObjects.insert(
      {uOID, std::make_unique<Object>(Name, uOID, func, &oInstance)});
  return TrackedObjects[uOID].get();
}

CameraObject* Tracker::initCameraObject(Tracker::Instance& pInstance, std::string name, std::function<void()> func, 
     DirectX::XMFLOAT3 initPos, 
    DirectX::XMFLOAT4 initRot, DirectX::XMFLOAT4 initUpDirection, 
    RenderedObject* link, bool Active){
      auto UOID = idTracker.AllocID();
  TrackedObjects.insert(
      {UOID, std::make_unique<CameraObject>(name,  UOID, func, 
    &pInstance, initPos, 
    initRot, initUpDirection, 
    link, Active)});
  return (CameraObject *)TrackedObjects[UOID].get();
    }

// Make objects able to be in multiple instances without loading new data
PhysicsObject *
Tracker::initPhysObject(Instance &Instance, std::string Name,
                    std::function<void()> func, umID filebModelIndex,
                    float mScale, DirectX::XMFLOAT3 initPos,
                    DirectX::XMFLOAT4 initRot, float mMass, float mFriction,
                    DirectX::XMFLOAT3 initVelDir, float initSpeed) {

  auto UOID = idTracker.AllocID();
  TrackedObjects.insert(
      {UOID, std::make_unique<PhysicsObject>(
                 Name, UOID, func, &Instance, nullptr,
                 filebModelIndex, mScale,
                 initPos, initRot, mMass, mFriction, initVelDir, initSpeed)});
  return (PhysicsObject *)TrackedObjects[UOID].get();
}

void Tracker::unloadObject(UOID obj)
{

}

void Tracker::Instance::AddObject(Object* obj){
  InstanceObjLock.lock();
  InstanceObjects.emplace_back(obj);
  Count++;
  obj->LinkedInstance = this;
  InstanceObjLock.unlock();
}




//This will definitely leak memory if the previous link isnt removed
void Tracker::Instance::LinkToModel(RenderedObject *obj, umID umID) {
  obj->CBVIndex = ModelLinkedObjects[umID].size();
  ModelLinkedObjects[umID].emplace_back(obj);
  if (!instancedCBVData.contains(umID))
    instancedCBVData.insert({umID, nullptr});
}

//Will leak memory and probably cause issues if not removed from previous tracker.
void Tracker::Instance::EnablePhysics(PhysicsObject *obj, UOID uOID) {
  PhysicsObjects.emplace_back(obj);
  physEnabled.insert({uOID, true});
}


void Tracker::Instance::MakeCamera(CameraObject *obj, UOID uOID) {
  Cameras.insert({uOID,obj});
  physEnabled.insert({uOID, true});
}

CameraObject* Tracker::Instance::ActiveCamera(CameraObject *obj) {
  if(obj != nullptr){
    if(Cameras.contains(obj->uOID)){
      auto oldCamera = activeCamera;
      activeCamera = obj;
      return (CameraObject*)oldCamera;
    }
  }else{
    return (CameraObject*)activeCamera;
  }
}

Object::Object(UOID uOID, std::function<void()> func,
               Tracker::Instance *pInstance)
    : name("OriginObject"), uOID(uOID), Script(func) {
      isOrigin = true;
      LinkedInstance = pInstance;
    };

Object::Object(std::string name, UOID uOID, std::function<void()> func,
               Tracker::Instance *pInstance)
    : name(name), uOID(uOID), Script(func) {
  pInstance->AddObject(this);
  AddParent(pInstance->InstanceObject);
};

void CameraObject::Update() {
  Script();
  // Toggle Freecam
	struct Movement {
		float forward = 0.0;
		float backward = 0.0;
		float left = 0.0;
		float right = 0.0;
		bool movestop = false;
	};
  Movement move;

  //Make a helper function for this so I dont have to do it for every single key
  if(dispID.Empty.load()){
    dispID = LinkedInstance->pTracker.InputHndlr.linkEvent('J', [this]{
    freeCam.store(freeCam.load() ? false : true);
  });
  }
  

  if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('J') && inputDelay.Peek() > 0.5) {
    inputDelay.Mark();
    //ToggleFreedom();
  }
  float pitch = 0;
  float yaw = 0;
  float roll = 0;
  {
    // Rotation per second in radians //Currently 90 degrees per second
    double rotationPS = ucontrolClock.Peek() * DirectX::XM_PIDIV2;
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed(VK_UP)) {
      pitch += rotationPS;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed(VK_DOWN)) {
      pitch += -rotationPS;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed(VK_LEFT)) {
      yaw += -rotationPS;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed(VK_RIGHT)) {
      yaw += rotationPS;
    }

    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('Q')) {
      roll += rotationPS;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('E')) {
      roll += -rotationPS;
    }
  }

  if (pitch != 0 || yaw != 0 || roll != 0)
    Rotate(pitch, yaw, roll);
  {
    auto movespeed = 2.0;
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('W')) {
      move.forward += movespeed;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('S')) {
      move.forward -= movespeed;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('A')) {
      move.left += movespeed;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('D')) {
      move.left -= movespeed;
    }
    if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('K')) {
      move.movestop = true;
    }
  }
  if (isFree()) {
    if (move.forward != 0 || move.left != 0) {
      // Figure this out
      using namespace DirectX;

      auto Pos = cPos.Get();
      auto Rotation = cPos.GetRotation();
      auto upDirection = cPos.GetUpDirection();
      XMStoreFloat3(&Pos, XMLoadFloat4(&Rotation) *
                                  (float)(move.forward * ucontrolClock.Peek()) +
                              XMLoadFloat3(&Pos));
      XMStoreFloat3(&Pos, XMVector3Transform(
                              XMLoadFloat4(&Rotation),
                              XMMatrixRotationAxis(XMLoadFloat4(&upDirection),
                                                   XMConvertToRadians(90.0f))) *
                                  (float)(move.left * ucontrolClock.Peek()) +
                              XMLoadFloat3(&Pos));
      cPos.Move(Pos);
    }
  } else if ((move.forward != 0 || move.left != 0) && GetLinked()) {
    using namespace DirectX;
    auto Pos = GetLinked()->mPos.Get();
    auto Rotation = cPos.GetRotation();
    auto upDirection = cPos.GetUpDirection();
    XMStoreFloat3(&Pos, XMLoadFloat4(&Rotation) *
                                (float)(move.forward * ucontrolClock.Peek()) +
                            XMLoadFloat3(&Pos));
    XMStoreFloat3(&Pos, XMVector3Transform(
                            XMLoadFloat4(&Rotation),
                            XMMatrixRotationAxis(XMLoadFloat4(&upDirection),
                                                 XMConvertToRadians(90.0f))) *
                                (float)(move.left * ucontrolClock.Peek()) +
                            XMLoadFloat3(&Pos));
    cPos.Move(Pos);
  }
  ucontrolClock.Mark();
  // Toggle Freecam
  if (linkedObject != nullptr && !freeCam.load()) {
    auto postomov = linkedObject->mPos.Get();
    cPos.Move(postomov);
  }
  auto position = cPos.Get();
  auto rotation = cPos.GetRotation();
  auto upDirection = cPos.GetUpDirection();
  DirectX::XMStoreFloat4x4(
      &cMatrix, DirectX::XMMatrixLookToRH(XMLoadFloat3(&position),
                                          XMLoadFloat4(&rotation),
                                          XMLoadFloat4(&upDirection)));
}

void CameraObject::LinkTo(RenderedObject *obj) {
  if (linkedObject) {
    linkedObject = nullptr;
  }
  linkedObject = obj;
}

void CameraObject::Rotate(float &Pitch, float &Yaw, float &Roll) {
  auto upDirection = cPos.GetUpDirection();
  auto updirect = XMLoadFloat4(&upDirection);
  DirectX::XMFLOAT4 rotation = cPos.GetRotation();
  auto lookdirect = XMLoadFloat4(&rotation);

  // auto gravdirect =
  // DirectX::XMQuaternionInverse(XMLoadFloat4(&plModel->grav));
  if (Yaw != 0) {
    auto temp = DirectX::XMQuaternionRotationNormal(updirect, Yaw);
    auto left = DirectX::XMQuaternionMultiply(temp, lookdirect);

    lookdirect = DirectX::XMQuaternionMultiply(
        left, DirectX::XMQuaternionConjugate(temp));
    auto qup = DirectX::XMQuaternionMultiply(temp, updirect);
    updirect = DirectX::XMQuaternionMultiply(
        qup, DirectX::XMQuaternionConjugate(temp));
  }
  if (Roll != 0) {
    auto temp = DirectX::XMQuaternionRotationNormal((lookdirect), Roll);
    auto qup = DirectX::XMQuaternionMultiply(temp, updirect);
    updirect = DirectX::XMQuaternionMultiply(
        qup, DirectX::XMQuaternionConjugate(temp));
    auto left = DirectX::XMQuaternionMultiply(temp, lookdirect);
    lookdirect = DirectX::XMQuaternionMultiply(
        left, DirectX::XMQuaternionConjugate((temp)));
  }
  if (Pitch != 0) {
    auto temp = DirectX::XMQuaternionRotationNormal(
        DirectX::XMQuaternionMultiply(lookdirect, updirect), Pitch);
    auto qup = DirectX::XMQuaternionMultiply(temp, updirect);

    updirect = DirectX::XMQuaternionMultiply(
        qup, DirectX::XMQuaternionConjugate(temp));

    auto left = DirectX::XMQuaternionMultiply(temp, lookdirect);
    lookdirect = DirectX::XMQuaternionMultiply(
        left, DirectX::XMQuaternionConjugate((temp)));
  }

  DirectX::XMStoreFloat4(&rotation, lookdirect);
  cPos.SetRotation(rotation);
  DirectX::XMStoreFloat4(&upDirection, updirect);
  cPos.SetUpDirection(upDirection);
}

CameraObject::CameraObject(Object &obj, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT4 initRot,
                           DirectX::XMFLOAT4 initUpDirection,
                           RenderedObject *link, bool Active)
    : Object(obj),
      isActive(Active), linkedObject(link), cPos(CameraPosition(initPos, initRot, initUpDirection))
      {
};

CameraObject::CameraObject(std::string name, UOID uOID,
                           std::function<void()> func,
                           Tracker::Instance *pInstance,
                           DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT4 initRot,
                           DirectX::XMFLOAT4 initUpDirection,
                           RenderedObject *link, bool Active)
    : Object(
          name, uOID, func, pInstance),
      isActive(Active), 
      linkedObject(link), cPos(CameraPosition(initPos, initRot, initUpDirection)) {
      LinkedInstance->MakeCamera(this, uOID);
};



RenderedObject::RenderedObject(std::string name, UOID UOID,
                               std::function<void()> func,
                               Tracker::Instance *pInstance,
                               RStorage::bmResource *model, umID filebModelIndex,
                               float mScale, DirectX::XMFLOAT3 initPos,
                               DirectX::XMFLOAT4 initRot)
    : Object(
          name, UOID, func, pInstance),
      model(model), scale(mScale),
      mPos(relposVect(initPos, initRot)) {
  LinkedInstance->LinkToModel(this, filebModelIndex);
};

PhysicsObject::PhysicsObject(std::string name, UOID UOID,
                             std::function<void()> func,
                             Tracker::Instance *pInstance,
                             RStorage::bmResource *model, umID filebModelIndex,
                             float mScale, DirectX::XMFLOAT3 initPos,
                             DirectX::XMFLOAT4 initRot, float mMass,
                             float mFriction, DirectX::XMFLOAT3 initVelDir,
                             float initSpeed)
    : RenderedObject(
          name, UOID, func, pInstance, model, filebModelIndex,
          mScale, initPos, initRot),
      mass(mMass), friction(mFriction), velDir(initVelDir), speed(initSpeed) {
  LinkedInstance->EnablePhysics(this, uOID);
};

//"Thread Safe" way to change position. Uses vector and direction to change
//position
void PhysicsObject::Move(DirectX::XMFLOAT4 Dir, float Dist)
{
	PhysicsUpdate.lock();
	using namespace DirectX;
	XMStoreFloat4(&pDir.emplace_back(XMFLOAT4{ 0,0,0,0 }), XMLoadFloat4(&Dir) * Dist);
	PhysicsUpdate.unlock();
}

void PhysicsObject::CollReset()
{
	PhysicsUpdate.lock();
	pDir.clear();
	PhysicsUpdate.unlock();
}

bool PhysicsObject::CollCheck()
{
	return false;
}

const DirectX::XMFLOAT3 relposVect::Move(DirectX::XMFLOAT3& NewPos){
  DirectX::XMFLOAT3 result;
  posMtx.lock();
  //auto OldestPosition = lastposition;
	lastposition = *position;
  result = lastposition;
	*position = NewPos;
	posMtx.unlock();
	return result;
}
const DirectX::XMFLOAT3 relposVect::Get(){
  DirectX::XMFLOAT3 result;
  posMtx.lock();
  result = *position;
  posMtx.unlock();
	return result;
}

const DirectX::XMFLOAT3 relposVect::GetLast(){
  DirectX::XMFLOAT3 result;
  posMtx.lock();
  result = lastposition;
  posMtx.unlock();
	return result;
}

const DirectX::XMFLOAT4 relposVect::GetRotation(){
  DirectX::XMFLOAT4 result;
  posMtx.lock();
  result = rotation;
  posMtx.unlock();
	return result;
}

void relposVect::SetRotation(DirectX::XMFLOAT4& Rotation){
  posMtx.lock();
  rotation = Rotation;
  posMtx.unlock();
}

void CameraPosition::SetUpDirection(DirectX::XMFLOAT4 UpDir){
  posMtx.lock();
  upDirection = UpDir;
  posMtx.unlock();
}
const DirectX::XMFLOAT4 CameraPosition::GetUpDirection(){
  
  posMtx.lock();
  auto upDir = upDirection;
  posMtx.unlock();
  return upDir;
}


void PhysicsObject::Update()
{
  Script();
	DirectX::XMFLOAT4 nDir{ 0,0,0,0 };
	PhysicsUpdate.lock();
	using namespace DirectX;
	std::for_each(pDir.begin(), pDir.end(), [&nDir](auto& x) {
		XMStoreFloat4(&nDir, XMLoadFloat4(&nDir) + XMLoadFloat4(&x));
	});
	PhysicsUpdate.unlock();

	using namespace DirectX;
  auto position = mPos.Get();
	XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat4(&nDir));
	CollReset();
	XMStoreFloat3(&position, XMLoadFloat3(&position) + XMLoadFloat3(&velDir) * (speed));
  mPos.Move(position);

}
