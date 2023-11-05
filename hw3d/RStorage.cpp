#include "RStorage.h"





RStorage::RStorage(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue) :
	pDevice(pDevice),
	commandList(commandList),
	commandQueue(commandQueue),
	commandAllocator(commandAllocator)
{
	OnInit();
}


void RStorage::OnInit() {
	std::vector<std::filesystem::path> folders = {};
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "models" }) {
		if (file.path().extension() == ".dae") {
			bmResource t{ file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string())) };
			t.model = file.path();
			Models.emplace_back(t);
		}
	}
	for (auto& file : std::filesystem::directory_iterator{ std::filesystem::current_path() / "textures" }) {
		if (file.path().extension() == ".dds") {
			auto name = file.path().filename().string().substr(0, file.path().filename().string().find(file.path().extension().string()));
			for (auto& m : Models) {
				if (m.name == name) {
					m.texture = file.path();
				}
			}
		}
	}
}

void RStorage::CreateBuffers(std::vector<bmResource*> bm) {

	for (auto& m : bm) {
		ReadX3D lfile = ReadX3D(m->model.string());
		m->vertexData = lfile.vertexData();
		m->indexData = lfile.indexData();
		m->fsize = lfile.fSize().fSize;
		m->vCount = lfile.fSize().vCount;
	}
	commandAllocator->Reset() >> chk;
	commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
	DirectX::ResourceUploadBatch upload(pDevice.Get());
	upload.Begin();
		for (auto& m : bm) {
			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m->fsize);
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
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m->fsize);
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
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(m->indexData)*sizeof(WORD));
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
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(std::size(m->indexData) * sizeof(WORD));
				pDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr, IID_PPV_ARGS(&m->uibuffer)
				) >> chk;
			}
			{
				CreateDDSTextureFromFile(pDevice.Get(), upload, m->texture.c_str(), m->tbuffer.GetAddressOf());
			}

			{
				ReadX3D::Vertex* mappedVertexData = nullptr;
				m->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
				WORD* mappedIndexData = nullptr;
				m->uibuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;
				
				
				for (auto i = 0; i < std::size(m->vertexData); i++) {
					memcpy(&mappedVertexData[i], &m->vertexData[i], sizeof(ReadX3D::Vertex));
					
				}
					
				for (auto i = 0; i < std::size(m->indexData); i++) {
					memcpy(&mappedIndexData[i], &m->indexData[i].index, sizeof(WORD));
				}
			}
			{
				m->uvbuffer->Unmap(0, nullptr);
				m->uibuffer->Unmap(0, nullptr);
				commandList->CopyResource(m->vbuffer.Get(), m->uvbuffer.Get());
				commandList->CopyResource(m->ibuffer.Get(), m->uibuffer.Get());
			}







			{
				const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m->vbuffer.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
				commandList->ResourceBarrier(1, &barrier);
			}
			{
				const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
					m->ibuffer.Get(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
				commandList->ResourceBarrier(1, &barrier);
			}
			
			
		}
		upload.End(commandQueue.Get());
		// close command list  
		commandList->Close() >> chk;
}

//Returns a model by its name
RStorage::bmResource* RStorage::Read(std::string name) {
	return lModel(name);
}

void RStorage::Delete(RStorage::bmResource* bm) {
	for (auto& m : modelVect) {
		modelVect.erase(std::remove(modelVect.begin(), modelVect.end(), bm), modelVect.end());
	}
}


RStorage::bmResource* RStorage::lModel(std::string name) noexcept
{
	for (auto& m : Models) {
		if (m.name == name) {
			return &m;
		}
	}
}


RStorage::~RStorage()
{
	for (auto& m : Models) {
		for (auto& bm : modelVect) {
			modelVect.erase(std::remove(modelVect.begin(), modelVect.end(), bm), modelVect.end());
		}
	}
}