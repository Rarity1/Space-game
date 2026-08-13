#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>

#include "Graphics.h"
#include "Exceptions.h"
#include "FrameResource.h"
#include "GraphicsErrors.h"
#include "ObjectTracking.h"
#include "RStorage.h"
#include "Profiler.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <directx/dxgiformat.h>
#include <fstream>
#include <functional>
#include <winnt.h>

EXTERN_C const GUID local_DXGI_DEBUG_ALL = {
    0xe48ae283,
    0xda80,
    0x490b,
    {0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x08}};

#ifdef  _DEBUG
void D3D12MessageCallback(D3D12_MESSAGE_CATEGORY Category,
                                   D3D12_MESSAGE_SEVERITY Severity,
                                   D3D12_MESSAGE_ID ID, LPCSTR pDescription,
                                   void *pContext);
#endif


Graphics::Graphics(WRect &WindowRect, HWND &hWnd, Tracker& oTracker)
    : // width(w),
      // height(h),
      oTracker(oTracker),
      hWnd(hWnd), 
      windowResolution(WindowRect), 
      PipelinePtrs(WindowRect),
      cframeIndex(0) {

  Profiler prof;
  // XMStoreFloat4x4(&fovPerspective,
  // DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV2,
  // float(windowResolution.wr.right - windowResolution.wr.left) /
  // float(windowResolution.wr.bottom - windowResolution.wr.top), 0.1f,
  // 100000.0f));

  windowResolution.Mtx.lock();
  scissorRect =
      CD3DX12_RECT(0, 0, windowResolution.wr.right, windowResolution.wr.bottom);
  viewport = CD3DX12_VIEWPORT(0.f, 0.f, windowResolution.wr.right,
                              windowResolution.wr.bottom);
  windowResolution.Mtx.unlock();
    float fov90Deg = std::numbers::pi_v<float> * 0.5;
    float aspectRatio = float(windowResolution.wr.right) / float(windowResolution.wr.bottom);
    float nearplane = 0;
  FovPerspectiveRHInfinite(fovPerspective, fov90Deg, aspectRatio, nearplane);


  // scissorRect = CD3DX12_RECT(0, 0, windowResolution.wr.right,
  // windowResolution.wr.bottom); viewport = CD3DX12_VIEWPORT(0.f, 0.f,
  // windowResolution.wr.right, windowResolution.wr.bottom);

  uint32_t dxgiFactoryFlags = 0;
#if defined(_DEBUG)
  D3D12GetDebugInterface(IID_PPV_ARGS(&debugController0)) >> chk;
  D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)) >> chk;
  debugController0->EnableDebugLayer();
  debugController1->SetEnableGPUBasedValidation(true);
  dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
  // Create D3d12 Device & dxgi factory
  CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)) >> chk;
  D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&pDevice)) >>
      chk;
  NAME_D3D12_OBJECT(pDevice);
  // RTV descriptors and buffer references
#ifdef _DEBUG
  pDevice.As(&D3DInfoQueue);
  DXGIDebug = LoadLibraryA("dxgidebug.dll");
  GetLastError() >> chk;
  typedef HRESULT(WINAPI * DXGIGetDebugInterfaceProc)(REFIID, void **);
  DXGIGetDebugInterfaceProc GetDBG = (DXGIGetDebugInterfaceProc)GetProcAddress(
      DXGIDebug, "DXGIGetDebugInterface");
  typedef GUID(WINAPI DXGI_DEBUG_ALLdll);
  assert(GetDBG);
  GetDBG(IID_PPV_ARGS(&DXGIInfoQueue)) >> chk;
  chk = GErrors::CheckerToken([this]() { dxgichk(); });

  DWORD CallbackCookie;
  D3DInfoQueue->RegisterMessageCallback(
      (D3D12MessageFunc)D3D12MessageCallback,
      D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, this, &CallbackCookie) >>
      chk;
#endif
  // Create Command Queue
  D3D12_COMMAND_QUEUE_DESC desc = {};
  desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)) >> chk;
  NAME_D3D12_OBJECT(commandQueue);

#ifndef __USEDXC
  slang::createGlobalSession(&slangGlobalDesc, gSession.writeRef()) >> chk;
  {
    slang::TargetDesc Shaders[1];
    auto &Target = Shaders[0];
    Target.format = SLANG_DXIL;
    Target.profile = gSession->findProfile("sm_6_8");
    Target.flags = 0;
    slang::SessionDesc sDesc;
    sDesc.targets = Shaders;
    sDesc.targetCount = 1;
    sDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
    gSession->createSession(sDesc, iSession.writeRef()) >> chk;
  }
