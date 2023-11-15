#include "RStorage.h"





RStorage::RStorage()
{
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
}

void RStorage::CreateBuffers(std::vector<bmResource*> bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, UINT buffercount) {

	commandAllocator->Reset() >> chk;
	commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
	DirectX::ResourceUploadBatch upload(pDevice.Get());
	upload.Begin();
	for (auto& m : bm) {
		if (!Models[m->umID].mappedBuffer) {
		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m->uData->fSize().fSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m->vbuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m->uData->fSize().fSize);
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&m->uvbuffer)
			) >> chk;
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(m->uData->indexData()) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&m->ibuffer));
		}

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(m->uData->indexData()) * sizeof(WORD));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr, IID_PPV_ARGS(&m->uibuffer)
			) >> chk;
		}
		{
			CreateDDSTextureFromFile(pDevice.Get(), upload, Models[m->umID].texture.c_str(), &m->tbuffer);
		}
		{
			ReadX3D::Vertex* mappedVertexData = nullptr;
			m->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
			WORD* mappedIndexData = nullptr;
			m->uibuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;


			auto temporaryVertex = m->uData->vertexData();
			for (auto i = 0; i < std::size(temporaryVertex); i++) {
				memcpy(&mappedVertexData[i], &temporaryVertex[i], sizeof(ReadX3D::Vertex));

			}

			auto temporaryIndex = m->uData->indexData();
			for (auto i = 0; i < std::size(temporaryIndex); i++) {
				memcpy(&mappedIndexData[i], &temporaryIndex[i].index, sizeof(WORD));
			}
		}
		{
			m->uvbuffer->Unmap(0, nullptr);
			m->uibuffer->Unmap(0, nullptr);
			commandList->CopyResource(m->vbuffer, m->uvbuffer);
			commandList->CopyResource(m->ibuffer, m->uibuffer);
		}



		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m->vbuffer,
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			commandList->ResourceBarrier(1, &barrier);
		}
		{
			const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
				m->ibuffer,
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			commandList->ResourceBarrier(1, &barrier);
		}
		Models[m->umID].mappedBuffer = true;
		Models[m->umID].vbuffer = m->vbuffer;
		Models[m->umID].ibuffer = m->ibuffer;
		Models[m->umID].tbuffer = m->tbuffer;
		}
		else {
			m->vbuffer = Models[m->umID].vbuffer;
			m->ibuffer = Models[m->umID].ibuffer;
			m->tbuffer = Models[m->umID].tbuffer;
		}
		}
		upload.End(commandQueue.Get());
		// close command list  
		commandList->Close() >> chk;

		
}

//Returns a model by its name
void RStorage::Delete(RStorage::bmResource* bm) {
	//modelVect.erase(std::remove(modelVect.begin(), modelVect.end(), bm), modelVect.end());
}


void RStorage::lModel(UINT umID, RStorage::bmResource* model) noexcept
{
			
	model->uData = new ReadX3D(Models[umID].model.string());
	modelVect.emplace_back(model);
}


RStorage::~RStorage()
{
		for (auto& bm : modelVect) {
			Delete(bm);
		}
}