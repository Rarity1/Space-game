#include "FrameResource.h"



FrameResource::FrameResource(fResources Resource, std::vector<RStorage::eResource>& models) :
    fenceValue(0),
    trackedObjects(models),
    pPipelineState(Resource.pPipelineState),
    pRootSignature(Resource.pRootSignature),
    viewport(0.0f, 0.0f, static_cast<float>(Resource.sResolution[0]), static_cast<float>(Resource.sResolution[1])),
    scissorRect(0, 0, static_cast<LONG>(Resource.sResolution[0]), static_cast<LONG>(Resource.sResolution[1])),
    renderTargets(*Resource.renderTargets)
{

    auto& pDevice = Resource.pDevice;
    auto& swapChain = Resource.swapChain;
    pCbvSrvDescriptorHeap = Resource.pCbvSrvDescriptorHeap;
    pSamplerDescriptorHeap = Resource.pSamplerDescriptorHeap;
    rtvDescriptorHeap = Resource.rtvDescriptorHeap;
    dsvDescriptorHeap = Resource.dsvDescriptorHeap;
    pRootSignature = Resource.pRootSignature;
    //pCommandList = Resource.pCommandList;
    uFrID = Resource.uFrID;

    rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    pCbvSrvDescriptorHeapSize = pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    samplerDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);


    // The command allocator is used by the main sample class when 
    // resetting the command list in the main update loop. Each frame 
    // resource needs a command allocator because command allocators 
    // cannot be reused until the GPU is done executing the commands 
    // associated with it.
    openbuffers.resize(std::size(trackedObjects));
    cbvbuff.resize(std::size(trackedObjects));

    pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))>>chk;
    //pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&bundleAllocator))>>chk;


    //Command List
    pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)) >> chk;
    NAME_D3D12_OBJECT(pCommandList);
    pCommandList->Close();


    






    

    CD3DX12_RANGE readRange(0, 0);
    for (auto i = 0; i < std::size(trackedObjects); i++) {
            vertexBufferView.emplace_back(D3D12_VERTEX_BUFFER_VIEW{
                .BufferLocation = trackedObjects[i].model->vbuffer->GetGPUVirtualAddress(),
                .SizeInBytes = (UINT)std::size(trackedObjects[i].model->uData->sIndex) * (UINT)sizeof(ReadX3D::Vertex),
                .StrideInBytes = (UINT)sizeof(ReadX3D::Vertex)
                });
            indexBufferView.emplace_back(D3D12_INDEX_BUFFER_VIEW{
                .BufferLocation = trackedObjects[i].model->ibuffer->GetGPUVirtualAddress(),
                .SizeInBytes = (UINT)std::size(trackedObjects[i].model->uData->sIndex) * (UINT)sizeof(uint32_t),
                .Format = DXGI_FORMAT_R32_UINT
                });

            {
                const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
                const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(DirectX::XMFLOAT4X4)+(UINT)192));
                pDevice->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&openbuffers[i])) >> chk;
            }
            openbuffers[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbvbuff[i])) >> chk;
        }


    

}

FrameResource::~FrameResource()
{
    for (auto& b : openbuffers) {
        b->Unmap(0, nullptr);
   }
}

void FrameResource::InitBundle(Microsoft::WRL::ComPtr<ID3D12Device>  pDevice, Microsoft::WRL::ComPtr < ID3D12PipelineState> pPso1,
    UINT frameResourceIndex, Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> pSamplerDescriptorHeap, UINT samplerDescriptorSize, Microsoft::WRL::ComPtr < ID3D12RootSignature> pRootSignature, std::vector<RStorage::eResource>& models)
{
    pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, bundleAllocator.Get(), pPso1.Get(), IID_PPV_ARGS(&bundle))>>chk;

    //PopulateCommandList(bundle, pPso1, frameResourceIndex, pCbvSrvDescriptorHeap, cbvSrvDescriptorSize, pSamplerDescriptorHeap, pRootSignature, models);

    bundle->Close()>>chk;
}




void FrameResource::PopulateCommandList(UINT frameID)
{

    using namespace DirectX;
    //commandAllocator->Reset() >> chk;
    //pCommandList->Reset(commandAllocator.Get(), pPipelineState.Get()) >> chk;

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { pCbvSrvDescriptorHeap.Get() , pSamplerDescriptorHeap.Get() };
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameID].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        pCommandList->ResourceBarrier(1, &barrier);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameID, rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    pCommandList->OMSetRenderTargets(1, &rtv,  FALSE, &dsv);
    const FLOAT clearColor[] = {
        0,
        0,
        0
    };
    pCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    pCommandList->ClearDepthStencilView(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    {
        //pCommandList->SetGraphicsRootSignature(pRootSignature.Get());
        //ID3D12DescriptorHeap* ppHeaps[] = { pCbvSrvDescriptorHeap.Get() , pSamplerDescriptorHeap.Get() };
        //pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UINT frameResourceDescriptorOffset = (uFrID * (UINT)std::size(trackedObjects) * 2);
        CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(pCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), frameResourceDescriptorOffset, pCbvSrvDescriptorHeapSize);

        pCommandList->SetGraphicsRootDescriptorTable(2, pSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        pCommandList->SetPipelineState(pPipelineState.Get());

        PIXBeginEvent(pCommandList.Get(), 0, "Draw everything");

        for (auto i = 0; i < std::size(trackedObjects); i++) {

            pCommandList->IASetIndexBuffer(&indexBufferView[i]);
            pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView[i]);

            pCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);
            cbvSrvHandle.Offset(pCbvSrvDescriptorHeapSize);
            pCommandList->SetGraphicsRootDescriptorTable(1, cbvSrvHandle);
            cbvSrvHandle.Offset(pCbvSrvDescriptorHeapSize);
            pCommandList->DrawIndexedInstanced(trackedObjects[i].model->uData->sIndex.size(), 1, 0, 0, 0);
        }
        PIXEndEvent(pCommandList.Get());


    }
   
    
    {
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            renderTargets[frameID].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
        );
        pCommandList->ResourceBarrier(1, &barrier);
    }
    //commandAllocator->Reset() >> chk;
    //pCommandList->Reset(commandAllocator.Get(), pPipelineState.Get()) >> chk;
    //pCommandList->Close()>>chk;
}

void FrameResource::UpdateConstantBuffers(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection)
{
    using namespace DirectX;
    for (auto i = 0; i < std::size(trackedObjects); i++)
    {

        XMStoreFloat4x4(cbvbuff[i], XMMatrixTranspose(XMLoadFloat4x4(&trackedObjects[i].cmatrix) * view * projection));
        // Copy this matrix into the appropriate location in the upload heap subresource.
        //memcpy(cbvbuff[i], &mvp, sizeof(mvp));
    }
}