#endif // using slang


  // Command Allocator
  pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                  IID_PPV_ARGS(&commandAllocator)) >>
      chk;
  NAME_D3D12_OBJECT(commandAllocator);
  // Command List
  pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                             commandAllocator.Get(), nullptr,
                             IID_PPV_ARGS(&commandList)) >>
      chk;
  NAME_D3D12_OBJECT(commandList);

  // Close the command list so it can be reset at top of draw loop
  commandList->Close() >> chk;

  // Fence
  pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) >> chk;
  NAME_D3D12_OBJECT(fence);

  // Fence signalling event
  fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!fenceEvent) {
    throw std::runtime_error("Failed to create fence event");
  }
}

void Graphics::dxgichk() {
  auto nummsgs = DXGIInfoQueue->GetNumStoredMessages(local_DXGI_DEBUG_ALL);
  for (int i = 0; i < nummsgs; i++) {
    SIZE_T messageLength = 0;
    HRESULT hr = DXGIInfoQueue->GetMessage(local_DXGI_DEBUG_ALL, i, NULL,
                                           &messageLength);
    if (hr == S_FALSE) {

      // Allocate space and get the message.
      DXGI_INFO_QUEUE_MESSAGE *pMessage =
          (DXGI_INFO_QUEUE_MESSAGE *)malloc(messageLength);
      hr = DXGIInfoQueue->GetMessage(local_DXGI_DEBUG_ALL, i, pMessage,
                                     &messageLength);

      // Do something with the message and free it
      if (hr == S_OK) {

        printf("DXGI: %s\n", pMessage->pDescription);
        fflush(stdout);
        free(pMessage);
      }
    }
  }
  DXGIInfoQueue->ClearStoredMessages(local_DXGI_DEBUG_ALL);
};

void Graphics::LoadPipeline() {
  // DStorageGetFactory(IID_PPV_ARGS(&pStorage)) >> chk;

  /*
  DSTORAGE_QUEUE_DESC qdesc = {};
  qdesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
  qdesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
  qdesc.Priority = DSTORAGE_PRIORITY_NORMAL;
  qdesc.Device = pDevice.Get();

  pStorage->CreateQueue(&qdesc, IID_PPV_ARGS(&storageQueue)) >> chk;
  */

  // Swap Chain Creation
  windowResolution.Mtx.lock();
  if (!swapChain) {
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.BufferCount = bufferCount;
    SwapChainDesc.Width = windowResolution.wr.right;
    SwapChainDesc.Height = windowResolution.wr.bottom;
    SwapChainDesc.Format = SwapChainFormat;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    HRESULT result = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(), hWnd, &SwapChainDesc, nullptr, nullptr,
        (IDXGISwapChain1 **)swapChain.GetAddressOf());
    result >> chk;
  } else {
    swapChain->Release();
    DXGI_SWAP_CHAIN_DESC1 sd = {};
    sd.BufferCount = bufferCount;
    sd.Width = windowResolution.wr.right;
    sd.Height = windowResolution.wr.bottom;
    sd.Format = SwapChainFormat;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.SampleDesc.Count = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    dxgiFactory->CreateSwapChainForHwnd(
        commandQueue.Get(), hWnd, &sd, nullptr, nullptr,
        (IDXGISwapChain1 **)swapChain.GetAddressOf()) >>
        chk;
  }
  windowResolution.Updated.store(false);
  windowResolution.Mtx.unlock();
  
  cframeIndex = swapChain->GetCurrentBackBufferIndex();

  // Sampler descriptor heap
  {
    D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
    dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    dsc.NumDescriptors = 1;
    dsc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    pDevice->CreateDescriptorHeap(&dsc,
                                  IID_PPV_ARGS(&pSamplerDescriptorHeap)) >>
        chk;
    D3D12_SAMPLER_DESC sDesc = {};

    // why so blurry anisotropic? :c
    sDesc.Filter = D3D12_FILTER_ANISOTROPIC;
    sDesc.MaxAnisotropy = 16;
    sDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sDesc.MipLODBias = 0;
    // sDesc.MinLOD = 0;
    // sDesc.MaxLOD = 100;
    D3D12_CPU_DESCRIPTOR_HANDLE temp;
    pSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart(&temp);
    pDevice->CreateSampler(&sDesc, temp);
  }


  // Root signaling
  {
    CD3DX12_DESCRIPTOR_RANGE1 ranges[3]{};
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0,
                   D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
    ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0,
                   D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
    ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0,
                   D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
    CD3DX12_ROOT_PARAMETER1 rootParameters[3]{};
    rootParameters[0].InitAsDescriptorTable(1, &ranges[0],
                                            D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsDescriptorTable(1, &ranges[1],
                                            D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(1, &ranges[2],
                                            D3D12_SHADER_VISIBILITY_PIXEL);
    
    const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;

    rootSignatureDesc.Init_1_1(rootSignatureDesc,
                               (uint32_t)std::size(rootParameters), rootParameters,
                               0, nullptr, rootSignatureFlags);
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3D10Blob> errorBlob;
    D3DX12SerializeVersionedRootSignature(&rootSignatureDesc,
                                          D3D_ROOT_SIGNATURE_VERSION_1_1,
                                          &signatureBlob, &errorBlob) >>
        chk;

    pDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
                                 signatureBlob->GetBufferSize(),
                                 IID_PPV_ARGS(&pRootSignature)) >>
        chk;
    NAME_D3D12_OBJECT(pRootSignature);
  }
  std::string RelPath = _CURRENTPATH.string();
  RelPath += ("/Shaders/");
  std::string pathVss = RelPath;
  pathVss += ("VertexShader.slang");
  std::string pathPss = RelPath;
  pathPss += ("PixelShader.slang");

#ifdef __USEDXC
  Slang::ComPtr<IDxcBlob> vsShaderblob;
  Slang::ComPtr<IDxcBlob> psShaderblob;
  vsShaderblob = CompileShader(pathVss, L"vs_6_8", L"VSmain");
  psShaderblob = CompileShader(pathPss, L"ps_6_8", L"PSmain");
#else
  Slang::ComPtr<slang::IBlob> vsShaderblob;
  Slang::ComPtr<slang::IBlob> psShaderblob;
  Slang::ComPtr<slang::ISession> vsiSession;
  Slang::ComPtr<slang::ISession> psiSession;
  vsShaderblob = CompileShaderSlang(pathVss, "VSmain", SLANG_STAGE_VERTEX);
  psShaderblob = CompileShaderSlang(pathPss, "PSmain", SLANG_STAGE_PIXEL);
#endif



  pipelineStateStream.RootSignature = pRootSignature.Get();
  pipelineStateStream.PrimitiveTopologyType =
      D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

  // Input layout for shaders
  D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
       D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
       0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
       {"TESTVAL", 0, DXGI_FORMAT_R8_UINT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
       D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  };
  pipelineStateStream.InputLayout = {inputLayout, (uint32_t)std::size(inputLayout)};
// Link Compiled shaders into pipeline state
#ifdef __USEDXC // this is stupid
  pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(
      vsShaderblob->GetBufferPointer(), vsShaderblob->GetBufferSize());
  pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(
      psShaderblob->GetBufferPointer(), psShaderblob->GetBufferSize());
#else
  pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(
      vsShaderblob->getBufferPointer(), vsShaderblob->getBufferSize());
  pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(
      psShaderblob->getBufferPointer(), psShaderblob->getBufferSize());
#endif

  pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  pipelineStateStream.RTVFormats = {
      .RTFormats{SwapChainFormat},
      .NumRenderTargets = 1,
  };

  //create the pipeline state
  D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
      sizeof(PipelineStateStream), &pipelineStateStream};
  pDevice->CreatePipelineState(&pipelineStateStreamDesc,
                               IID_PPV_ARGS(&pPipelineState)) >> chk;
  NAME_D3D12_OBJECT(pPipelineState);

  // ImGuiSetup req
  // imHAllocator = std::make_shared<DescriptorHeapAllocator>(pDevice, 64);
  objectAllocator = std::make_unique<DescriptorHeapAllocator>(pDevice);
