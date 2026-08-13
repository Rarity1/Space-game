#include "FrameResource.h"

FrameResource::FrameResource(Pipeline &PipeLine, uint8_t &puFID,
                             CD3DX12_CPU_DESCRIPTOR_HANDLE rtv,
                             CD3DX12_CPU_DESCRIPTOR_HANDLE dsv)
    : pPipelineState(PipeLine.pPipelineState),
      pRootSignature(PipeLine.pRootSignature), RenderTargetView(rtv),
      DepthStencilView(dsv), uFID(puFID), fenceValue(0) {

  PipeLine.swapChain->GetBuffer(uFID, IID_PPV_ARGS(&renderTarget)) >> chk;
  PipeLine.pDevice->CreateRenderTargetView(renderTarget.Get(), nullptr,
                                           RenderTargetView);
  {
    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil = {1.f, 0};
    auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, PipeLine.windowResolution.wr.right,
        PipeLine.windowResolution.wr.bottom, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    PipeLine.pDevice->CreateCommittedResource(
        &heapprop, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue,
        IID_PPV_ARGS(&depthBuffer)) >>
        chk;
  }
  PipeLine.pDevice->CreateDepthStencilView(depthBuffer.Get(), nullptr,
                                           DepthStencilView);
  PipeLine.pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                           IID_PPV_ARGS(&commandAllocator)) >>
      chk;
  PipeLine.pDevice->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&imguicommandAllocator)) >>
      chk;

  // Command List
  PipeLine.pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      commandAllocator.Get(), nullptr,
                                      IID_PPV_ARGS(&pCommandList)) >>
      chk;
  PipeLine.pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      imguicommandAllocator.Get(), nullptr,
                                      IID_PPV_ARGS(&imguiCommandList)) >>
      chk;
  // NAME_D3D12_OBJECT(pCommandList);
  pCommandList->Close();
  imguiCommandList->Close();
}

FrameResource::~FrameResource() {
  renderTarget.Reset();
  depthBuffer.Reset();
  pCommandList.Reset();
  commandAllocator.Reset();
}

void FrameResource::UpdateResolution(Pipeline &PipeLine) {

  PipeLine.swapChain->GetBuffer(uFID, IID_PPV_ARGS(&renderTarget)) >> chk;
  PipeLine.pDevice->CreateRenderTargetView(renderTarget.Get(), nullptr,
                                           RenderTargetView);

  // DSV
  depthBuffer.Reset();
  {
    auto heapprop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, PipeLine.windowResolution.wr.right,
        PipeLine.windowResolution.wr.bottom, 1, 0, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    PipeLine.pDevice->CreateCommittedResource(
        &heapprop, D3D12_HEAP_FLAG_NONE, &resDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr,
        IID_PPV_ARGS(&depthBuffer)) >>
        chk;
  }
  PipeLine.pDevice->CreateDepthStencilView(depthBuffer.Get(), nullptr,
                                           DepthStencilView);
}

void FrameResource::PopulateCommandList(
    CD3DX12_RECT *scissorRect, CD3DX12_VIEWPORT *viewport,
    std::unordered_map<umID, RenderBuffers> &Buffers,
    std::vector<ID3D12DescriptorHeap *> &ppHeaps,
    D3D12_GPU_DESCRIPTOR_HANDLE &SamplerHeapGpuHandle) {

  commandAllocator->Reset() >> chk;
  pCommandList->Reset(commandAllocator.Get(), pPipelineState) >> chk;

  {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget.Get(), D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    pCommandList->ResourceBarrier(1, &barrier);
  }

  pCommandList->SetGraphicsRootSignature(pRootSignature);

  pCommandList->SetDescriptorHeaps(ppHeaps.size(), ppHeaps.data());

  pCommandList->RSSetViewports(1, viewport);
  pCommandList->RSSetScissorRects(1, scissorRect);

  pCommandList->OMSetRenderTargets(1, &RenderTargetView, FALSE,
                                   &DepthStencilView);
  constexpr FLOAT clearColor[] = {0, 0, 0};
  pCommandList->ClearRenderTargetView(RenderTargetView, clearColor, 0, nullptr);
  pCommandList->ClearDepthStencilView(DepthStencilView, D3D12_CLEAR_FLAG_DEPTH,
                                      1.f, 0, 0, nullptr);

  pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  pCommandList->SetGraphicsRootDescriptorTable(2, SamplerHeapGpuHandle);
  pCommandList->SetPipelineState(pPipelineState);

  for (auto &Pair : Buffers) {
    auto & Buff = Pair.second;
    pCommandList->IASetIndexBuffer(&Buff.ibuffView);
    pCommandList->IASetVertexBuffers(0, 1, &Buff.vbuffView);
    pCommandList->SetGraphicsRootDescriptorTable(0, Buff.cbvGpuHandle);
    pCommandList->SetGraphicsRootDescriptorTable(1, Buff.srvGpuHandle);
    pCommandList->DrawIndexedInstanced(Buff.IndexCount,Buff.InstanceCount, 0, 0, 0);
  }

  {
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT);
    pCommandList->ResourceBarrier(1, &barrier);
  }

  pCommandList->Close() >> chk;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE FrameResource::GetRTV() {
  return RenderTargetView;
}
