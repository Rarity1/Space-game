#include "RStorage.h"





RStorage::RStorage(){
	std::vector<std::filesystem::path> folders = {};
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "models" }) {
		if (file.path().extension() == ".dae") {
			auto t = new unmappedData;
			t->model = file.path();
			bool toggle = false;
			auto name = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
			std::vector<char> UniqueID(1, '0');
			for (auto& c : name) {
				if (c == '#') {
					toggle = !toggle ? true : false;
				}
				if (toggle && c != '#') {
					UniqueID.emplace_back(c);
				}
				if (!toggle && c != '#') {
					t->name = t->name + c;
				}

			}
			t->umID = std::stoull((std::string)UniqueID.data());
			Models.emplace_back(t);
		}
	}
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "textures" }) {
		if (file.path().extension() == ".dds") {
			Textures.emplace_back(file.path());
		}
	}
}



void RStorage::CreateBuffers(std::vector<eResource>& m, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, UINT buffercount) {

	commandAllocator->Reset() >> chk;
	commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
	DirectX::ResourceUploadBatch upload(pDevice.Get());
	upload.Begin();
	for (auto& bm : m) {
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm.model->uData->idata) * sizeof(ReadX3D::Vertex));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				nullptr,
				IID_PPV_ARGS(&bm.model->vbuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm.model->uData->idata) * sizeof(ReadX3D::Vertex));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&bm.model->uvbuffer)
			) >> chk;
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm.model->uData->idata) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_INDEX_BUFFER,
				nullptr,
				IID_PPV_ARGS(&bm.model->ibuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm.model->uData->idata) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&bm.model->uibuffer)
			) >> chk;
		}
		{
			{
				ReadX3D::Vertex* mappedVertexData = nullptr;
				bm.model->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
				WORD* mappedIndexData = nullptr;
				bm.model->uibuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;


				//auto &temporaryVertex = bm.model->uData->MappedVertices;
				auto &temporaryIndex = bm.model->uData->idata;
				auto& map = bm.model->uData->MappedVertices;
				//For some reason models only render correctly if the indices are inverted?
				for (int i = 0; i < std::size(temporaryIndex); i++) {
					WORD index = std::size(temporaryIndex) - 1 - i;
					memcpy(&mappedVertexData[i], &map[temporaryIndex[i].index][i%3], sizeof(ReadX3D::Vertex));
					memcpy(&mappedIndexData[i], &index, sizeof(WORD));
				}
			}
			bm.model->uvbuffer->Unmap(0, nullptr);
			bm.model->uibuffer->Unmap(0, nullptr);
			

			//BC7 Textures dont work in release mode ??? wrld texture doesnt work in release mode ??????
			CreateDDSTextureFromFile(pDevice.Get(), upload, bm.curTexture.wstring().c_str(), bm.tbuffer.GetAddressOf())>>chk;
			UpdBuffer(bm, commandList, pDevice, commandAllocator, commandQueue);
		}
		
	}
	upload.End(commandQueue.Get());
 
	commandList->Close() >> chk;
		
}

void RStorage::UpdBuffer(eResource& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue) {
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->vbuffer.Get(),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->ibuffer.Get(),
			D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	commandList->CopyResource(bm.model->vbuffer.Get(), bm.model->uvbuffer.Get());
	commandList->CopyResource(bm.model->ibuffer.Get(), bm.model->uibuffer.Get());
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->vbuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->ibuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
}

 RStorage::bmResource* RStorage::lModel(UINT umID) noexcept
{
	 unmappedData* uData = findUm(umID);
	if (uData != nullptr) {
		auto ReadData = CheckLoaded(umID);
		if (ReadData == nullptr) {
			ReadData = new ReadX3D{ uData->model.string() };
		}
		auto model = new RStorage::bmResource{ umID, ReadData, uData->name };
		return model;
	}
	return nullptr;
}





 //Seperate this into initializing objects and initializing resources. Positions and other data is unreleated to model and texture data. Something can be known about without being loaded into memory.0
