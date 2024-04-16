#include "FrameResource.h"



FrameResource::FrameResource(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, std::vector<RStorage::bmResource*> models) :
    fenceValue(0)
{

    // The command allocator is used by the main sample class when 
    // resetting the command list in the main update loop. Each frame 
    // resource needs a command allocator because command allocators 
    // cannot be reused until the GPU is done executing the commands 
    // associated with it.
    

    pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))>>chk;
    pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&bundleAllocator))>>chk;
 

    openbuffers.resize(std::size(models));
    cbvbuff.resize(std::size(models));
    auto temp = 0;
        for (auto& m : models) {
            vertexBufferView.emplace_back(D3D12_VERTEX_BUFFER_VIEW{
                .BufferLocation = m->vbuffer->GetGPUVirtualAddress(),
                .SizeInBytes = m->uData->fsize.fSize,
                .StrideInBytes = (UINT)sizeof(ReadX3D::Vertex)
                });
            indexBufferView.emplace_back(D3D12_INDEX_BUFFER_VIEW{
                .BufferLocation = m->ibuffer->GetGPUVirtualAddress(),
                .SizeInBytes = (UINT)std::size(m->uData->idata) * (UINT)sizeof(WORD),
                .Format = DXGI_FORMAT_R16_UINT
                });

            {
                const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
                const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(XMFLOAT4X4) + (UINT)192));
                pDevice->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &resourceDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&openbuffers[temp])) >> chk;
            }
            CD3DX12_RANGE readRange(0, 0);
            openbuffers[temp]->Map(0, &readRange, reinterpret_cast<void**>(&cbvbuff[temp])) >> chk;
            temp++;
        }
}

FrameResource::~FrameResource()
{
    for (auto& b : openbuffers) {
        b->Unmap(0, nullptr);
   }
    for (auto& b : cbvbuff)
    {
        b = nullptr;
    }
    
}

void FrameResource::InitBundle(ID3D12Device* pDevice, ID3D12PipelineState* pPso1,
    UINT frameResourceIndex, ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, UINT samplerDescriptorSize, ID3D12RootSignature* pRootSignature, std::vector<RStorage::bmResource*> models)
{
    pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, bundleAllocator.Get(), pPso1, IID_PPV_ARGS(&bundle))>>chk;

    PopulateCommandList(bundle.Get(), pPso1, frameResourceIndex, pCbvSrvDescriptorHeap, cbvSrvDescriptorSize, pSamplerDescriptorHeap, pRootSignature, models);

    bundle->Close()>>chk;
}




void FrameResource::PopulateCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPso1,
    UINT frameResourceIndex, ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature, std::vector<RStorage::bmResource*> models)
{
    pCommandList->SetGraphicsRootSignature(pRootSignature);

    ID3D12DescriptorHeap* ppHeaps[] = { pCbvSrvDescriptorHeap, pSamplerDescriptorHeap };
    pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    
    UINT frameResourceDescriptorOffset = (frameResourceIndex * (UINT)std::size(models)*2);
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvHandle(pCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), frameResourceDescriptorOffset, cbvSrvDescriptorSize);
    
    pCommandList->SetGraphicsRootDescriptorTable(1, pSamplerDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    pCommandList->SetPipelineState(pPso1);

    PIXBeginEvent(pCommandList, 0, "Draw everything");


    std::vector<int> tempvect{};

    auto temp = 0;
    for (auto& m : models) {
        
            pCommandList->IASetIndexBuffer(&indexBufferView[temp]);
            pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView[temp]);
            
            pCommandList->SetGraphicsRootDescriptorTable(2, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);
            pCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);
            pCommandList->DrawIndexedInstanced(std::size(m->uData->idata), 1, 0, 0, 0);
        temp++;
    }
    PIXEndEvent(pCommandList);
}

void FrameResource::UpdateConstantBuffers(FXMMATRIX view, CXMMATRIX projection, std::vector<RStorage::bmResource*> Modls)
{
    XMFLOAT4X4 mvp;
    auto temp = 0;
        for (auto& m : Modls)
        {
            // Compute the model-view-projection matrix.
            //XMStoreFloat4x4(&mvp,  XMMatrixTranspose(m->cmatrix * view * projection));
            XMStoreFloat4x4(&mvp, XMMatrixTranspose(m->cmatrix * view * projection));
            // Copy this matrix into the appropriate location in the upload heap subresource.
            memcpy(cbvbuff[temp], &mvp, sizeof(mvp));
            temp++;
        }
}
