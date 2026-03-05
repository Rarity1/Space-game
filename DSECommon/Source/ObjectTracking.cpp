#include "ObjectTracking.h"




//"Thread Safe" way to change position. Uses vector and direction to change position
void Object::Move(DirectX::XMFLOAT4 Dir, float Dist)
{
	PhysicsUpdate.lock();
	using namespace DirectX;
	XMStoreFloat4(&pDir.emplace_back(XMFLOAT4{ 0,0,0,0 }), XMLoadFloat4(&Dir) * Dist);
	PhysicsUpdate.unlock();
}

void Object::CollReset()
{
	PhysicsUpdate.lock();
	pDir.clear();
	PhysicsUpdate.unlock();
}

bool Object::CollCheck()
{
	return false;
}

DirectX::XMFLOAT3 Object::UpdatePosition()
{
	DirectX::XMFLOAT4 nDir{ 0,0,0,0 };


	PhysicsUpdate.lock();
	using namespace DirectX;
	std::for_each(Object::pDir.begin(), pDir.end(), [this, &nDir](auto& x) {
		XMStoreFloat4(&nDir, XMLoadFloat4(&nDir) + XMLoadFloat4(&x));
	});
	PhysicsUpdate.unlock();




	using namespace DirectX;
	mPos.posMtx.lock();
	mPos.lastposition = *mPos.position;
	XMStoreFloat3(mPos.position.get(), XMLoadFloat3(mPos.position.get()) + XMLoadFloat4(&nDir));
	CollReset();
	XMStoreFloat3(mPos.position.get(), XMLoadFloat3(mPos.position.get()) + XMLoadFloat4(&velDir) * (speed));
	mPos.posMtx.unlock();

	return mPos.lastposition;
}


void Tracker::lModel(Object& obj, UINT umID) noexcept
{
	obj.model = storage->loadModel(umID);
	obj.model->curTexture = storage->getTexture(obj.model->name);
	obj.loadedModel = true;
}




RStorage::bmResource* Tracker::GetModel(UINT umID)
{
	return storage->GetModel(umID);
}

Tracker::InstanceStruc& Tracker::initInstance(uint16_t instanceID)
{
	objectInstances[instanceID] = Tracker::InstanceStruc{};
	objectInstances[instanceID].pTracker = this;
	return objectInstances[instanceID];
}

Tracker::InstanceStruc& Tracker::getInstance(uint16_t instanceID)
{
	if (objectInstances.find(instanceID) == objectInstances.end()) {
		return initInstance(instanceID);
	}
	return objectInstances[instanceID];
}

//Need to be able to init an object not connected to an instance so that one object can be on multiple instances
//also try to seperate object tracking between rendering and physics calculations
Object* Tracker::initObject(InstanceStruc& oInstance, std::string textureName, UINT filebModelIndex, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed) {

	auto UOID = idTracker->AllocID();

	mapUOID[UOID] = std::make_unique<Object>(textureName,
		nullptr,
		mScale, mMass, mFriction, initPos, initRot, initVelDir, initSpeed);
	mapUOID[UOID]->UOID = UOID;

	mapUOID[UOID]->CBVIndex = oInstance.tmodelLinkedObjects[filebModelIndex].size();
	auto& result = oInstance.tmodelLinkedObjects[filebModelIndex].emplace_back(mapUOID[UOID].get());
	oInstance.instancedCBVData[filebModelIndex] = nullptr;

	oInstance.Count++;
	return result;

}
void Tracker::unloadObject(uint64_t obj)
{

}