#ifndef IMGUI_DISABLE

  ImGuiInfo.Device = pDevice.Get();
  ImGuiInfo.CommandQueue = commandQueue.Get();
  ImGuiInfo.NumFramesInFlight = bufferCount;
  ImGuiInfo.RTVFormat = SwapChainFormat;
  // ImGuiInfo.SrvDescriptorHeap = imHAllocator->SrvDescHeap;
  // std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE*,
  // D3D12_GPU_DESCRIPTOR_HANDLE*)> Alloc =
  // std::bind(&DescriptorHeapAllocator::Alloc, imHAllocator,
  // std::placeholders::_1, std::placeholders::_2);
  std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE *,
                     D3D12_GPU_DESCRIPTOR_HANDLE *)>
      Alloc{[this](D3D12_CPU_DESCRIPTOR_HANDLE *cpuh,
                   D3D12_GPU_DESCRIPTOR_HANDLE *gpuh) {
        objectAllocator->Alloc(cpuh, gpuh);
      }};

  // std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE,
  // D3D12_GPU_DESCRIPTOR_HANDLE)> Free =
  // std::bind(&DescriptorHeapAllocator::Free, imHAllocator,
  // std::placeholders::_1, std::placeholders::_2);
  std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)>
      Free{[this](D3D12_CPU_DESCRIPTOR_HANDLE cpuh,
                  D3D12_GPU_DESCRIPTOR_HANDLE gpuh) {
        objectAllocator->Free(cpuh, gpuh);
      }};
  ImGuiInfo.SrvDescriptorAllocFn = Alloc;
  ImGuiInfo.SrvDescriptorFreeFn = Free;
  iGui = std::make_unique<imguid>(hWnd, &ImGuiInfo);
