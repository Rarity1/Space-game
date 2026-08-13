#include "ObjectTracking.h"
#include "InputHandler.h"
#include "RStorage.h"
#include "ePhysics.h"
#include <cstdint>
#include <iterator>
#include <memory>






Tracker::Instance::Instance(Tracker& Parent, std::function<void()> func):
InstanceObject(Parent.initOriginObject(func)),
uOID(InstanceObject->GetID()),
pTracker(Parent)
{};


Tracker::Instance &Tracker::initInstance(std::function<void()> func) {
  auto inst = std::make_unique<Instance>(*this, func);
  auto ID = inst->uOID;
  Instances.insert({ID, std::move(inst)});
  return *Instances[ID];
}

Tracker::Instance* Tracker::getInstance(UOID instanceID)
{
	if (!Instances.contains(instanceID)) {
		return nullptr;
	}
	return Instances[instanceID].get();
}
std::unordered_map<UOID, std::shared_ptr<Object>>& Tracker::Instance::GetInstanceObjects()
{
  return InstanceObject->GetChildren();
};

void Tracker::MakeInstanceActive(UOID id) {
  if (!isInstanceActive(id)) {
    ActiveInstances.insert({id, Instances[id]});
  }
}

void Tracker::Instance::MakeInstanceActive() {
  pTracker.MakeInstanceActive(uOID);
}

void Tracker::Instance::AddObject(std::shared_ptr<Object>& obj){
  obj->AddParent(InstanceObject);
}

std::shared_ptr<Object>& Tracker::initOriginObject(std::function<void()> func) {
  auto uOID = idTracker.AllocID();
  Objects.insert({uOID, std::make_shared<Object>("OriginObject", uOID, *this)});
  if (func) {
    Objects[uOID]->linkScript(func);
  }
  Objects[uOID]->isOrigin = true;
  return Objects[uOID];
}

std::shared_ptr<Object>& Tracker::initObject(std::string Name,
                            std::function<void()> func, Instance* oInstance) {
  auto uOID = idTracker.AllocID();
  Objects.insert({uOID, std::make_shared<Object>(Name, uOID, *this)});
  if (func) {
    Objects[uOID]->linkScript(func);
  }
  return Objects[uOID];
}

std::shared_ptr<Object>& Tracker::initCameraObject(std::string name, std::function<void()> func,
                                        FLOAT3 initPos, 
                                        FLOAT4 initRot,
                                        FLOAT4 initUpDirection,
                                        bool Active) {
  auto UOID = idTracker.AllocID();
  Objects.insert({UOID, std::make_unique<CameraObject>(
                                   name, UOID, *this, initPos, initRot,
                                   initUpDirection, Active)});
  CameraObjects.insert({UOID, Objects[UOID]});
  if (func) {
    Objects[UOID]->linkScript(func);
  }
  return Objects[UOID];
}

// Make objects able to be in multiple instances without loading new data
std::shared_ptr<Object>& Tracker::initPhysObject(std::string Name, std::function<void()> func,
                                       umID filebModelIndex, float mScale,
                                       FLOAT3 initPos, FLOAT4 initRot,
                                       float mMass, float mFriction,
                                       FLOAT3 initVelDir, float initSpeed) {
  auto UOID = idTracker.AllocID();
  Objects.insert({UOID, std::make_unique<PhysicsObject>(
                                   Name, UOID, *this, mScale, initPos, initRot, mMass,
                                   mFriction, initVelDir, initSpeed)});
  RenderObjects.insert({UOID, Objects[UOID]});
  PhysicsObjects.insert({UOID, Objects[UOID]});
  ((PhysicsObject*)Objects[UOID].get())->LinkToModel(filebModelIndex);
  if (func) {
    Objects[UOID]->linkScript(func);
  }
  return Objects[UOID];
}

void Tracker::MakeCameraObj(UOID uOID) {
  std::unique_ptr<CameraObject> old = std::make_unique<CameraObject>(*Objects[uOID]);
  Objects[uOID].reset(old.release());
  CameraObjects.insert({uOID, Objects[uOID]});
}

