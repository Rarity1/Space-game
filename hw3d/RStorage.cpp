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
}

void RStorage::CreateBuffers(std::vector<bmResource*> m, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, UINT buffercount) {

	commandAllocator->Reset() >> chk;
	commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
	DirectX::ResourceUploadBatch upload(pDevice.Get());
	upload.Begin();
	for(auto& bm : m)
	if (!Models[bm->umID].mappedBuffer) {
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bm->uData->fsize.fSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
				nullptr,
				IID_PPV_ARGS(&bm->vbuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(bm->uData->fsize.fSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&bm->uvbuffer)
			) >> chk;
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm->uData->idata) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_INDEX_BUFFER,
				nullptr,
				IID_PPV_ARGS(&bm->ibuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(bm->uData->idata) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&bm->uibuffer)
			) >> chk;
		}
		{
			{
				ReadX3D::Vertex* mappedVertexData = nullptr;
				bm->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
				WORD* mappedIndexData = nullptr;
				bm->uibuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;


				auto& temporaryVertex = bm->uData->cdata;
				auto& idata = bm->uData->idata;
				auto tempcount = 0;
				for (auto i = 0; i < std::size(temporaryVertex); i++) {
					memcpy(&mappedVertexData[i], &temporaryVertex[tempcount].verts[idata[i].index % 3], sizeof(ReadX3D::Vertex));
					tempcount += idata[i].index % 3 == 0 ? 1 : 0;
				}

				auto& temporaryIndex = bm->uData->idata;
				for (auto i = 0; i < std::size(temporaryIndex); i++) {
					memcpy(&mappedIndexData[i], &temporaryIndex[i].index, sizeof(WORD));
				}
			}
			bm->uvbuffer->Unmap(0, nullptr);
			bm->uibuffer->Unmap(0, nullptr);
			CreateDDSTextureFromFile(pDevice.Get(), upload, Models[bm->umID].texture.c_str(), &bm->tbuffer);
			UpdBuffer(bm, commandList, pDevice, commandAllocator, commandQueue);
		}
		Models[bm->umID].mappedBuffer = true;
		Models[bm->umID].vbuffer = bm->vbuffer;
		Models[bm->umID].ibuffer = bm->ibuffer;
		Models[bm->umID].tbuffer = bm->tbuffer;
		Models[bm->umID].uvbuffer = bm->uvbuffer;

		}
		else {
			bm->vbuffer = Models[bm->umID].vbuffer;
			bm->ibuffer = Models[bm->umID].ibuffer;
			bm->tbuffer = Models[bm->umID].tbuffer;
			bm->uvbuffer = Models[bm->umID].uvbuffer;
		}
		
		upload.End(commandQueue.Get());
 
		commandList->Close() >> chk;
		
}

void RStorage::UpdBuffer(bmResource* bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue) {
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->vbuffer,
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->ibuffer,
			D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	commandList->CopyResource(bm->vbuffer, bm->uvbuffer);
	if(bm->uibuffer != nullptr)
	commandList->CopyResource(bm->ibuffer, bm->uibuffer);
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->vbuffer,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm->ibuffer,
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
	bm->animate.store(false);
}

void RStorage::Delete(RStorage::bmResource* bm) {
	delete bm->uData;
	modelVect.erase(std::remove(modelVect.begin(), modelVect.end(), bm), modelVect.end());
}


RStorage::bmResource* RStorage::lModel(UINT umID) noexcept
{
	
	if (umID < (std::size(Models))) {
		auto model = new RStorage::bmResource{ umID, CheckLoaded(umID) };
		modelVect.emplace_back(model);
		return model;
	}
	return nullptr;
}

ReadX3D* RStorage::CheckLoaded(int umID) {
	if(loadedModels[umID] == nullptr)
		loadedModels[umID] = new ReadX3D(Models[umID].model.string());
	return loadedModels[umID];
}


RStorage::~RStorage()
{
}