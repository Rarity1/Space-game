#include <DirectXMath.h>

#pragma once
#include "FrameResource.h"
#include "EngineTime.h"


#include <slang.h>
#include "slang-com-ptr.h"
#include <condition_variable>

#ifdef __WIN32
#ifdef __USEDXC
#include <dxcapi.h>
#endif

#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#endif

#ifndef IMGUI_DISABLE
#include <DLLGui.h>
#endif

class DLL Graphics {
  friend class FrameResource;
  friend class Engine;

public:
  Graphics(WRect &WindowRect, HWND &hWnd, Tracker& oTracker);
  Graphics(const Graphics &) = delete;
  Graphics &operator=(const Graphics &) = delete;
  ~Graphics();
#ifndef IMGUI_DISABLE
  std::unique_ptr<imguid> iGui;
  ImGui_ImplDX12_InitInfo ImGuiInfo;
#endif
private:
Tracker& oTracker;
  const DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
  HMODULE DXGIDebug;
  void dxgichk();
  // update graphics for a list of tracked objects. Preferably objects loaded in
  // memory and meant to be rendered
  void Update();
  void CreateBuffers(Tracker::Instance &tInstance);
  void
  UpdBuffer(RStorage::bmResource &bm,
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList,
            Microsoft::WRL::ComPtr<ID3D12Device> pDevice,
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator,
            Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
  void UpdateModel(RenderedObject *bm);
  void RenderFrame();
  void LoadResources(Tracker::Instance &tInstance);
  void LoadPipeline();
  void UpdateLocalTransform(RenderedObject &bm);
  #ifdef __USEDXC
  Slang::ComPtr<IDxcBlob> CompileShader(std::string ShaderSrc,
                 std::wstring CompileVersion, std::wstring EntryPoint);
  #else
  Slang::ComPtr<slang::IBlob> CompileShaderSlang(std::string ShaderSrc,
                 std::string EntryPoint, 
                 SlangStage stage);
  #endif
  HWND &hWnd;
  const UINT bufferCount = 3;
  std::vector<std::string> loadbuff;
  void UpdateFrameResources();
  std::condition_variable uFrameResource;
  std::mutex frMutex;

  FLOAT4X4 fovPerspective;
  float timesincestart;
  void RecurLTrans(ModelData::Node *n, ModelData::Node *P);

  // uint16_t& width;
  // uint16_t& height;

  WRect &windowResolution;
  CD3DX12_RECT scissorRect;
  CD3DX12_VIEWPORT viewport;
  FrameResource::Pipeline PipelinePtrs;
  static const bool UseBundles = true;
  std::vector<std::unique_ptr<FrameResource>> FrameResources;
  struct PipelineStateStream {
    CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
    CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
    CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
    CD3DX12_PIPELINE_STATE_STREAM_VS VS;
    CD3DX12_PIPELINE_STATE_STREAM_PS PS;
    CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
    CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
  } pipelineStateStream;

  //slang compiler stuff
  SlangGlobalSessionDesc slangGlobalDesc = {};
  Slang::ComPtr<slang::IGlobalSession> gSession;
  Slang::ComPtr<slang::ISession> iSession;

  void FovPerspectiveRHInfinite(FLOAT4X4& fovOutput, float& fovRadians, float& aspect, float& nearClip);

  uint8_t cframeIndex;

  uint8_t fenceValue = 0;
  Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;
  Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
  Microsoft::WRL::ComPtr<ID3D12Device9> pDevice;
  #ifdef _DEBUG
  Microsoft::WRL::ComPtr<ID3D12Debug> debugController0;
  Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
  Microsoft::WRL::ComPtr<ID3D12InfoQueue1> D3DInfoQueue;
  Microsoft::WRL::ComPtr<IDXGIDebug> dxgiDebugController;
  Microsoft::WRL::ComPtr<IDXGIInfoQueue> DXGIInfoQueue;
  #endif
  // Microsoft::WRL::ComPtr<IDStorageFactory> pStorage;
  // Microsoft::WRL::ComPtr<IDStorageQueue> storageQueue;
  Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSamplerDescriptorHeap;
  // Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
  std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> renderTargets;
  Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
  Microsoft::WRL::ComPtr<ID3D12Fence> fence;
  std::vector<std::unique_ptr<FrameResource>> backBuffers;
  std::vector<D3D12_VERTEX_BUFFER_VIEW *> vbvarr;
  std::vector<D3D12_INDEX_BUFFER_VIEW *> ibvarr;
  Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState;

  GErrors Errors;

  GErrors::CheckerToken chk;

  UINT modelSubCount;

  HANDLE fenceEvent;

  EngineTime timer;

  std::unique_ptr<DescriptorHeapAllocator> lmodelSRVCVB;
  std::shared_ptr<DescriptorHeapAllocator> imHAllocator;
  std::unique_ptr<DescriptorHeapAllocator> objectAllocator;
};