#endif

  PipelinePtrs.pDevice = pDevice.Get();
  PipelinePtrs.swapChain = swapChain.Get();
  PipelinePtrs.pPipelineState = pPipelineState.Get();
  PipelinePtrs.pRootSignature = pRootSignature.Get();
}

void Graphics::Update() {
  UpdateFrameResources();
  CreateBuffers();
  UpdateResources();
  for(auto& buffers : RenderBufferMap){
  std::vector<CBVData> tempData(buffers.second.InstanceCount);
  for (auto &tInstance : oTracker.GetActiveInstances()) {
    for (auto &trackedModel : tInstance.second->GetInstanceObjects()) {
      auto Object = oTracker.IsRendered(trackedModel.second->GetID());
      if(Object){
      auto &rModel = *Object;
      if(rModel.hasModel.load() && !oTracker.CBVFlagged(rModel.ModelID) && buffers.first == rModel.ModelID){
      auto &Rotation = rModel.mPos.GetRotation();
      auto &Position = rModel.mPos.Get();
      tempData[rModel.CBVIndex].cbvMatrix = ((FLOAT4X4::Rotation(Rotation) *
           FLOAT4X4::Translation(Position).Transpose() *
           tInstance.second->GetActiveCamera()->cMatrix * fovPerspective)
              .Transpose());
      }

      }

    }
  };
  memcpy(oTracker.GetViewMatrix(buffers.first, 0), tempData.data(),
         tempData.size() * sizeof(CBVData));
  }
}

