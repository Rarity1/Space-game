#pragma once
#include "CWin.h"
#include "FrameResource.h"
#include "RStorage.h"
#include "EngineTime.h"
#include "../ImGui/DLLGui.h"

class DLL Graphics
{
public:
	static const UINT bufferCount = 3;
	Graphics(HWND hWnd, int height, int widthm);
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
	
	void UpdateModel(RStorage::eResource* bm);
	void RenderFrame();
	void LoadResources();
	void LoadPipeline();
	void UpdateLocalTransform(RStorage::eResource& bm);
	std::shared_ptr<imguid> iGui;
	pCamera curCamera;
	std::vector<std::string> loadbuff;
	std::mutex umodel;
	std::unique_ptr<RStorage> lModels;
	std::vector<RStorage::eResource>& trackedObjects;
private:
	ImGui_ImplDX12_InitInfo ImGuiInfo;

	float Max(float number, float maximum);
	float Min(float minimum, float number);
	float RotateHelper(float& rNumber);
	float timesincestart;
	void CreateFrameResources();
	void RecurLTrans(ReadX3D::Node* n, ReadX3D::Node* P);
	int bIndex(std::vector<int> w, int bInd);
	GErrors::CheckerToken chk;
	UINT width;
	UINT height;
	HWND hwnd;
	

	static const bool UseBundles = true;
	std::vector<std::unique_ptr<FrameResource>> frameResources;
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
	UINT CurBackBuffer;
	UINT cframeIndex;
	
	uint64_t fenceValue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12Device2> pDevice;
	Microsoft::WRL::ComPtr<IDStorageFactory> pStorage;
	Microsoft::WRL::ComPtr<IDStorageQueue> storageQueue;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSamplerDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets;


	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	std::vector<std::unique_ptr<FrameResource>> backBuffers;
	std::vector<D3D12_VERTEX_BUFFER_VIEW*> vbvarr;
	std::vector<D3D12_INDEX_BUFFER_VIEW*> ibvarr;
	

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	CD3DX12_RECT scissorRect;
	CD3DX12_VIEWPORT viewport;
	UINT modelSubCount;

	HANDLE fenceEvent;

	EngineTime timer;



	class DescriptorHeapAllocator
	{
	public:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
		D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
		D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
		D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
		UINT                        HeapHandleIncrement;
		std::vector<int>               FreeIndices;

		DescriptorHeapAllocator(Microsoft::WRL::ComPtr<ID3D12Device2>& pDevice, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiSrvDescHeap)
		{
			Heap = imguiSrvDescHeap;
			D3D12_DESCRIPTOR_HEAP_DESC desc = imguiSrvDescHeap->GetDesc();
			HeapType = desc.Type;
			HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
			HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
			HeapHandleIncrement = pDevice->GetDescriptorHandleIncrementSize(HeapType);
			FreeIndices.reserve((int)desc.NumDescriptors);
			for (int n = desc.NumDescriptors; n > 0; n--)
				FreeIndices.push_back(n - 1);
		}
		~DescriptorHeapAllocator() {
			Destroy();
		}

		void Destroy()
		{
			Heap = nullptr;
			FreeIndices.clear();
		}
		void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
		{
			_ASSERT(FreeIndices.size() > 0);
			int idx = FreeIndices.back();
			FreeIndices.pop_back();
			out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
			out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
		}
		void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
		{
			int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
			int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
			_ASSERT(cpu_idx == gpu_idx);
			FreeIndices.push_back(cpu_idx);
		}
	};
	std::shared_ptr<DescriptorHeapAllocator> imHAllocator;
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> imGuicommandAllocator;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> imGuicommandList;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> imguiSrvDescHeap;
};