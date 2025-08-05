#pragma once
#include "FrameResource.h"
#include "Graphics.h"



FrameResource::FrameResource(
    Graphics* Parent,
    uint16_t puFID
):
    uFID(puFID),
    fenceValue(0)
{
    pPipelineState = (Parent->pPipelineState);
    pRootSignature = (Parent->pRootSignature);
    rtvDescriptorHeap = (Parent->rtvDescriptorHeap);
    dsvDescriptorHeap = (Parent->dsvDescriptorHeap);
    pSamplerDescriptorHeap = (Parent->pSamplerDescriptorHeap);
    pCbvSrvDescriptorHeap = Parent->objectAllocator->DescHeap;
    rtvDescriptorSize = Parent->pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    samplerDescriptorSize = Parent->pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), uFID, rtvDescriptorSize);
    dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), uFID, Parent->pDevice->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
    Parent->swapChain->GetBuffer(uFID, IID_PPV_ARGS(&renderTarget)) >> chk;
    Parent->pDevice->CreateRenderTargetView(renderTarget.Get(), nullptr, rtv);
    //DSV
    {

        auto width = Parent->windowResolution.right - Parent->windowResolution.left;
        auto height = Parent->windowResolution.bottom - Parent->windowResolution.top;
        auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            width, height,
            1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil = { 1.0f, 0 };
        Parent->pDevice->CreateCommittedResource(
            &heapprop,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthBuffer)
        ) >> chk;

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        Parent->pDevice->CreateDepthStencilView(depthBuffer.Get(), &depthStencilDesc, dsv);


    }






    Parent->pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))>>chk;


    //Command List
    Parent->pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator.Get(), nullptr, IID_PPV_ARGS(&pCommandList)) >> chk;
    NAME_D3D12_OBJECT(pCommandList);
    pCommandList->Close();


}


/*
void FrameResource::InitBundle(Microsoft::WRL::ComPtr<ID3D12Device>  pDevice, Microsoft::WRL::ComPtr < ID3D12PipelineState> pPso1,
    UINT frameResourceIndex, Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, Microsoft::WRL::ComPtr < ID3D12DescriptorHeap> pSamplerDescriptorHeap, UINT samplerDescriptorSize, Microsoft::WRL::ComPtr < ID3D12RootSignature> pRootSignature, std::vector<RStorage::eResource>& models)
{
    pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, bundleAllocator.Get(), pPso1.Get(), IID_PPV_ARGS(&bundle))>>chk;

    //PopulateCommandList(bundle, pPso1, frameResourceIndex, pCbvSrvDescriptorHeap, cbvSrvDescriptorSize, pSamplerDescriptorHeap, pRootSignature, models);

    bundle->Close()>>chk;
}


*/




void FrameResource::UpdateResolution(Graphics* Parent)
{

    Parent->swapChain->GetBuffer(uFID, IID_PPV_ARGS(&renderTarget)) >> chk;
    Parent->pDevice->CreateRenderTargetView(renderTarget.Get(), nullptr, rtv);
    //DSV
    {
        auto width = Parent->windowResolution.right - Parent->windowResolution.left;
        auto height = Parent->windowResolution.bottom - Parent->windowResolution.top;
        auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT,
            width, height,
            1, 0, 1, 0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );
        D3D12_CLEAR_VALUE clearValue{};
        clearValue.Format = DXGI_FORMAT_D32_FLOAT;
        clearValue.DepthStencil = { 1.0f, 0 };
        Parent->pDevice->CreateCommittedResource(
            &heapprop,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &clearValue,
            IID_PPV_ARGS(&depthBuffer)
        ) >> chk;

        D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        Parent->pDevice->CreateDepthStencilView(depthBuffer.Get(), &depthStencilDesc, dsv);


    }


}

void FrameResource::PopulateCommandList(CD3DX12_RECT& scissorRect, CD3DX12_VIEWPORT& viewport, std::list<Object>& trackedObjects)
{

    pCommandList->SetGraphicsRootSignature(pRootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { pCbvSrvDescriptorHeap.Get(), pSamplerDescriptorHeap.Get()};
    //ID3D12DescriptorHeap* ppHeaps[] = { pCbvSrvDescriptorHeap.Get() };

    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);


    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);



    pCommandList->OMSetRenderTargets(1, &rtv,  FALSE, &dsv);
    const FLOAT clearColor[] = {
        0,
        0,
        0
    };
    pCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    pCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    {

        pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //pCommandList->SetGraphicsRootDescriptorTable(2, pSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        pCommandList->SetPipelineState(pPipelineState.Get());

        //PIXBeginEvent(pCommandList.Get(), 0, "Draw everything");

        //Try to Render by models not tracked objects. 
        UINT indexC = 0;
        for (auto& tO : trackedObjects) {

            pCommandList->IASetIndexBuffer(&tO.model->ibuffView);
            pCommandList->IASetVertexBuffers(0, 1, &tO.model->vbuffView);

            pCommandList->SetGraphicsRootDescriptorTable(0, tO.cbvGpuHandle);
            pCommandList->SetGraphicsRootDescriptorTable(1, tO.srvGpuHandle);

            pCommandList->DrawIndexedInstanced(tO.model->uData->sIndex.size(), 1, 0, 0, 0);

        }

        //PIXEndEvent(pCommandList.Get());


    }
  
}


