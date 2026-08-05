#include "ObjectTracking.h"
#include "InputHandler.h"
#include "RStorage.h"
#include "ePhysics.h"
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
     FLOAT3 initPos, 
    FLOAT4 initRot, FLOAT4 initUpDirection, 
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
                    float mScale, FLOAT3 initPos,
                    FLOAT4 initRot, float mMass, float mFriction,
                    FLOAT3 initVelDir, float initSpeed) {

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
		float right = 0.0;
		bool movestop = false;
	};
  Movement move;

  //Make a helper function for this so I dont have to do it for every single key
  if(dispID.Empty.load()){
    dispID = LinkedInstance->pTracker.InputHndlr.linkEvent('J', [this]{
    ToggleFreedom();
  });
  }
  


  float pitch = 0;
  float yaw = 0;
  float roll = 0;
  {
    // Rotation per second in radians //Currently 90 degrees per second
    double rotationPS = ucontrolClock.Peek() * _DEGREES90;
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

  if (isFree()) {
    
      // Figure this out

      auto movespeed = 2.0 * ucontrolClock.Peek();
      if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('W')) {
        move.forward += movespeed;
      }
      if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('S')) {
        move.forward -= movespeed;
      }
      if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('D')) {
        move.right += movespeed;
      }
      if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('A')) {
        move.right -= movespeed;
      }
      if (LinkedInstance->pTracker.InputHndlr.keyboard.KeyIsPressed('K')) {
        move.movestop = true;
      }
  if (move.forward != 0 || move.right != 0) {
      auto Rotation = cPos.GetRotation();
      auto upDirection = cPos.GetUpDirection();
      FLOAT4 NewPos;
      NewPos = Rotation * (move.forward) + cPos.Get();
      NewPos = Rotation *
                   FLOAT4X4::Rotation(
                       FLOAT4::RotateQuaternion(upDirection, _DEGREES90)) *
                   (move.right) +
               NewPos;
      FLOAT3 mv = {NewPos.x, NewPos.y, NewPos.z};
      cPos.Move(mv);
  }
  } else if (GetLinked() && !isFree()) {
    FLOAT3 mv = GetLinked()->mPos.Get();
    cPos.Move(mv);
  }
  ucontrolClock.Mark();
  // Toggle Freecam

  auto position = cPos.Get();
  auto rotation = cPos.GetRotation();
  auto upDirection = cPos.GetUpDirection();
  cMatrix = LookTo(position*-1.f, rotation*-1.f, upDirection);
}

void CameraObject::LinkTo(RenderedObject *obj) {
  if (linkedObject) {
    linkedObject = nullptr;
  }
  linkedObject = obj;
}

void CameraObject::Rotate(float &Pitch, float &Yaw, float &Roll) {
  FLOAT4 upDirection = cPos.GetUpDirection();
  FLOAT4 rotation = cPos.GetRotation();

  // auto gravdirect =
  // DirectX::XMQuaternionInverse(XMLoadFloat4((XMFLOAT4*)&plModel->grav));
  if (Yaw != 0) {
    auto rotat = FLOAT4::RotateQuaternion(upDirection, Yaw);
    rotation = rotation.RotateByQuaternion(rotat).Real().Normal();
    auto upchang = rotat.QuaternionMul(upDirection);
    upDirection = upchang.RotateByQuaternion(upchang).Real().Normal();
  }
  
  if (Roll != 0) {
    auto rotat = FLOAT4::RotateQuaternion(rotation, Roll);
    auto upchang = rotat.QuaternionMul(upDirection);
    upDirection =  upchang.RotateByQuaternion(rotat).Real().Normal();
    rotation = rotation.RotateByQuaternion(rotat).Real().Normal();
  }
  if (Pitch != 0) {

    auto rotat = FLOAT4::RotateQuaternion(rotation.QuaternionMul(upDirection), Pitch).Normal();
    auto qup = rotat.QuaternionMul(upDirection);
    upDirection = qup.QuaternionMul(rotat.Conjugate()).Real().Normal();
    auto left = rotat.QuaternionMul(rotation);
    rotation = left.QuaternionMul(rotat.Conjugate()).Real().Normal();
  }
 //DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&rotation, lookdirect);
  cPos.SetRotation(rotation);
  //DirectX::XMStoreFloat4((DirectX::XMFLOAT4*)&upDirection, updirect);
  cPos.SetUpDirection(upDirection);
 
}

CameraObject::CameraObject(Object &obj, FLOAT3 initPos, FLOAT4 initRot,
                           FLOAT4 initUpDirection,
                           RenderedObject *link, bool Active)
    : Object(obj),
      isActive(Active), linkedObject(link), cPos(CameraPosition(initPos, initRot, initUpDirection))
      {
};

CameraObject::CameraObject(std::string name, UOID uOID,
                           std::function<void()> func,
                           Tracker::Instance *pInstance,
                           FLOAT3 initPos, FLOAT4 initRot,
                           FLOAT4 initUpDirection,
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
                               float mScale, FLOAT3 initPos,
                               FLOAT4 initRot)
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
                             float mScale, FLOAT3 initPos,
                             FLOAT4 initRot, float mMass,
                             float mFriction, FLOAT3 initVelDir,
                             float initSpeed)
    : RenderedObject(
          name, UOID, func, pInstance, model, filebModelIndex,
          mScale, initPos, initRot),
      mass(mMass), friction(mFriction), velDir(initVelDir), speed(initSpeed) {
  LinkedInstance->EnablePhysics(this, uOID);
};

//"Thread Safe" way to change position. Uses vector and direction to change
//position
void PhysicsObject::Move(FLOAT4 Dir, float Dist)
{
	PhysicsUpdate.lock();
	pDir.emplace_back(FLOAT3(Dir.x, Dir.y, Dir.z) * Dist);
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

const FLOAT3 relposVect::Move(FLOAT3& NewPos){
  FLOAT3 result;
  posMtx.lock();
  //auto OldestPosition = lastposition;
	lastposition = *position;
  result = lastposition;
	*position = NewPos;
	posMtx.unlock();
	return result;
}
const FLOAT3 relposVect::Get(){
  FLOAT3 result;
  posMtx.lock();
  result = *position;
  posMtx.unlock();
	return result;
}

const FLOAT3 relposVect::GetLast(){
  FLOAT3 result;
  posMtx.lock();
  result = lastposition;
  posMtx.unlock();
	return result;
}

const FLOAT4 relposVect::GetRotation(){
  FLOAT4 result;
  posMtx.lock();
  result = rotation;
  posMtx.unlock();
	return result;
}

void relposVect::SetRotation(FLOAT4& Rotation){
  posMtx.lock();
  rotation = Rotation;
  posMtx.unlock();
}

void CameraPosition::SetUpDirection(FLOAT4 UpDir){
  posMtx.lock();
  upDirection = UpDir;
  posMtx.unlock();
}
const FLOAT4 CameraPosition::GetUpDirection(){
  
  posMtx.lock();
  auto upDir = upDirection;
  posMtx.unlock();
  return upDir;
}


void PhysicsObject::Update()
{
  Script();
	FLOAT3 nDir{ 0,0,0 };
	PhysicsUpdate.lock();
	std::for_each(pDir.begin(), pDir.end(), [&nDir](auto& x) {
		nDir = nDir + x;
	});
	PhysicsUpdate.unlock();


  auto position = mPos.Get();
	position = position + nDir;
	CollReset();
	position = position + (velDir * speed);
  mPos.Move(position);

}