RStorage::eResource* RStorage::initObject(std::string textureName, int filebModelIndex, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed) {


	auto& result = trackedObjects.emplace_back(RStorage::eResource(textureName,
			//Move this elsewhere.
			RStorage::lModel(filebModelIndex),
			mScale, mMass, mFriction, initPos, initRot, initVelDir, initSpeed));

	auto curtex = std::find_if(Textures.begin(), Textures.end(), [textureName](auto e) {
		return e.filename().string().substr(0, e.filename().string().find(e.extension().string())) == textureName; });
	result.curTexture = curtex != Textures.end() ? *curtex : L"reee";

	return &result;
}

ReadX3D* RStorage::CheckLoaded(int umID) {
	ReadX3D* result = nullptr;
	unmappedData* uData = findUm(umID);
	if (uData != nullptr) 
		if (uData->lModel != nullptr)
			result = uData->lModel;
	
	return result;
}

RStorage::unmappedData* RStorage::findUm(UINT umID)
{
	auto test = [umID](const RStorage::unmappedData* UDat) {
		return UDat->umID == umID;
	};
	auto result = std::find_if(Models.begin(), Models.end(), test);

	return result != Models.end() ? *result : nullptr;
}


RStorage::eResource::eResource(std::string name, RStorage::bmResource* model, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed)
	:
	name(name),
	model(model),
	scale(mScale),
	mass(mMass),
	friction(mFriction),
	velDir(),
	speed(),
	currentMtx(*new std::mutex),
	PhysicsUpdate(*new std::mutex),
	Filled(*new std::atomic<bool>),
	updated(*new std::atomic<bool>),
	Collision(*new std::atomic<bool>),
	mPos(*new relposVect(initPos))
{

}


RStorage::~RStorage()
{
	for (auto& m : Models) {
		if (m->lModel != nullptr)
			delete m->lModel;
		delete m;
	}
	trackedObjects.clear();
}

void RStorage::eResource::CollisionUp(DirectX::XMFLOAT4 Dir, float Dist)
{
	std::thread([this, Dir, Dist]() {
		this->Filled.store(true);
		RStorage::eResource::PhysicsUpdate.lock();
		using namespace DirectX;
		XMFLOAT4 TempDir{ 0,0,0,0 };
		XMStoreFloat4(&TempDir, XMLoadFloat4(&Dir) * Dist);
		RStorage::eResource::pDir.emplace_back(TempDir);
		RStorage::eResource::PhysicsUpdate.unlock();
	}).detach();
}

void RStorage::eResource::CollReset()
{
	std::thread([this]() {
		RStorage::eResource::PhysicsUpdate.lock();
		RStorage::eResource::pDir.clear();
		RStorage::eResource::PhysicsUpdate.unlock();
		this->Filled.store(false);
	}).detach();
}

bool RStorage::eResource::CollCheck()
{
	return this->Filled.load();
}

DirectX::XMFLOAT4 RStorage::eResource::CollDir(pChange Which, int Index)
{
	DirectX::XMFLOAT4 Result{0,0,0,0};
	switch (Which) {
	case ALL:
			RStorage::eResource::PhysicsUpdate.lock();
			using namespace DirectX;
			std::for_each(RStorage::eResource::pDir.begin(), RStorage::eResource::pDir.end(), [this, &Result](auto x){
				XMStoreFloat4(&Result, XMLoadFloat4(&Result) + XMLoadFloat4(&x));
			});
			RStorage::eResource::PhysicsUpdate.unlock();
		break;
	case ONE:
		RStorage::eResource::PhysicsUpdate.lock();
		using namespace DirectX;

		XMStoreFloat4(&Result, XMLoadFloat4(&Result) + XMLoadFloat4(&RStorage::eResource::pDir[Index]));

		RStorage::eResource::PhysicsUpdate.unlock();
		break;

	}
	return Result;
}

RStorage::relposVect::relposVect(DirectX::XMFLOAT3 initPos):
	position(new DirectX::XMFLOAT3(initPos)),
	posMtx(*new std::mutex)
{

}

RStorage::relposVect::~relposVect()
{
	delete position;
}