void Tracker::MakeRenderObj(UOID ID){
  std::unique_ptr<RenderedObject> old = std::make_unique<RenderedObject>(*Objects[ID]);
  Objects[ID].reset(old.release());
  RenderObjects.insert({ID, Objects[ID]});
}
//Will leak memory and probably cause issues if not removed from previous tracker.
void Tracker::MakePhysicsObj(UOID uOID) {
  if(!IsRendered(uOID)){
    MakeRenderObj(uOID);
  }
  std::unique_ptr<PhysicsObject> old = std::make_unique<PhysicsObject>(*(RenderedObject*)Objects[uOID].get());
  Objects[uOID].reset(old.release());
  PhysicsObjects.insert({uOID, Objects[uOID]});
}

CameraObject *Tracker::IsCamera(Object *Obj){return IsCamera(Obj->GetID());};
RenderedObject *Tracker::IsRendered(Object *Obj){return IsRendered(Obj->GetID());};
PhysicsObject *Tracker::IsPhysics(Object *Obj){return IsPhysics(Obj->GetID());};

// Really should validate if model ID is actually valid.
void Tracker::LinkModel(UOID ID, umID mID) {
  auto Object = IsRendered(ID);
  if (Object) {
    if (ViewBuffers.contains(mID)) {

      ViewBuffers[mID].ViewBuffLock.lock();
      Object->CBVIndex = ViewBuffers[mID].LinkedObjects.size();
      ViewBuffers[mID].LinkedObjects.emplace_back(Objects[ID]);
      Object->CBVRef = --ViewBuffers[mID].LinkedObjects.end();
      ViewBuffers[mID].vCount+=1;
      ViewBuffers[mID].ViewBuffLock.unlock();
    } else {
      ViewBuffers[mID].ViewBuffLock.lock();
      auto list =std::list{Objects[ID]};
      ViewBuffers.insert(std::pair{mID, ModelLinkedViewBuffers()});
      Object->CBVIndex = ViewBuffers[mID].LinkedObjects.size();
      ViewBuffers[mID].LinkedObjects.emplace_back(Objects[ID]);
      Object->CBVRef = --ViewBuffers[mID].LinkedObjects.end();
      ViewBuffers[mID].vCount+=1;
      ViewBuffers[mID].ViewBuffLock.unlock();
    }
  } else {
    if (Objects.contains(ID)) {
      MakeRenderObj(ID);
      LinkModel(ID, mID);
    }
  }
}

void Tracker::unLinkModel(UOID ID) {
  auto RenderObjPtr = IsRendered(ID);
  if (RenderObjPtr) {
    if (ViewBuffers.contains(RenderObjPtr->ModelID)) {
      ViewBuffers[RenderObjPtr->ModelID].ViewBuffLock.lock();
      ViewBuffers[RenderObjPtr->ModelID].LinkedObjects.erase(
          RenderObjPtr->CBVRef);
      ViewBuffers[RenderObjPtr->ModelID].vCount -= 1;
      ViewBuffers[RenderObjPtr->ModelID].FlaggedForUpdate = true;
      ViewBuffers[RenderObjPtr->ModelID].ViewBuffLock.unlock();
    }
  }
}

void Tracker::unloadObject(UOID obj)
{

}



//This will definitely leak memory if the previous link isnt removed
void RenderedObject::LinkToModel(umID mID) {
  if(hasModel.load()){
    pTracker.unLinkModel(uOID);
  }
  ModelID = mID;
  pTracker.LinkModel(uOID, mID);

}

void RenderedObject::loadModel() noexcept
{
  pTracker.loadModel(ModelID);
  hasModel.store(true);
}


Object::Object(std::string name, UOID uOID, Tracker& tracker)
    : name(name), uOID(uOID), pTracker(tracker) {};