void Graphics::RenderFrame() {

  
  if (backBuffers[cframeIndex]->fenceValue !=
      (uint8_t)fence->GetCompletedValue()) {
    fence->SetEventOnCompletion(backBuffers[cframeIndex]->fenceValue,
                                fenceEvent) >>
        chk;
    WaitForSingleObject(fenceEvent, INFINITE);
  }


  iGui->imPrepare();
  auto &cframeBuffer = *backBuffers[swapChain->GetCurrentBackBufferIndex()];
  
    
#ifndef IMGUI_DISABLE
  iGui->imPopulateCommand(cframeBuffer.GetRTV(), objectAllocator->DescHeap,
                          cframeBuffer.renderTarget.Get(),
                          cframeBuffer.imguiCommandList.Get(),
                          cframeBuffer.imguicommandAllocator.Get());
#endif
  std::vector<ID3D12DescriptorHeap*> ppHeaps = {objectAllocator->DescHeap.Get(), pSamplerDescriptorHeap.Get()};
  D3D12_GPU_DESCRIPTOR_HANDLE SamplerHeapGpuHandle;
  pSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart(&SamplerHeapGpuHandle);
  cframeBuffer.PopulateCommandList(&scissorRect, &viewport, RenderBufferMap, ppHeaps, SamplerHeapGpuHandle);

//COmmand list order DOES matter. First in first out.
  std::vector<ID3D12CommandList *> commandLists = {
       cframeBuffer.pCommandList.Get(), cframeBuffer.imguiCommandList.Get()};

  cframeBuffer.fenceValue = (uint8_t)fence->GetCompletedValue() + 1;
  commandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());
  commandQueue->Signal(fence.Get(), cframeBuffer.fenceValue) >> chk;

  //  0 is valid though throws a
  // warning in debug mode.
  cframeIndex = swapChain->GetCurrentBackBufferIndex();
  // Vsync off. add toggle here for changing vsync
  swapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING | DXGI_PRESENT_DO_NOT_WAIT) >> chk;
}
// Upload batch is very expensive replace this with something better
void Graphics::CreateBuffers() {

  for (auto &ViewBuffers : oTracker.GetViewBuffers()) {
    RenderBuffers *buffptr = nullptr;
    if (RenderBufferMap.contains(ViewBuffers.first)) {
      buffptr = &RenderBufferMap[ViewBuffers.first];
    } else {
      RenderBufferMap.insert({ViewBuffers.first, RenderBuffers()});
      buffptr = &RenderBufferMap[ViewBuffers.first];
    }
    auto &Buffer = *buffptr;

    auto umID = ViewBuffers.first;
    if (oTracker.CBVFlagged(umID)) {
      oTracker.CBVUnFlag(umID);
      Buffer.InstanceCount = ViewBuffers.second.vCount;
      auto &model = *oTracker.GetModel(umID);
      Buffer.IndexCount = model.sIndex.size();

      if (Buffer.vbuffer == nullptr) {
        uint32_t vbuffSize = model.sIndex.size() * sizeof(Vertex);
        {
          const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
          const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vbuffSize);

          pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
                                           &resourceDesc,
                                           D3D12_RESOURCE_STATE_COMMON, nullptr,
                                           IID_PPV_ARGS(&Buffer.vbuffer)) >>
              chk;
          Buffer.vbuffView = D3D12_VERTEX_BUFFER_VIEW{
              .BufferLocation = Buffer.vbuffer->GetGPUVirtualAddress(),
              .SizeInBytes = vbuffSize,
              .StrideInBytes = (uint32_t)sizeof(Vertex)};
        }
        {
          const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_UPLOAD};
          const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vbuffSize);
          pDevice->CreateCommittedResource(
              &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
              D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
              IID_PPV_ARGS(&Buffer.uvbuffer)) >>
              chk;
        }
        uint32_t ibuffSize = model.sIndex.size() * sizeof(uint32_t);

        {
          const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_DEFAULT};
          const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(ibuffSize);
          pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
                                           &resourceDesc,
                                           D3D12_RESOURCE_STATE_COMMON, nullptr,
                                           IID_PPV_ARGS(&Buffer.ibuffer)) >>
              chk;
          Buffer.ibuffView = D3D12_INDEX_BUFFER_VIEW{
              .BufferLocation = Buffer.ibuffer->GetGPUVirtualAddress(),
              .SizeInBytes = (uint32_t)std::size(model.sIndex) *
                             (uint32_t)sizeof(uint32_t),
              .Format = DXGI_FORMAT_R32_UINT};
        }

        {
          const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_UPLOAD};
          const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(ibuffSize);
          pDevice->CreateCommittedResource(
              &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
              D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
              IID_PPV_ARGS(&Buffer.uibuffer)) >>
              chk;
        }
        {
          {

            Vertex *mappedVertexData = nullptr;
            Buffer.uvbuffer->Map(
                0, nullptr, reinterpret_cast<void **>(&mappedVertexData)) >>
                chk;
            uint32_t *mappedIndexData = nullptr;
            Buffer.uibuffer->Map(0, nullptr,
                                 reinterpret_cast<void **>(&mappedIndexData)) >>
                chk;

            memcpy(mappedVertexData, model.MappedVertices.data(), vbuffSize);
            memcpy(mappedIndexData, model.sIndex.data(), ibuffSize);
          }
          Buffer.uvbuffer->Unmap(0, nullptr);
          Buffer.uibuffer->Unmap(0, nullptr);
        }
      }
      if (Buffer.cbvwriteBuffer != nullptr) {
        objectAllocator->Free(Buffer.cbvCpuHandle, Buffer.cbvGpuHandle);
        Buffer.cbvwriteBuffer->Unmap(0, nullptr);
        Buffer.cbvwriteBuffer.Reset();
      }
      objectAllocator->Alloc(&Buffer.cbvCpuHandle, &Buffer.cbvGpuHandle);

      uint32_t size =
          (sizeof(CBVData) * oTracker.ModelObjectCount(umID)) +
          (256 - ((sizeof(CBVData) * oTracker.ModelObjectCount(umID)) % 256));
      {
        const CD3DX12_HEAP_PROPERTIES heapProps{D3D12_HEAP_TYPE_UPLOAD};
        const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
        pDevice->CreateCommittedResource(
            &heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&Buffer.cbvwriteBuffer)) >>
            chk;
      }
      CD3DX12_RANGE readRange(0, 0);
      Buffer.cbvwriteBuffer->Map(
          0, &readRange,
          reinterpret_cast<void **>(oTracker.GetViewMatrixAddress(umID))) >>
          chk;
      {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = Buffer.cbvwriteBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = size;
        pDevice->CreateConstantBufferView(&cbvDesc, Buffer.cbvCpuHandle);
      }
      if (Buffer.tbuffer == nullptr) {
        DirectX::ResourceUploadBatch ResourceUpload(pDevice.Get());
        ResourceUpload.Begin();

        // Check if texture exists. This does not currently do that
        auto tname = oTracker.GetResource(umID)->name; 
        if (tname != "") {
          CreateDDSTextureFromFile(pDevice.Get(), ResourceUpload,
                                   oTracker.GetTexture(tname).c_str(),
                                   Buffer.tbuffer.ReleaseAndGetAddressOf()) >>
              chk;
        } else {
          CreateDDSTextureFromMemory(pDevice.Get(), ResourceUpload, missing_dds,
                                     missing_dds_size,
                                     Buffer.tbuffer.ReleaseAndGetAddressOf()) >>
              chk;
        }

        {
          D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
          srvDesc.Shader4ComponentMapping =
              D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
          // Remember to change to bc7 textures when gimp gets support
          srvDesc.Format = DXGI_FORMAT_UNKNOWN;
          srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
          srvDesc.Texture2D.MipLevels = -1;
          objectAllocator->Alloc(&Buffer.srvCpuHandle, &Buffer.srvGpuHandle);
          pDevice->CreateShaderResourceView(Buffer.tbuffer.Get(), &srvDesc,
                                            Buffer.srvCpuHandle);
        }
        ResourceUpload.End(commandQueue.Get());
      }
      pDevice->GetDeviceRemovedReason() >> chk;
    }
  }
}

