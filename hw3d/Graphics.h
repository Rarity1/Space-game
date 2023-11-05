#pragma once
#include "CWin.h"
#include "FrameResource.h"
#include "RStorage.h"
#include "SimpleCamera.h"


class Graphics
{
public:
	static const UINT bufferCount = 3;
	Graphics(HWND* hWnd, int height, int widthm, Keyboard* kbd);
	virtual void OnInit();
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	~Graphics();
	
	struct rpVect {
		XMFLOAT3 position;
		RStorage::aRotation rotation;
		RStorage::pChange which;
		RStorage::bmResource* model;
	};
	void SetModelPosition(std::string name, XMFLOAT3 position, RStorage::aRotation rotation, RStorage::pChange which);
	//Only use this version if you dont hate perfomance
	void SetModelPosition(RStorage::bmResource* model, XMFLOAT3 position, RStorage::aRotation rotation, RStorage::pChange which);
	void SetModelVectPositions(std::vector<rpVect> rpVect);
	void OnUpdate();
	void RenderFrame();

	
	
private:

	float Max(float number, float maximum);
	float Min(float minimum, float number);
	float RotateHelper(float& rNumber);
	float timesincestart;
	void CreateFrameResources();
	void LoadPipeline();
	void LoadResources();
	void PopCommandList(FrameResource* backBuffer);
	GErrors::CheckerToken chk;
	UINT width;
	UINT height;
	HWND* hWnd;
	std::unique_ptr<RStorage> lModels;
	std::vector<RStorage::bmResource*> modelVect;
	static const bool UseBundles = true;
	std::vector<FrameResource*> frameResources;
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
	
	uint64_t fenceValue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12Device2> pDevice;
	Microsoft::WRL::ComPtr<IDStorageFactory> pStorage;
	Microsoft::WRL::ComPtr<IDStorageQueue> storageQueue;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerDescriptorHeap;
	
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;

	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	ComPtr<ID3D12Resource> renderTargets[bufferCount];
	std::vector<FrameResource *> backBuffers;
	FrameResource* cbackBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
	std::vector<D3D12_VERTEX_BUFFER_VIEW*> vbvarr;
	std::vector<D3D12_INDEX_BUFFER_VIEW*> ibvarr;
	

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	CD3DX12_RECT scissorRect;
	CD3DX12_VIEWPORT viewport;
	UINT modelSubCount;

	HANDLE fenceEvent;
	UINT rtvDescriptorSize;
	UINT srvDescriptorSize;
	UINT samplerDescriptorSize;
	EngineTime timer;
	SimpleCamera camera;
	Keyboard* kbd;
};