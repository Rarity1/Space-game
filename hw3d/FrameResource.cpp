#include "FrameResource.h"



FrameResource::FrameResource(Microsoft::WRL::ComPtr<ID3D12Device> pDevice, std::vector<RStorage::eResource*>& models) :
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
    CD3DX12_RANGE readRange(0, 0);
    for (auto i = 0; i < std::size(models); i++) {
            vertexBufferView.emplace_back(D3D12_VERTEX_BUFFER_VIEW{
                .BufferLocation = models[i]->model->vbuffer->GetGPUVirtualAddress(),
                .SizeInBytes = (UINT)std::size(models[i]->model->uData->Vertdata) * (UINT)sizeof(ReadX3D::Vertex),
                .StrideInBytes = (UINT)sizeof(ReadX3D::Vertex)
                });
            indexBufferView.emplace_back(D3D12_INDEX_BUFFER_VIEW{
                .BufferLocation = models[i]->model->ibuffer->GetGPUVirtualAddress(),
                .SizeInBytes = (UINT)std::size(models[i]->model->uData->idata) * (UINT)sizeof(WORD),
                .Format = DXGI_FORMAT_R16_UINT
                });

            {
                const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
                const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer((sizeof(DirectX::XMFLOAT4X4) + (UINT)192));
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
    cbvbuff.resize(0);
}

void FrameResource::InitBundle(ID3D12Device* pDevice, ID3D12PipelineState* pPso1,
    UINT frameResourceIndex, ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, UINT samplerDescriptorSize, ID3D12RootSignature* pRootSignature, std::vector<RStorage::eResource*>& models)
{
    pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, bundleAllocator.Get(), pPso1, IID_PPV_ARGS(&bundle))>>chk;

    PopulateCommandList(bundle.Get(), pPso1, frameResourceIndex, pCbvSrvDescriptorHeap, cbvSrvDescriptorSize, pSamplerDescriptorHeap, pRootSignature, models);

    bundle->Close()>>chk;
}




void FrameResource::PopulateCommandList(ID3D12GraphicsCommandList* pCommandList, ID3D12PipelineState* pPso1,
    UINT frameResourceIndex, ID3D12DescriptorHeap* pCbvSrvDescriptorHeap, UINT cbvSrvDescriptorSize, ID3D12DescriptorHeap* pSamplerDescriptorHeap, ID3D12RootSignature* pRootSignature, std::vector<RStorage::eResource*>& models)
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
    for (auto i = 0; i < std::size(models); i++) {
        
            pCommandList->IASetIndexBuffer(&indexBufferView[i]);
            pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView[i]);
            
            pCommandList->SetGraphicsRootDescriptorTable(2, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);
            pCommandList->SetGraphicsRootDescriptorTable(0, cbvSrvHandle);
            cbvSrvHandle.Offset(cbvSrvDescriptorSize);
            pCommandList->DrawIndexedInstanced(std::size(models[i]->model->uData->idata), 1, 0, 0, 0);
    }
    PIXEndEvent(pCommandList);
}

void FrameResource::UpdateConstantBuffers(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection, std::vector<RStorage::eResource*>& Modls)
{
    DirectX::XMFLOAT4X4 mvp;
    for (auto i = 0; i < std::size(Modls); i++)
    {
        // Compute the model-view-projection matrix.
        //XMStoreFloat4x4(&mvp,  XMMatrixTranspose(m->cmatrix * view * projection));
        XMStoreFloat4x4(&mvp, XMMatrixTranspose(Modls[i]->model->cmatrix * view * projection));
        // Copy this matrix into the appropriate location in the upload heap subresource.
        memcpy(cbvbuff[i], &mvp, sizeof(mvp));
    }
}