// Requires you to open and close command list
void const Graphics::UpdateBuffer(
    RenderBuffers& buffers,
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {
  {
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        buffers.vbuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);
  }
  {
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        buffers.ibuffer.Get(), D3D12_RESOURCE_STATE_INDEX_BUFFER,
        D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->ResourceBarrier(1, &barrier);
  }
  commandList->CopyResource(buffers.vbuffer.Get(), buffers.uvbuffer.Get());
  commandList->CopyResource(buffers.ibuffer.Get(), buffers.uibuffer.Get());
  {
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        buffers.vbuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    commandList->ResourceBarrier(1, &barrier);
  }
  {
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        buffers.ibuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_INDEX_BUFFER);
    commandList->ResourceBarrier(1, &barrier);
  }
}
//Needs some barriers here to prevent resetting every frame
void Graphics::UpdateResources() {
  commandAllocator->Reset() >> chk;
  commandList->Reset(commandAllocator.Get(), nullptr) >> chk;
  for (auto &buffers : RenderBufferMap) {
    auto &Buffer = buffers.second;
    {
      auto& model= *oTracker.GetModel(buffers.first);
      Vertex *mappedVertexData = nullptr;
      Buffer.uvbuffer->Map(0, nullptr,
                           reinterpret_cast<void **>(&mappedVertexData)) >>
          chk;
      uint32_t vbuffSize = model.sIndex.size() * sizeof(Vertex);
      memcpy(mappedVertexData, model.MappedVertices.data(), vbuffSize);
    }
    Buffer.uvbuffer->Unmap(0, nullptr);
    UpdateBuffer(buffers.second, commandList);
  }
  commandList->Close() >> chk;
  {
    ID3D12CommandList *const commandLists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(std::size(commandLists), commandLists);
    // Need a fence here. So that resources can be uploaded on the fly
  }
}

void Graphics::UpdateLocalTransform(RenderedObject &bm) {
  using namespace DirectX;
  if (std::strstr(oTracker.GetModel(bm.ModelID)->bdata[0].name.c_str(), "placeholder"))
    return;
  const FLOAT4X4 temp{1.f, 0.f, 0.f, 0.f, 
                0.f, 1.f, 0.f, 0.f,
                0.f, 0.f, 1.f, 0.f,
                0.f, 0.f, 0.f, 1.f};
  oTracker.GetModel(bm.ModelID)->ndata.LocalTransform = temp * oTracker.GetModel(bm.ModelID)->ndata.matrix;

  for (auto &c : oTracker.GetModel(bm.ModelID)->ndata.children) {
    RecurLTrans(&c, &oTracker.GetModel(bm.ModelID)->ndata);
  }

  for (auto &P : oTracker.GetModel(bm.ModelID)->ndata.aChildren) {
    for (auto &n : P->aChildren) {
      RecurLTrans(n, P);
    }
  }
}

void Graphics::RecurLTrans(Node *n, Node *P) {
  n->LocalTransform = P->LocalTransform * n->matrix;
}

// Fix model updates
void Graphics::UpdateModel(RenderedObject *bm) {
  auto GlobITrans =
      oTracker.GetModel(bm->ModelID)->ndata.matrix.Inverse();
  if (!std::strstr(oTracker.GetModel(bm->ModelID)->bdata[0].name.c_str(), "placeholder"))
    for (auto &b : oTracker.GetModel(bm->ModelID)->bdata) {
      b.finalTransform = b.matrix * b.node->LocalTransform * GlobITrans;
    };
  auto vdata = oTracker.GetModel(bm->ModelID)->MappedVertices;
  auto &idata = oTracker.GetModel(bm->ModelID)->sIndex;
  Vertex *mappedVertexData = nullptr;
  //oTracker.GetModel(bm->ModelID).uvbuffer->Map(0, nullptr, reinterpret_cast<void **>(&mappedVertexData)) >> chk;
  // Fix animations
  //UpdateLocalTransform(*bm);
  if (oTracker.GetModel(bm->ModelID)->bdata.size() > 0) {
    for (auto v = 0; v < std::size(idata); v++) {
      auto &weights = oTracker.GetModel(bm->ModelID)->weights[v];
      FLOAT4 vd = vdata[idata[v]][v % 3].position;

      for (auto w = 0; w < std::size(weights.weight); w++) {
        vd = (vd * (oTracker.GetModel(bm->ModelID)->bdata[weights.bIndex[w]].finalTransform * weights.weight[w]));
      }
      vdata[idata[v]][v % 3].position.x += vd.x;
      vdata[idata[v]][v % 3].position.y += vd.y;
      vdata[idata[v]][v % 3].position.z += vd.z;
    }
  }
  //memcpy(mappedVertexData, vdata.data(), sizeof(Vertex) * idata.size());

  //oTracker.GetModel(bm->ModelID).uvbuffer->Unmap(0, nullptr);

  //UpdBuffer(oTracker.GetModel(bm->ModelID), commandList, pDevice, commandAllocator, commandQueue);
}

