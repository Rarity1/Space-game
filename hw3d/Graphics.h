#pragma once
#include "CWin.h"
#include "FrameResource.h"
#include "RStorage.h"


class Graphics
{
public:
	static const UINT bufferCount = 3;
	Graphics(HWND* hWnd, int height, int widthm);
	Graphics(const Graphics&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	~Graphics();
	struct pCamera {
		DirectX::XMFLOAT3* position = nullptr;
		DirectX::XMFLOAT4 rotation = {1,0,0,0};
		DirectX::XMFLOAT4 upDirection = { 0,0,1,0 };
		DirectX::XMFLOAT4 forwardDirect = { 1,0,0,0 };
		DirectX::XMMATRIX cmatrix;
	};
	
	void UpdateModel(RStorage::eResource* bm);
	void RenderFrame();
	void LoadResources(int numLoadedSrv);
	void LoadPipeline();
	int loadModels(RStorage::eResource* model, bool unique = false);
	int loadModels(std::vector<RStorage::eResource*>& model, bool replace = false);
	void UpdateLocalTransform(RStorage::eResource* bm);
	pCamera curCamera;
	std::vector<std::string> loadbuff;
	//std::vector<RStorage::bmResource*> modelVect;
	std::mutex umodel;
	std::unique_ptr<RStorage> lModels;

private:

	float Max(float number, float maximum);
	float Min(float minimum, float number);
	float RotateHelper(float& rNumber);
	float timesincestart;
	void CreateFrameResources();
	void RecurLTrans(ReadX3D::Node* n, ReadX3D::Node* P);
	void PopCommandList(FrameResource* backBuffer);
	int bIndex(std::vector<int> w, int bInd);
	GErrors::CheckerToken chk;
	UINT width;
	UINT height;
	HWND* hWnd;
	

	static const bool UseBundles = true;
	std::vector<FrameResource*> frameResources;
	std::vector<RStorage::eResource*>* modelVect;
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
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets[bufferCount];
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
	Keyboard* kbd;
};