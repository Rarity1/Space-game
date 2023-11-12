#pragma once
#include "CWin.h"
#include "RStorage.h"



#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class FrameResource
{
private:
    GErrors::CheckerToken chk;
    
    
public:
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
    std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferView;
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ComPtr<ID3D12CommandAllocator> bundleAllocator;
    ComPtr<ID3D12GraphicsCommandList> bundle;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> openbuffers;
    std::vector<DirectX::XMFLOAT4X4*> cbvbuff;
    UINT64 fenceValue;
    std::vector<XMFLOAT4X4> modelMatrices;
    FrameResource(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, std::vector<RStorage::bmResource*> models);
    ~FrameResource();

    void InitBundle(ID3D12Device* pDevice, ID3D12PipelineState* pPso1,
        UINT frameResourceIndex, ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, UINT samplerDescriptorSize, ID3D12RootSignature* pRootSignature, std::vector<RStorage::bmResource*> models);
    

    void PopulateCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPso1,
        UINT frameResourceIndex, ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature, std::vector<RStorage::bmResource*> models);

    void XM_CALLCONV UpdateConstantBuffers(FXMMATRIX view, CXMMATRIX projection, std::vector<RStorage::bmResource*> Modls);
};