void CameraObject::Update() {
  if(Script){
    Script();
  }

  // Toggle Freecam
	struct Movement {
		float forward = 0.0;
		float right = 0.0;
		bool movestop = false;
	};
  Movement move;

  //Make a helper function for this so I dont have to do it for every single key
  if(dispID.Empty.load()){
    dispID = pTracker.InputHndlr.linkEvent('J', [this]{
    ToggleFreedom();
  });
  }
  


  float pitch = 0;
  float yaw = 0;
  float roll = 0;
  {
    // Rotation per second in radians //Currently 90 degrees per second
    double rotationPS = ucontrolClock.Peek() * _DEGREES90;
    if (pTracker.InputHndlr.keyboard.KeyIsPressed(0x26)) {//up
      pitch += rotationPS;
    }
    if (pTracker.InputHndlr.keyboard.KeyIsPressed(0x28)) {//down
      pitch += -rotationPS;
    }
    if (pTracker.InputHndlr.keyboard.KeyIsPressed(0x25)) {//left
      yaw += -rotationPS;
    }
    if (pTracker.InputHndlr.keyboard.KeyIsPressed(0x27)) {//right
      yaw += rotationPS;
    }

    if (pTracker.InputHndlr.keyboard.KeyIsPressed('Q')) {
      roll += rotationPS;
    }
    if (pTracker.InputHndlr.keyboard.KeyIsPressed('E')) {
      roll += -rotationPS;
    }
  }

  if (pitch != 0 || yaw != 0 || roll != 0)
    Rotate(pitch, yaw, roll);

  if (isFree()) {
    
      // Figure this out

      auto movespeed = 2.0 * ucontrolClock.Peek();
      if (pTracker.InputHndlr.keyboard.KeyIsPressed('W')) {
        move.forward += movespeed;
      }
      if (pTracker.InputHndlr.keyboard.KeyIsPressed('S')) {
        move.forward -= movespeed;
      }
      if (pTracker.InputHndlr.keyboard.KeyIsPressed('D')) {
        move.right += movespeed;
      }
      if (pTracker.InputHndlr.keyboard.KeyIsPressed('A')) {
        move.right -= movespeed;
      }
      if (pTracker.InputHndlr.keyboard.KeyIsPressed('K')) {
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
    FLOAT3 mv = ((RenderedObject*)GetLinked().get())->mPos.Get();
    cPos.Move(mv);
  }
  ucontrolClock.Mark();
  // Toggle Freecam

  auto position = cPos.Get();
  auto rotation = cPos.GetRotation();
  auto upDirection = cPos.GetUpDirection();
  cMatrix = LookTo(position*-1.f, rotation*-1.f, upDirection);
}

void CameraObject::LinkTo(const std::shared_ptr<Object>& obj) {
  if (pTracker.IsRendered(obj->GetID()) ) {
      linkedObject = obj;
  }
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

CameraObject::CameraObject(Object& obj,
                           FLOAT3 initPos, FLOAT4 initRot,
                           FLOAT4 initUpDirection,
                           bool Active)
    : Object(obj),
      isActive(Active), 
     cPos(CameraPosition(initPos, initRot, initUpDirection)) {
};


CameraObject::CameraObject(std::string name, UOID uOID,
                          Tracker& tracker,
                           FLOAT3 initPos, FLOAT4 initRot,
                           FLOAT4 initUpDirection,
                            bool Active)
    : Object(name, uOID, tracker),
      isActive(Active), 
       cPos(CameraPosition(initPos, initRot, initUpDirection)) {
};


RenderedObject::RenderedObject(Object& old,
                               float mScale, FLOAT3 initPos,
                               FLOAT4 initRot)
    : Object(std::move(old)), scale(mScale),
      mPos(relposVect(initPos, initRot)) {};

RenderedObject::RenderedObject(std::string name, UOID UOID,
                                Tracker& tracker,
                               float mScale, FLOAT3 initPos,
                               FLOAT4 initRot)
    : Object(name, UOID, tracker),
       scale(mScale),
      mPos(relposVect(initPos, initRot)) {};

PhysicsObject::PhysicsObject(RenderedObject& obj,
                             float mScale, FLOAT3 initPos, FLOAT4 initRot,
                             float mMass, float mFriction, FLOAT3 initVelDir,
                             float initSpeed)
    : RenderedObject(obj),
      mass(mMass), friction(mFriction), velDir(initVelDir), speed(initSpeed) {};


PhysicsObject::PhysicsObject(std::string name, UOID UOID,
                             Tracker& tracker,
                             float mScale, FLOAT3 initPos, FLOAT4 initRot,
                             float mMass, float mFriction, FLOAT3 initVelDir,
                             float initSpeed)
    : RenderedObject(name, UOID, tracker, mScale,
                     initPos, initRot),
      mass(mMass), friction(mFriction), velDir(initVelDir), speed(initSpeed) {};

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
  if(Script){
  Script();
  }
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
