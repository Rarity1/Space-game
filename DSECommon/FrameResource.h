#pragma once
#include "CWin.h"
#include "RStorage.h"
#include "ObjectTracking.h"


#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
    pObject->SetName(name);
}

class FrameResource
{
    friend class Graphics;
private:
    GErrors::CheckerToken chk;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& rtvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& dsvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>& pPipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>& pRootSignature;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pSamplerDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pCbvSrvDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> renderTarget;
    UINT rtvDescriptorSize;
    //UINT pCbvSrvDescriptorHeapSize;
    UINT samplerDescriptorSize;
    //uint8_t uFrID = 0;
    //CD3DX12_RECT scissorRect;
    //CD3DX12_VIEWPORT viewport;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthBuffer;
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv;
    uint16_t uFID = 0;
public:
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> pCommandList;
    struct fResources{


        //Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap;

    };
    fResources localfResource;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> bundleAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> bundle;
    //std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> openbuffers;
    //std::vector<DirectX::XMFLOAT4X4*> cbvbuff;
    uint8_t fenceValue;
    FrameResource(
        Graphics* Parent,
        uint16_t uFID
    );
    ~FrameResource();

    //void InitBundle(Microsoft::WRL::ComPtr<ID3D12Device>  pDevice, Microsoft::WRL::ComPtr < ID3D12PipelineState> pPso1,
    //    UINT frameResourceIndex, Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> pSamplerDescriptorHeap, UINT samplerDescriptorSize, Microsoft::WRL::ComPtr < ID3D12RootSignature> pRootSignature, std::vector<RStorage::eResource>& models);
    
    void UpdateResolution(Graphics* Parent);


    void PopulateCommandList(CD3DX12_RECT& scissorRect, CD3DX12_VIEWPORT& viewport, Tracker::InstanceStruc& tInstance);

};
