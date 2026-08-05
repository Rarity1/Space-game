#pragma once
#include "CommonStructs.h"
#include "GraphicsErrors.h"
#include "ModelData.h"
#include "text.h"
#include <CL/opencl.hpp>
#include <filesystem>
#include <numeric>

// #include <dstorage.h>
//Unique model ID
typedef UINT umID; 
class DLL RStorage {
  // friend class Object;
  // friend class Graphics;
public:
  RStorage();
  ~RStorage();

  // Do NOT copy.
  struct bmResource {
    bmResource() = default;
    bmResource(umID &uemID, ModelData euData, std::string &ename) {
      umID = uemID;
      uData = std::make_unique<ModelData>(std::move(euData));
      name = ename;
    };
    bmResource(const bmResource &old) noexcept {
      umID = old.umID;
      uData = std::make_unique<ModelData>(*old.uData);
      name = old.name;
      clIndexBuff = old.clIndexBuff;
      clIndexMap = old.clIndexMap;
    };
    bmResource(bmResource &&old) noexcept {
      umID = std::move(old.umID);
      uData = std::move(old.uData);
      name = std::move(old.name);
      vbuffer = std::move(old.vbuffer);
      ibuffer = std::move(old.ibuffer);
      uvbuffer = std::move(old.uvbuffer);
      uibuffer = std::move(old.uibuffer);
      vbuffView = std::move(old.vbuffView);
      ibuffView = std::move(old.ibuffView);
      clBoneBuff = std::move(old.clBoneBuff);
      clBuff = std::move(old.clBuff);
      clIndexBuff = std::move(old.clIndexBuff);
      clIndexMap = std::move(old.clIndexMap);
    };
    ~bmResource() = default;
    bmResource &operator=(const bmResource &old) {
      umID = old.umID;
      uData = std::make_unique<ModelData>(*old.uData);
      name = old.name;
      clIndexBuff = old.clIndexBuff;
      clIndexMap = old.clIndexMap;
      return *this;
    }

    umID umID = 0;
    std::unique_ptr<ModelData> uData;
    std::string name;
    // Instanced texture path and buffer.
    std::filesystem::path curTexture;
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
    cl::Buffer clBoneBuff;
    cl::Buffer clBuff;
    cl::Buffer clIndexBuff;
    cl::Buffer clIndexMap;
  };

  struct mData {
    umID umID = 0;
    bmResource *model = nullptr;
    std::string name = "";
    UINT fsize = 0;
    UINT vCount = 0;
    std::filesystem::path modelPath;
  };

  bmResource& GetModel(umID umID);
  bmResource *loadModel(umID umID);
  std::filesystem::path getTexture(std::string name);

private:
  std::unordered_map<umID, mData> AvailableModels;
  std::unordered_map<umID, bmResource> LoadedModels;
  std::vector<std::filesystem::path> Textures;
  std::vector<uint8_t> defaultTexture;
  GErrors::CheckerToken chk;
};

class DescriptorHeapAllocator {
public:
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DescHeap;
  D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
  D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
  D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
  struct handls {
    D3D12_CPU_DESCRIPTOR_HANDLE *cpuHndl;
    D3D12_GPU_DESCRIPTOR_HANDLE *gpuHndl;
    // Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> originHeap;
  };
  // std::unordered_map<uint64_t, handls> allocatedHandles;
  // std::unordered_map<SIZE_T, uint64_t> uidHndls;

  Microsoft::WRL::ComPtr<ID3D12Device9> pDevice;
  uint64_t HeapHandleIncrement;
  std::vector<uint64_t> FreeIndices;
  uint64_t lastSize = 0;
  DescriptorHeapAllocator(Microsoft::WRL::ComPtr<ID3D12Device9> &pD,
                          uint64_t DescpCount = 1000000)
      : pDevice(pD) {
    {
      D3D12_DESCRIPTOR_HEAP_DESC desc = {};
      desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      desc.NumDescriptors = DescpCount;
      desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&DescHeap));
    }

    D3D12_DESCRIPTOR_HEAP_DESC desc;
    DescHeap->GetDesc(&desc);
    HeapType = desc.Type;
    DescHeap->GetCPUDescriptorHandleForHeapStart(&HeapStartCpu);
    DescHeap->GetGPUDescriptorHandleForHeapStart(&HeapStartGpu);
    HeapHandleIncrement = pDevice->GetDescriptorHandleIncrementSize(HeapType);
    FreeIndices.reserve(desc.NumDescriptors);
    FreeIndices.resize(desc.NumDescriptors);
    std::iota(FreeIndices.begin(), FreeIndices.end(), 0);
    std::reverse(FreeIndices.begin(), FreeIndices.end());
    lastSize = desc.NumDescriptors;
  }
  ~DescriptorHeapAllocator() { Destroy(); }

  void Destroy() {
    DescHeap = nullptr;
    FreeIndices.clear();
  }
  void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *out_cpu_desc_handle,
             D3D12_GPU_DESCRIPTOR_HANDLE *out_gpu_desc_handle) {
    uint64_t idx = FreeIndices.back();
    FreeIndices.pop_back();
    out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
    out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);

    // Is a map faster than division?
    // uidHndls[out_cpu_desc_handle->ptr] = idx;
    // allocatedHandles[idx] = { out_cpu_desc_handle , out_gpu_desc_handle};
  }
  void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle,
            D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle) {
    // Map faster or slower than division ?
    // uint64_t cpu_idx = uidHndls[out_cpu_desc_handle.ptr];
    // uidHndls.erase(out_cpu_desc_handle.ptr);
    uint64_t cpu_idx =
        (out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement;
    uint64_t gpu_idx =
        (out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement;
    //_ASSERT(cpu_idx == gpu_idx);
    // allocatedHandles.erase(cpu_idx);
    FreeIndices.push_back(cpu_idx);
  }
};
