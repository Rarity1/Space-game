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
	//READ ABOVE
	//std::thread([this]() {
	PhysicsUpdate.lock();
	pDir.clear();
	PhysicsUpdate.unlock();
	//}).detach();
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
	XMStoreFloat3(mPos.position, XMLoadFloat3(mPos.position) + XMLoadFloat4(&nDir));
	CollReset();
	XMStoreFloat3(mPos.position, XMLoadFloat3(mPos.position) + XMLoadFloat4(&velDir) * (speed));
	mPos.posMtx.unlock();

	viewMtx.lock();
	XMStoreFloat4x4(&vMatrix, XMMatrixRotationQuaternion(XMLoadFloat4(&mPos.rotation)) * XMMatrixTranslation(mPos.position->x, mPos.position->y, mPos.position->z));
	viewMtx.unlock();
	return mPos.lastposition;
}

UINT Tracker::getModelID(uint64_t oID)
{

	return storage.getModelID(oID);
}

std::list<Object>& Tracker::initInstance(uint16_t instanceID)
{
	objectInstances.insert({ instanceID,  std::list<Object>()});
	return objectInstances[instanceID];
}

std::list<Object>& Tracker::getInstance(uint16_t instanceID)
{
	if (objectInstances.find(instanceID) == objectInstances.end()) {
		return initInstance(instanceID);
	}
	return objectInstances[instanceID];
}

Object* Tracker::initObject(std::list<Object>* oInstance, std::string textureName, UINT filebModelIndex, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed) {

//Impliment object tracking for real. Track per world(Doesnt have to be entire planet just local zeropoint.)
	auto& result = oInstance->emplace_back(Object(textureName,
		//Move this elsewhere.
		nullptr,
		mScale, mMass, mFriction, initPos, initRot, initVelDir, initSpeed));
	objCounter[filebModelIndex].emplace_back(&result);
	{
		auto UIOD = lastUOIDused.load();
		storage.trackModelID(filebModelIndex, UIOD);
		result.UOID = UIOD;
	}
	mapUOID[result.UOID] = &result;
	lastUOIDused++;
	return &result;

}
void Tracker::unloadObject(uint64_t obj)
{

}
