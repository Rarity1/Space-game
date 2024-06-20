#include "RStorage.h"





RStorage::RStorage():
	Models({})
{
	OnInit();
}


void RStorage::OnInit() {
	std::vector<std::filesystem::path> folders = {};
	auto temp = 0;
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "models" }) {
		if (file.path().extension() == ".dae") {
			unmappedData t{};
			t.model = file.path();
			t.texture = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
			t.umID = temp;
			t.mappedBuffer = false;
			temp++;
			Models.emplace_back(t);
		}
	}
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "textures" }) {
		if (file.path().extension() == ".dds") {
			auto name = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
			for (auto& m : Models) {
				if (m.texture == name) {
					m.texture = file.path();
				}
			}
		}
	}
	loadedModels.resize(std::size(Models));
	TrackedPtrs.resize(std::size(Models));

}


void RStorage::CreateBuffers(std::vector<eResource*>& m, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, UINT buffercount) {

	commandAllocator->Reset() >> chk;
	commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
	DirectX::ResourceUploadBatch upload(pDevice.Get());
	upload.Begin();
	for(auto& bm : m)
	if (!Models[bm->model->umID].mappedBuffer) {
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bm->model->uData->fsize.fSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				nullptr,
				IID_PPV_ARGS(&bm->model->vbuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bm->model->uData->fsize.fSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&bm->model->uvbuffer)
			) >> chk;
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm->model->uData->idata) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_INDEX_BUFFER,
				nullptr,
				IID_PPV_ARGS(&bm->model->ibuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm->model->uData->idata) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&bm->model->uibuffer)
			) >> chk;
		}
		{
			{
				ReadX3D::Vertex* mappedVertexData = nullptr;
				bm->model->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
				WORD* mappedIndexData = nullptr;
				bm->model->uibuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;


				auto& temporaryVertex = bm->model->uData->cdata;
				auto& idata = bm->model->uData->idata;
				auto tempcount = 0;
				for (auto i = 0; i < std::size(temporaryVertex); i++) {
					memcpy(&mappedVertexData[i], &temporaryVertex[tempcount].verts[idata[i].index % 3], sizeof(ReadX3D::Vertex));
					tempcount += idata[i].index % 3 == 0 ? 1 : 0;
				}

				auto& temporaryIndex = bm->model->uData->idata;
				for (auto i = 0; i < std::size(temporaryIndex); i++) {
					memcpy(&mappedIndexData[i], &temporaryIndex[i].index, sizeof(WORD));
				}
			}
			bm->model->uvbuffer->Unmap(0, nullptr);
			bm->model->uibuffer->Unmap(0, nullptr);
			CreateDDSTextureFromFile(pDevice.Get(), upload, Models[bm->model->umID].texture.c_str(), &bm->model->tbuffer);
			UpdBuffer(bm, commandList, pDevice, commandAllocator, commandQueue);
		}
		Models[bm->model->umID].mappedBuffer = true;
		Models[bm->model->umID].vbuffer = bm->model->vbuffer;
		Models[bm->model->umID].ibuffer = bm->model->ibuffer;
		Models[bm->model->umID].tbuffer = bm->model->tbuffer;
		Models[bm->model->umID].uvbuffer = bm->model->uvbuffer;

		}
		else {
			bm->model->vbuffer = Models[bm->model->umID].vbuffer;
			bm->model->ibuffer = Models[bm->model->umID].ibuffer;
			bm->model->tbuffer = Models[bm->model->umID].tbuffer;
			bm->model->uvbuffer = Models[bm->model->umID].uvbuffer;
		}
		
		upload.End(commandQueue.Get());
 
		commandList->Close() >> chk;
		
}

void RStorage::UpdBuffer(eResource* bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue) {
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->model->vbuffer,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->model->ibuffer,
			D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	commandList->CopyResource(bm->model->vbuffer, bm->model->uvbuffer);
	if(bm->model->uibuffer != nullptr)
	commandList->CopyResource(bm->model->ibuffer, bm->model->uibuffer);
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->model->vbuffer,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->model->ibuffer,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
}




 RStorage::bmResource* RStorage::lModel(UINT umID) noexcept
{
	if (umID < (std::size(this->Models)) && !umID <= 0 || umID == 0) {
		auto model = new RStorage::bmResource{ umID, CheckLoaded(umID) };
		TrackedPtrs[umID] = model;
		return model;
	}
	return nullptr;
}

RStorage::eResource* RStorage::initResource(char name, int filebModelIndex, float mScale, float mMass, float mFriction, DirectX::XMFLOAT3 initPos, DirectX::XMFLOAT3 initRot, DirectX::XMFLOAT3 initVelDir, float initSpeed) {
	auto& farb = initializedModels.emplace_back(new RStorage::eResource{ std::string(&name), RStorage::lModel(filebModelIndex), mScale,mMass, mFriction, new DirectX::XMFLOAT3{initPos} });

	return farb;

}

ReadX3D* RStorage::CheckLoaded(int umID) {
	if(loadedModels[umID] == nullptr)
		loadedModels[umID] = new ReadX3D(Models[umID].model.string());
	return loadedModels[umID];
}


RStorage::~RStorage()
{
	for (auto& l : loadedModels) {
		delete l;
	}
	for (auto& t : TrackedPtrs) {
		if(t != nullptr)
		delete t;
	}
	for (auto& m : initializedModels) {
		delete m->mPos.position;
		delete m;
	}
}