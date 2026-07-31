#include "ObjectTracking.h"
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


Tracker::Instance::Instance(InstID instanceID, Tracker& Parent):
instanceID(instanceID),
pTracker(Parent){};

RStorage::bmResource& Tracker::GetModel(umID umID)
{
	return storage.GetModel(umID);
}

Tracker::Instance& Tracker::initInstance(InstID instanceID)
{
  if(Instances.contains(instanceID)){
    return initInstance(instanceID + 1);
  }else{
    Instances.insert({instanceID, std::make_unique<Instance>(instanceID, *this)});
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

Object *Tracker::initObject(Instance &oInstance, std::string Name,
                            std::function<void()> func) {
  auto uOID = idTracker.AllocID();
  TrackedObjects.insert(
      {uOID, std::make_unique<Object>(Name, uOID, func, &oInstance)});
  return TrackedObjects[uOID].get();
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
                 Name, UOID, [] {}, &Instance, nullptr,
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

Object::Object(std::string name, UOID uOID, 
    std::function<void()> func, 
    Tracker::Instance* pInstance) : 
    name(name), uOID(uOID), ActiveFunction(func){
      pInstance->AddObject(this);
    };
    
RenderedObject::RenderedObject(Object &obj, RStorage::bmResource *model,
                               umID filebModelIndex, float mScale,
                               DirectX::XMFLOAT3 initPos,
                               DirectX::XMFLOAT4 initRot)
    : Object(obj), model(model), CBVIndex(LinkedInstance->ModelLinkedObjects[filebModelIndex].size()), scale(mScale),
      mPos(relposVect(initPos, initRot)) {
  LinkedInstance->ModelLinkedObjects[filebModelIndex].emplace_back(this);
  if(!LinkedInstance->instancedCBVData.contains(filebModelIndex))
  LinkedInstance->instancedCBVData.insert({filebModelIndex, nullptr});
};

RenderedObject::RenderedObject(std::string name, UOID UOID,
                               std::function<void()> func,
                               Tracker::Instance *pInstance,
                               RStorage::bmResource *model, umID filebModelIndex,
                               float mScale, DirectX::XMFLOAT3 initPos,
                               DirectX::XMFLOAT4 initRot)
    : Object(
          name, UOID, [func]() { func(); }, pInstance),
      model(model), CBVIndex(LinkedInstance->ModelLinkedObjects[filebModelIndex].size()), scale(mScale),
      mPos(relposVect(initPos, initRot)) {
  LinkedInstance->ModelLinkedObjects[filebModelIndex].emplace_back(this);
  if(!LinkedInstance->instancedCBVData.contains(filebModelIndex))
  LinkedInstance->instancedCBVData.insert({filebModelIndex, nullptr});
};

PhysicsObject::PhysicsObject(RenderedObject &obj, float mMass, float mFriction,
                             DirectX::XMFLOAT3 initVelDir, float initSpeed)
    : RenderedObject(obj), mass(mMass), friction(mFriction), velDir(initVelDir),
      speed(initSpeed) {
        LinkedInstance->PhysicsObjects.emplace_back(this);
        LinkedInstance->physEnabled.insert({obj.GetID(), true});
      };

PhysicsObject::PhysicsObject(std::string name, UOID UOID,
                             std::function<void()> func,
                             Tracker::Instance *pInstance,
                             RStorage::bmResource *model, 
                             umID filebModelIndex,
                             float mScale, DirectX::XMFLOAT3 initPos,
                             DirectX::XMFLOAT4 initRot, float mMass,
                             float mFriction, DirectX::XMFLOAT3 initVelDir,
                             float initSpeed)
    : RenderedObject(
          name, UOID, [func] { func(); }, pInstance, model, filebModelIndex, mScale, initPos,
          initRot),
      mass(mMass), friction(mFriction), velDir(initVelDir), speed(initSpeed) {
        LinkedInstance->PhysicsObjects.emplace_back(this);
        LinkedInstance->physEnabled.insert({UOID, true});
      };

      //"Thread Safe" way to change position. Uses vector and direction to change position
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

const DirectX::XMFLOAT3& relposVect::Move(DirectX::XMFLOAT3& NewPos){
  using namespace DirectX;
  posMtx.lock();
  //auto OldestPosition = lastposition;
	lastposition = *position;
	*position = NewPos;
	posMtx.unlock();
	return lastposition;
}
const DirectX::XMFLOAT3& relposVect::Get(){
	return *position;
}
const DirectX::XMFLOAT4& relposVect::GetRotation(){
	return rotation;
}
DirectX::XMFLOAT3 PhysicsObject::UpdatePosition()
{
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
  auto result = mPos.Move(position);
	return result;
}
