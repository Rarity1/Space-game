#pragma once
#include "CWin.h"
#include "FrameResource.h"
#include "RStorage.h"
#include "SimpleCamera.h"


class Graphics
{
public:
	static const UINT bufferCount = 3;
	Graphics(HWND* hWnd, int height, int widthm);
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	~Graphics();
	struct pCamera {
		XMFLOAT4 position = {0,0,0,0};
		XMFLOAT4 rotation = {1,0,0,0};
		XMFLOAT4 upDirection = { 0,0,1,0 };
		XMFLOAT4 forwardDirect = { 0,0,0,0 };
	};
	struct rpVect {
		XMFLOAT3 position = {0,0,0};
		XMFLOAT4 rotation = {0,0,0,0};
		XMFLOAT4 orbit = { 0,0,0,0 };
		RStorage::pChange which = RStorage::INIT;
		RStorage::bmResource* model;
		XMFLOAT3 lastposition = { 0,0,0 };
		XMFLOAT3 lastrotation = { 1,0,0 };
		XMFLOAT3 lastorbit = { 1,0,0 };
	};
	void SetModelPosition(rpVect* model);
	void SetModelVectPositions(std::vector<rpVect> rpVect);
	void OnUpdate();
	void RenderFrame();
	void LoadResources();
	void LoadPipeline();
	void loadModels(UINT umID, RStorage::bmResource* model);
	pCamera curCamera;
	std::vector<std::string> loadbuff;
	std::vector<RStorage::bmResource*> modelVect;

private:

	float Max(float number, float maximum);
	float Min(float minimum, float number);
	float RotateHelper(float& rNumber);
	float timesincestart;
	void CreateFrameResources();


	void PopCommandList(FrameResource* backBuffer);
	GErrors::CheckerToken chk;
	UINT width;
	UINT height;
	HWND* hWnd;
	RStorage lModels;

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
	UINT cframeIndex;
	
	uint64_t fenceValue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12Device2> pDevice;
	Microsoft::WRL::ComPtr<IDStorageFactory> pStorage;
	Microsoft::WRL::ComPtr<IDStorageQueue> storageQueue;
	Microsoft::WRL::ComPtr<IDXGIFactory4> dxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplerDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap;
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