#pragma once
#include "CWin.h"
#include "FrameResource.h"
#include "EngineTime.h"
#include "ObjectTracking.h"
#include "../ImGui/DLLGui.h"
#include <dxcapi.h>
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>


class DLL Graphics
{
	friend class Window;
	friend class FrameResource;
	friend class Engine;
public:
	struct thRect {
		RECT wr;
		std::mutex Mtx;
	};
	Graphics(HWND& hWnd, thRect& WindowRect);
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	~Graphics();
	struct pCamera {
		DirectX::XMFLOAT3* position = nullptr;
		std::mutex* posMtx;
		DirectX::XMFLOAT4 rotation = {1,0,0,0};
		DirectX::XMFLOAT4 upDirection = { 0,0,1,0 };
		DirectX::XMFLOAT4 forwardDirect = { 1,0,0,0 };
		DirectX::XMMATRIX cmatrix;
	};
	//update graphics for a list of tracked objects. Preferably objects loaded in memory and meant to be rendered
	void Update(Tracker::InstanceStruc& tInstance);


	void CreateBuffers(Tracker::InstanceStruc& tInstance);
	void UpdBuffer(RStorage::bmResource& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);

	void lModel(Object& obj, UINT umID) noexcept;


	void UpdateModel(Object* bm);
	void RenderFrame(Tracker::InstanceStruc& tInstance);
	void LoadResources(Tracker::InstanceStruc& tInstance);
	void LoadPipeline();
	void UpdateLocalTransform(Object& bm);
	static void UpdateConstantBuffers(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection, Tracker::InstanceStruc& tInstance);
private:
	const UINT bufferCount = 3;
	std::unique_ptr<imguid> iGui;
	pCamera curCamera;
	std::vector<std::string> loadbuff;
	std::unique_ptr<RStorage> rStorage;
	std::atomic<bool> updateResolution;
	void UpdateFrameResources();
	void CreateFrameResources();
	std::condition_variable uFrameResource;
	std::mutex frMutex;
	ImGui_ImplDX12_InitInfo ImGuiInfo;
	DirectX::XMFLOAT4X4 fovPerspective;
	float Max(float number, float maximum);
	float Min(float minimum, float number);
	float RotateHelper(float& rNumber);
	float timesincestart;
	void RecurLTrans(ReadXML::Node* n, ReadXML::Node* P);
	GErrors::CheckerToken chk;
	//uint16_t& width;
	//uint16_t& height;

	thRect& windowResolution;
	HWND& hwnd;
	CD3DX12_RECT scissorRect;
	CD3DX12_VIEWPORT viewport;
	static const bool UseBundles = true;
	std::vector<std::unique_ptr<FrameResource>> FrameResources;
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;
	uint8_t cframeIndex;
	UINT rtvDescriptorSize;
	UINT dsvDescriptorSize;

	
	uint8_t fenceValue = 0;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;
	Microsoft::WRL::ComPtr<ID3D12Device9> pDevice;
	Microsoft::WRL::ComPtr<IDStorageFactory> pStorage;
	Microsoft::WRL::ComPtr<IDStorageQueue> storageQueue;
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSamplerDescriptorHeap;
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets;


	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	std::vector<std::unique_ptr<FrameResource>> backBuffers;
	std::vector<D3D12_VERTEX_BUFFER_VIEW*> vbvarr;
	std::vector<D3D12_INDEX_BUFFER_VIEW*> ibvarr;
	

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState;

	UINT modelSubCount;

	HANDLE fenceEvent;

	EngineTime timer;



	class DescriptorHeapAllocator
	{
	public:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
		D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
		D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
		struct handls {
			D3D12_CPU_DESCRIPTOR_HANDLE* cpuHndl;
			D3D12_GPU_DESCRIPTOR_HANDLE* gpuHndl;
			//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> originHeap;
		};
		//std::unordered_map<uint64_t, handls> allocatedHandles;
		std::unordered_map<SIZE_T, uint64_t> uidHndls;

		Microsoft::WRL::ComPtr<ID3D12Device9> pDevice;
		uint64_t HeapHandleIncrement;
		std::vector<uint64_t> FreeIndices;
		uint64_t lastSize = 0;
		DescriptorHeapAllocator(Microsoft::WRL::ComPtr<ID3D12Device9>& pD, uint64_t DescpCount = 1000000):
			pDevice(pD)
		{
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				desc.NumDescriptors = DescpCount;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&DescHeap));
			}

			D3D12_DESCRIPTOR_HEAP_DESC desc = DescHeap->GetDesc();
			HeapType = desc.Type;
			HeapStartCpu = DescHeap->GetCPUDescriptorHandleForHeapStart();
			HeapStartGpu = DescHeap->GetGPUDescriptorHandleForHeapStart();
			HeapHandleIncrement = pDevice->GetDescriptorHandleIncrementSize(HeapType);
			FreeIndices.reserve(desc.NumDescriptors);
			FreeIndices.resize(desc.NumDescriptors);
			std::iota(FreeIndices.begin(), FreeIndices.end(), 0);
			std::reverse(FreeIndices.begin(), FreeIndices.end());
			lastSize = desc.NumDescriptors;
		}
		~DescriptorHeapAllocator() {
			Destroy();
		}


		void Destroy()
		{
			DescHeap = nullptr;
			FreeIndices.clear();
		}
		void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
		{
			uint64_t idx = FreeIndices.back();
			FreeIndices.pop_back();
			out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
			out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);

			//Is a map faster than division?
			//uidHndls[out_cpu_desc_handle->ptr] = idx;
			//allocatedHandles[idx] = { out_cpu_desc_handle , out_gpu_desc_handle};

		}
		void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
		{
			//Map faster or slower than division ?
			//uint64_t cpu_idx = uidHndls[out_cpu_desc_handle.ptr];
			//uidHndls.erase(out_cpu_desc_handle.ptr);
			uint64_t cpu_idx = (out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement;
			uint64_t gpu_idx = (out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement;
			_ASSERT(cpu_idx == gpu_idx);
			//allocatedHandles.erase(cpu_idx);
			FreeIndices.push_back(cpu_idx);
		}
	};

	std::unique_ptr< DescriptorHeapAllocator> lmodelSRVCVB;

	std::shared_ptr<DescriptorHeapAllocator> imHAllocator;
	std::unique_ptr<DescriptorHeapAllocator> objectAllocator;

	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> imGuicommandAllocator;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> imGuicommandList;

};