#ifdef __USEDXC
Slang::ComPtr<IDxcBlob> Graphics::CompileShader(std::string ShaderSrc,
                                                std::wstring CompileVersion,
                                                std::wstring EntryPoint) {
  Microsoft::WRL::ComPtr<IDxcCompiler3> compiler(nullptr);
  DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)) >> chk;
  Microsoft::WRL::ComPtr<IDxcUtils> utils(nullptr);
  DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)) >> chk;
  Microsoft::WRL::ComPtr<IDxcIncludeHandler> handler(nullptr);
  utils->CreateDefaultIncludeHandler(&handler) >> chk;
  Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceblob(nullptr);
  std::wstring depreciatedButNot =
      std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ShaderSrc);

  utils->LoadFile(depreciatedButNot.c_str(), nullptr, &sourceblob) >> chk;
  LPCWSTR args[]{
      L"-E",
      EntryPoint.c_str(),
      L"-T",
      CompileVersion.c_str(),
  // DXC_ARG_ALL_RESOURCES_BOUND,
#ifdef _DEBUG
      DXC_ARG_DEBUG,
      DXC_ARG_SKIP_OPTIMIZATIONS,
#else
      DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
      DXC_ARG_WARNINGS_ARE_ERRORS,
      // L"-Qstrip_reflect",
      // L"-Qstrip_debug",
  };
  DxcBuffer buffer{};
  buffer.Encoding = DXC_CP_ACP;
  buffer.Ptr = sourceblob->GetBufferPointer();
  buffer.Size = sourceblob->GetBufferSize();
  Microsoft::WRL::ComPtr<IDxcResult> results;
  compiler->Compile(&buffer, args, _countof(args), handler.Get(),
                    IID_PPV_ARGS(&results)) >>
      chk;
  Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
  results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr) >> chk;

  if (errors && errors->GetStringLength()) {
    printf("ERROR: %s/n", errors->GetStringPointer());
    fflush(stdout);
    OutputDebugStringA("\n Shader compilation error: \n");
    OutputDebugStringA(errors->GetStringPointer());
  } else {
    OutputDebugStringA("\n Shader compilation Succeeded \n");
  }
  HRESULT status{S_OK};
  results->GetStatus(&status) >> chk;
  status >> chk;

  Slang::ComPtr<IDxcBlob> Shaderblob(nullptr);

  results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(Shaderblob.writeRef()),
                     nullptr) >>
      chk;
  return Shaderblob;
}
#else
Slang::ComPtr<slang::IBlob> Graphics::CompileShaderSlang(std::string ShaderSrc,
                                                         std::string EntryPoint,
                                                         SlangStage stage) {
  using namespace slang;
  // Get slang to work somehow
  Slang::ComPtr<slang::IBlob> Result;
  Slang::ComPtr<slang::IBlob> diag;
  // Debuging code for shaders
  std::ifstream ShaderStream(ShaderSrc);
  std::ostringstream ifsBuffer;
  ifsBuffer << ShaderStream.rdbuf();
  ShaderStream.close();
  std::string SrcString = ifsBuffer.str();
  Slang::ComPtr<slang::IModule> Module;
  Module = iSession->loadModuleFromSourceString(
      EntryPoint.c_str(), ShaderSrc.c_str(), SrcString.c_str(),
      diag.writeRef());
  if (diag) {
    printf("ERROR: %s /n", (const char *)diag->getBufferPointer());
    fflush(stdout);
  }

  Slang::ComPtr<slang::IEntryPoint> vsEntry;
  Module->findAndCheckEntryPoint(EntryPoint.c_str(), stage, vsEntry.writeRef(),
                                 diag.writeRef());
  if (diag) {
    printf("ERROR: %s /n", (const char *)diag->getBufferPointer());
    fflush(stdout);
  }
  std::array<slang::IComponentType *, 2> cTypes = {Module, vsEntry};
  Slang::ComPtr<slang::IComponentType> ComposedShader;

  iSession->createCompositeComponentType(cTypes.data(), cTypes.size(),
                                         ComposedShader.writeRef(),
                                         diag.writeRef()) >>
      chk;
  if (diag) {
    std::string((const char *)diag->getBufferPointer()).c_str() >> Exceptions::CheckerToken();
  }
  Slang::ComPtr<slang::IComponentType> LinkedShader;
  ComposedShader->link(LinkedShader.writeRef(), diag.writeRef()) >> chk;
  if (diag) {
    std::string((const char *)diag->getBufferPointer()).c_str() >> Exceptions::CheckerToken();
  }
  LinkedShader->getEntryPointCode(0, 0, Result.writeRef(), diag.writeRef());
  if (diag) {
    std::string((const char *)diag->getBufferPointer()).c_str() >> Exceptions::CheckerToken();
  }
  return Result;
}
#endif

