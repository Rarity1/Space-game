#pragma once

#include "RStorage.h"
#include <cstdint>
#include <directx/d3dx12.h>
#include "ObjectTracking.h"
#include "CommonStructs.h"

#ifdef _WIN32
#include "CWin.h"
#include <dxgi1_6.h>
#endif

#define NAME_D3D12_OBJECT(x) SetName((x).Get(), (wchar_t *)#x)
inline void SetName(ID3D12Object *pObject, LPCWSTR name) {
  pObject->SetName(name);
}


//Need to separate texture buffers and figure out how to upload multiple textures into one buffer
struct RenderBuffers {
  Microsoft::WRL::ComPtr<ID3D12Resource> tbuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> vbuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> ibuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> uvbuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> uibuffer;
  Microsoft::WRL::ComPtr<ID3D12Resource> cbvwriteBuffer;
  CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle;
  CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle;
  CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle;
  CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
  D3D12_VERTEX_BUFFER_VIEW vbuffView;
  D3D12_INDEX_BUFFER_VIEW ibuffView;
  uint32_t IndexCount = 0;
  uint32_t InstanceCount = 0;
};

class FrameResource {
private:
  GErrors::CheckerToken chk;
  ID3D12PipelineState *pPipelineState = nullptr;
  ID3D12RootSignature *pRootSignature = nullptr;
  Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
  CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTargetView;
  CD3DX12_CPU_DESCRIPTOR_HANDLE DepthStencilView;
  uint8_t uFID = 0;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

public:
  Microsoft::WRL::ComPtr<ID3D12Resource> renderTarget;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> imguiCommandList;
  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> imguicommandAllocator;

  Microsoft::WRL::ComPtr<ID3D12CommandAllocator> bundleAllocator;
  Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> bundle;
  // std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> openbuffers;
  // std::vector<DirectX::XMFLOAT4X4*> cbvbuff;
  uint8_t fenceValue;
  struct Pipeline {
    WRect &windowResolution;
    ID3D12Device9 *pDevice;
    IDXGISwapChain4 *swapChain;
    ID3D12PipelineState *pPipelineState;
    ID3D12RootSignature *pRootSignature;
  };
  FrameResource(Pipeline &PipeLine, uint8_t &puFID,
                CD3DX12_CPU_DESCRIPTOR_HANDLE rtv,
                CD3DX12_CPU_DESCRIPTOR_HANDLE dsv);
  ~FrameResource();

  // void InitBundle(Microsoft::WRL::ComPtr<ID3D12Device>  pDevice,
  // Microsoft::WRL::ComPtr < ID3D12PipelineState> pPso1,
  //     UINT frameResourceIndex, Microsoft::WRL::ComPtr <ID3D12DescriptorHeap>
  //     pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize,
  //     Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> pSamplerDescriptorHeap,
  //     UINT samplerDescriptorSize, Microsoft::WRL::ComPtr <
  //     ID3D12RootSignature> pRootSignature, std::vector<RStorage::eResource>&
  //     models);

  void UpdateResolution(Pipeline &PipeLine);

  struct CMDListInfo {};

  void PopulateCommandList(CD3DX12_RECT *scissorRect,
                           CD3DX12_VIEWPORT *viewport,
                           std::unordered_map<umID, RenderBuffers> &Buffers,
                           std::vector<ID3D12DescriptorHeap *> &ppHeaps,
                           D3D12_GPU_DESCRIPTOR_HANDLE &SamplerHeapGpuHandle);
  CD3DX12_CPU_DESCRIPTOR_HANDLE GetRTV();
};
