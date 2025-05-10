#pragma once
#include "CWin.h"
#include "RStorage.h"



#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}

class FrameResource
{
private:
    GErrors::CheckerToken chk;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSamplerDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& renderTargets;
    UINT rtvDescriptorSize;
    UINT pCbvSrvDescriptorHeapSize;
    UINT samplerDescriptorSize;
    uint_fast32_t uFrID = 0;
    CD3DX12_RECT scissorRect;
    CD3DX12_VIEWPORT viewport;
public:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
    struct fResources{
        uint_fast32_t uFrID;
        //width x, height y
        uint_fast32_t sResolution[2] {0, 0};
        Microsoft::WRL::ComPtr<ID3D12Device>  pDevice;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pSamplerDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>* renderTargets;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSignature;
        std::vector<RStorage::eResource>* models;
    };
    fResources localfResource;
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertexBufferView;
    std::vector<D3D12_INDEX_BUFFER_VIEW> indexBufferView;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> bundleAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> bundle;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> openbuffers;
    std::vector<DirectX::XMFLOAT4X4*> cbvbuff;
    UINT64 fenceValue;
    std::vector<DirectX::XMFLOAT4X4> modelMatrices;
    std::vector<RStorage::eResource>& models;
    FrameResource(fResources Resource);
    ~FrameResource();

    void InitBundle(Microsoft::WRL::ComPtr<ID3D12Device>  pDevice, Microsoft::WRL::ComPtr < ID3D12PipelineState> pPso1,
        UINT frameResourceIndex, Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> pSamplerDescriptorHeap, UINT samplerDescriptorSize, Microsoft::WRL::ComPtr < ID3D12RootSignature> pRootSignature, std::vector<RStorage::eResource>& models);
    

    void PopulateCommandList(UINT frameID);

    void UpdateConstantBuffers(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection, std::vector<RStorage::eResource>& Modls);
};