void Graphics::FovPerspectiveRHInfinite(FLOAT4X4& fovOutput, float& fovRadians, float& aspect, float& nearClip){
  
  uint32_t wrap = 0;
  wrap += -1;
  auto e = 1/((float)wrap);
  float focallength = 1/fovRadians*2;
  FLOAT4X4 Result{
    focallength, 0,0,0,
    0,focallength*aspect, 0,0,
    0,0, e-(1-nearClip), (e-2)*(1-nearClip),
    0,0,-1,0
  };
  fovOutput = Result;
}

void Graphics::UpdateFrameResources() {
  if (backBuffers.size() < bufferCount) {
    rtvDescriptorHeap.Reset();
    {
      D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
      dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
      dsc.NumDescriptors = bufferCount;
      pDevice->CreateDescriptorHeap(&dsc, IID_PPV_ARGS(&rtvDescriptorHeap)) >>
          chk;
    }
    dsvDescriptorHeap.Reset();
    // Depth Stencil view Descriptor heap
    {
      const D3D12_DESCRIPTOR_HEAP_DESC desc = {
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
          .NumDescriptors = bufferCount,
      };
      pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvDescriptorHeap)) >>
          chk;
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE temprtv;
    rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(&temprtv);
    CD3DX12_CPU_DESCRIPTOR_HANDLE tempdsv;
    dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(&tempdsv);
    auto RtvSize = pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    auto DsvSize = pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    backBuffers.resize(0);
    for (uint8_t uFID = 0; uFID < bufferCount; uFID++) {

      backBuffers.emplace_back(std::make_unique<FrameResource>(
          PipelinePtrs, uFID,
          CD3DX12_CPU_DESCRIPTOR_HANDLE(temprtv, uFID, RtvSize),
          CD3DX12_CPU_DESCRIPTOR_HANDLE(tempdsv, uFID, DsvSize)));
      // pFrameResource.InitBundle(pDevice.Get(), pipelineState.Get(), i,
      // srvDescriptorHeap.Get(), srvDescriptorSize,
      // samplerDescriptorHeap.Get(), samplerDescriptorSize,
      // rootSignature.Get(), modelVect);
    }
  }

  if (windowResolution.Updated) {
  windowResolution.Updated.store(false);

  windowResolution.Mtx.lock();
    fenceValue = fence->GetCompletedValue();
    {
      int8_t fv = fenceValue + 1;
      commandQueue->Signal(fence.Get(), fv);
      if (fenceValue != fv) {
        fence->SetEventOnCompletion(fv, fenceEvent) >> chk;
        WaitForSingleObject(fenceEvent, INFINITE);
      }
    }

    // Swapchain buffers wont resize until all buffers are unused.
    for (auto &b : backBuffers) {
      b->renderTarget.Reset();
    }

    swapChain->ResizeBuffers(bufferCount, windowResolution.wr.right, windowResolution.wr.bottom,
                             DXGI_FORMAT_UNKNOWN,
                             DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) >>
        chk;
    scissorRect = CD3DX12_RECT(0, 0, windowResolution.wr.right, windowResolution.wr.bottom );
    viewport = CD3DX12_VIEWPORT(0.f, 0.f, windowResolution.wr.right, windowResolution.wr.bottom );
    float fov90Deg = std::numbers::pi_v<float>*0.5;
    float aspectRatio = float(windowResolution.wr.right) / float(windowResolution.wr.bottom);
    float nearplane = 0.f;
    FovPerspectiveRHInfinite(fovPerspective, fov90Deg, aspectRatio, nearplane);

    for (auto &b : backBuffers) {
      b->UpdateResolution(PipelinePtrs);
    }
    windowResolution.Mtx.unlock();
  }

}

Graphics::~Graphics() {
  // iGui Is not very graceful to shutdown. Need to close it first before
  // graphics unload.
  for(auto& b : RenderBufferMap){
    if(b.second.cbvwriteBuffer.Get()){
      b.second.cbvwriteBuffer->Unmap(0, nullptr);
    }
  }
  if (pDevice != nullptr) {
    fenceValue = fence->GetCompletedValue();
    {
      int8_t fv = fenceValue + 1;
      commandQueue->Signal(fence.Get(), fv);
      if (fenceValue != fv) {
        fence->SetEventOnCompletion(fv, fenceEvent) >> chk;
        WaitForSingleObject(fenceEvent, INFINITE);
      }
    }
#ifndef IMGUI_DISABLE
    iGui.reset();
#endif
    GetLastError() >> chk;
    CloseHandle(fenceEvent);
    backBuffers.resize(0);
  }
}
