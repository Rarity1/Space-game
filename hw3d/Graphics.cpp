
#include "Graphics.h"


Graphics::Graphics(HWND* hWnd, int height, int width)
	:
	width(width),
	height(height),
	hWnd(hWnd),
	viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
	scissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	rtvDescriptorSize(0),
	CurBackBuffer(0),
	cframeIndex(0),
	cbackBuffer(nullptr),
	lModels(std::make_unique<RStorage>())
{
}


void Graphics::LoadPipeline() {
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debugController0;
	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController1;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController0)) >> chk;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugController1)) >> chk;
	debugController0->EnableDebugLayer();
	debugController1->SetEnableGPUBasedValidation(true);
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	
	//DStorageGetFactory(IID_PPV_ARGS(&pStorage)) >> chk;
	CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)) >> chk;
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&pDevice)) >> chk;
	NAME_D3D12_OBJECT(pDevice);


	//Command Queue
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	
	pDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)) >> chk;
	NAME_D3D12_OBJECT(commandQueue);
	
	/*
	DSTORAGE_QUEUE_DESC qdesc = {};
	qdesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
	qdesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
	qdesc.Priority = DSTORAGE_PRIORITY_NORMAL;
	qdesc.Device = pDevice.Get();

	pStorage->CreateQueue(&qdesc, IID_PPV_ARGS(&storageQueue)) >> chk;
	*/

	//Swap Chain
	Microsoft::WRL::ComPtr<IDXGISwapChain1> TswapChain;
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.BufferCount = bufferCount;
	sd.Width = 0;
	sd.Height = 0;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.SampleDesc.Count = 1;
	dxgiFactory->CreateSwapChainForHwnd(
		commandQueue.Get(),
		*hWnd,
		&sd,
		nullptr,
		nullptr,
		&TswapChain
	) >> chk;

	dxgiFactory->MakeWindowAssociation(*hWnd, DXGI_MWA_NO_ALT_ENTER) >> chk;
	TswapChain.As(&swapChain) >> chk;
	cframeIndex = swapChain->GetCurrentBackBufferIndex();


	//rtv Descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
		dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		dsc.NumDescriptors = bufferCount;
		pDevice->CreateDescriptorHeap(&dsc, IID_PPV_ARGS(&rtvDescriptorHeap)) >> chk;
		rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV
		);
	}
	//DSV des heap
	{
		const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
		const CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
			DXGI_FORMAT_D32_FLOAT,
			width, height,
			1, 0, 1, 0,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
		);
		D3D12_CLEAR_VALUE clearValue{};
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil = { 1.0f, 0 };
		pDevice->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&clearValue, IID_PPV_ARGS(&depthBuffer)
		) >> chk;
		{
			const D3D12_DESCRIPTOR_HEAP_DESC desc = {
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
				.NumDescriptors = 1,
			};
			pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvDescriptorHeap)) >> chk;
		}
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

		// dsv and handle 
		pDevice->CreateDepthStencilView(depthBuffer.Get(), &depthStencilDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());


	}
	
	//sample Descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
		dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		dsc.NumDescriptors = 1;
		dsc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		pDevice->CreateDescriptorHeap(&dsc, IID_PPV_ARGS(&samplerDescriptorHeap)) >> chk;
		samplerDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	}

	
	{
		srvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	//Command Allocator
	pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&commandAllocator)) >> chk;

	//Command List
	pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)) >> chk;
	NAME_D3D12_OBJECT(commandList);

	//Close the command list so it can be reset at top of draw loop
	commandList->Close() >> chk;

	//Fence
	pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) >> chk;

	//Fence signalling event
	fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
	if (!fenceEvent) {
		throw std::runtime_error("Failed to create fence event");
	}

	//Root signaling
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[3]{};
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3]{};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL);

		const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{};
		rootSignatureDesc.Init_1_2(rootSignatureDesc, (UINT)std::size(rootParameters), rootParameters,
			0, nullptr, rootSignatureFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3D10Blob> errorBlob;
		D3DX12SerializeVersionedRootSignature(
			&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0,
			&signatureBlob, &errorBlob) >> chk;

		pDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
			signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)) >> chk;
		NAME_D3D12_OBJECT(rootSignature);
	}

	//Input layout and shaders
	{

		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
		//Load vertex shader
		D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob) >> chk;
		//Load Pixel shader
		D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob) >> chk;
		// filling pso structure 
		pipelineStateStream.RootSignature = rootSignature.Get();
		pipelineStateStream.InputLayout = { inputLayout, (UINT)std::size(inputLayout) };
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateStream.RTVFormats = {
				.RTFormats{ DXGI_FORMAT_R8G8B8A8_UNORM },
				.NumRenderTargets = 1,
		};
		const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
				sizeof(PipelineStateStream), &pipelineStateStream
		};
		pDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pipelineState)) >> chk;
		NAME_D3D12_OBJECT(pipelineState);


	}

	//RTV descriptors and buffer references
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	{

		for (int i = 0; i < bufferCount; i++) {
			swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
			pDevice->CreateRenderTargetView(renderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, rtvDescriptorSize);
		}
	}

}
int Graphics::loadModels(RStorage::eResource* model, bool unique) {
	if (unique) {
		modelVect->emplace_back(model);
	}
	else {
		bool check = false;
		for (auto& m : *modelVect) {
			check = m->model->umID == model->model->umID && !check ? true : check;
		}
		if (!check) {
			modelVect->emplace_back(model);
		}
	}

	return model->model->umID;
}

int Graphics::loadModels(std::vector<RStorage::eResource*>& model, bool replace) {
	if (replace) {
		modelVect = &model;
	}
	else {
		modelVect->append_range(model);
	}
	return 0;
}

void Graphics::LoadResources(int numLoadedSrv)
{
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
		dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		dsc.NumDescriptors = (numLoadedSrv * bufferCount * 2);
		dsc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		pDevice->CreateDescriptorHeap(&dsc, IID_PPV_ARGS(&srvDescriptorHeap)) >> chk;
	}

	lModels->CreateBuffers(*modelVect, commandList, pDevice, commandAllocator, commandQueue, bufferCount);
	
	// submit command list to queue as array with single element 
	{
		ID3D12CommandList* const commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists((UINT)std::size(commandLists), commandLists);
		// insert fence to detect when upload is complete 
		commandQueue->Signal(fence.Get(), ++fenceValue) >> chk;
		fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
		if (WaitForSingleObject(fenceEvent, INFINITE) == WAIT_FAILED) {
			GetLastError() >> chk;
		}
	}
	CreateFrameResources();
}

void Graphics::UpdateLocalTransform(RStorage::eResource* bm)
{
	using namespace DirectX;
	if (std::strstr(bm->model->uData->bdata[0].name.c_str(), "placeholder"))
		return;
	XMFLOAT4X4 temp{ 1.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,1.f };
	XMStoreFloat4x4(&bm->model->uData->ndata.LocalTransform, XMLoadFloat4x4(&temp) * XMLoadFloat4x4(&bm->model->uData->ndata.matrix));

	for (auto& c : bm->model->uData->ndata.children) {
		RecurLTrans(c, &bm->model->uData->ndata);
	}
	
	for (auto& P : bm->model->uData->ndata.aChildren) {
		for (auto& n : P->aChildren) {
			RecurLTrans(n, P);
		}
	}
}

void Graphics::RecurLTrans(ReadX3D::Node* n, ReadX3D::Node* P) {
	XMStoreFloat4x4(&n->LocalTransform, XMMatrixMultiply(XMLoadFloat4x4(&P->LocalTransform), XMLoadFloat4x4(&n->matrix)));
}


void Graphics::UpdateModel(RStorage::eResource* bm) {
	auto GlobITrans = XMMatrixInverse(nullptr, XMLoadFloat4x4(&bm->model->uData->ndata.matrix));
	if (!std::strstr(bm->model->uData->bdata[0].name.c_str(), "placeholder"))
	for (auto& b : bm->model->uData->bdata) {
		//XMStoreFloat4x4(&b.finalTransform, XMLoadFloat4x4(&b.matrix) * XMLoadFloat4x4(&b.node->LocalTransform) * GlobITrans);
	}
	auto& vdata = bm->model->uData->cdata;
	auto& idata = bm->model->uData->idata;
	ReadX3D::Vertex* mappedVertexData = nullptr;
	bm->model->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
	//Fix animations
	/*if (std::size(bm->uData->bdata) > 0) {
		auto tcount = 0;
		for (auto v = 0; v < std::size(idata); v++) {
			auto& weights = bm->uData->weights[v];
			auto vd = vdata[tcount].verts[idata[v].index % 3].position;

			XMFLOAT4 vf = XMFLOAT4{ vd.x,vd.y,vd.z, 1 };
			for (auto w = 0; w < std::size(weights.weight); w++) {
				auto matrix = XMLoadFloat4x4(&bm->uData->bdata[weights.bIndex[w]].finalTransform) * weights.weight[w];
				auto temp = XMVector3TransformNormal(XMLoadFloat4(&vf), (matrix));
				XMStoreFloat4(&vf, temp);
			}
			vdata[tcount].verts[idata[v].index % 3].position.x += vf.x;
			vdata[tcount].verts[idata[v].index % 3].position.y += vf.y;
			vdata[tcount].verts[idata[v].index % 3].position.z += vf.z;
			tcount += idata[v].index % 3 == 0 ? 1 : 0;
		}
	}*/
	
	auto tempcount = 0;
	for (auto v = 0; v < std::size(idata); v++) {
		memcpy(&mappedVertexData[v], &vdata[tempcount].verts[idata[v].index % 3], sizeof(ReadX3D::Vertex));
		tempcount += idata[v].index % 3 == 0 ? 1 : 0;
	}

	bm->model->uvbuffer->Unmap(0, nullptr);
	bm->model->animate.store(true);
}

int Graphics::bIndex(std::vector<int> w, int bInd) {
	int Index = 0;
	for (auto i = 0; i < std::size(w); i++) {
		if (w[i] == bInd) {
			Index = i;
		}
	}
	return Index;
}

void Graphics::CreateFrameResources() {
	for (auto& f : backBuffers)
	{
		delete f;
	}
	backBuffers = {};
	// Initialize each frame resource.
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvHandle(srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < bufferCount; i++)
	{
		FrameResource* pFrameResource = new FrameResource(pDevice.Get(), *modelVect);
		auto temp = 0;
		for (auto& m : *modelVect) {
			// Describe and create a constant buffer view (CBV).
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pFrameResource->openbuffers[temp]->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = sizeof(DirectX::XMFLOAT4X4) + (UINT)192;
			pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvHandle);
			cbvSrvHandle.Offset(srvDescriptorSize);

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = 1;
			pDevice->CreateShaderResourceView(m->model->tbuffer, &srvDesc, cbvSrvHandle);
			cbvSrvHandle.Offset(srvDescriptorSize);
			temp++;


		}

		pFrameResource->InitBundle(pDevice.Get(), pipelineState.Get(), i, srvDescriptorHeap.Get(), srvDescriptorSize, samplerDescriptorHeap.Get(), samplerDescriptorSize, rootSignature.Get(), *modelVect);

		backBuffers.emplace_back(pFrameResource);

	}
}

void Graphics::PopCommandList(FrameResource* backBuffer) {
	
	using namespace DirectX;
	cbackBuffer->commandAllocator->Reset() >> chk;
	commandList->Reset(cbackBuffer->commandAllocator.Get(), pipelineState.Get()) >> chk;
	for (auto& m : *modelVect) {
		UpdateLocalTransform(m);
		if (m->model->animate.load()) {
			lModels->UpdBuffer(m, commandList, pDevice, commandAllocator, commandQueue);
			m->model->animate.store(false);
		}
	}
	commandList->SetGraphicsRootSignature(rootSignature.Get());

	ID3D12DescriptorHeap* ppHeaps[] = { srvDescriptorHeap.Get(), samplerDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[cframeIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), cframeIndex, rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
	const FLOAT clearColor[] = {
		sin(2.f + 1.f) / 2.f + 0.5f,
		sin(3.f + 2.f) / 2.f + 0.5f,
		sin(5.f + 3.f) / 2.f + 0.5f
	};
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

	//Bundle execution?

	commandList->ExecuteBundle(backBuffer->bundle.Get());
	//backBuffer->PopulateCommandList(commandList.Get(), pipelineState.Get(), CurBackBuffer, srvDescriptorHeap.Get(), srvDescriptorSize, samplerDescriptorHeap.Get(), rootSignature.Get(), lModels->modelVect);


	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[cframeIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		commandList->ResourceBarrier(1, &barrier);
	}
	commandList->Close() >> chk;
}

Graphics::~Graphics() {
	if (pDevice != nullptr) {
		const UINT64 fencev = fenceValue;
		const UINT64 lastCompletedFence = fence->GetCompletedValue();
		commandQueue->Signal(fence.Get(), fenceValue);
		fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
		fenceValue++;
		if (lastCompletedFence < fencev)
		{
			GetLastError() >> chk;
		}
		CloseHandle(fenceEvent);

		for (auto& f : backBuffers)
		{
			delete f;
		}

		pDevice.Reset();
	}


}



void Graphics::RenderFrame() {
	umodel.lock();
	const UINT64 lastCompletedFence = fence->GetCompletedValue();
	CurBackBuffer = (CurBackBuffer + 1) % bufferCount;
	cbackBuffer = backBuffers[CurBackBuffer];
	if (cbackBuffer->fenceValue != 0 && cbackBuffer->fenceValue > lastCompletedFence) {
		fenceValue++;
		fence->SetEventOnCompletion(cbackBuffer->fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
	curCamera.cmatrix = DirectX::XMMatrixLookToRH(XMLoadFloat3(curCamera.position), XMLoadFloat4(&curCamera.rotation), XMLoadFloat4(&curCamera.upDirection));
	cbackBuffer->UpdateConstantBuffers(curCamera.cmatrix,
		DirectX::XMMatrixPerspectiveFovRH(1.333f, float(width) / float(height), 0.1f, 100000.0f), *modelVect);
	umodel.unlock();
	PopCommandList(cbackBuffer);
	
	ID3D12CommandList* commandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);


	swapChain->Present(0, 4) >> chk;

	cframeIndex = swapChain->GetCurrentBackBufferIndex();
	cbackBuffer->fenceValue = ++fenceValue;
	commandQueue->Signal(fence.Get(), ++fenceValue) >> chk;
}


float Graphics::Max(float number, float maximum)
{
	float rtrn = number;
	if (number > maximum) {
		rtrn = maximum;
	}
	return rtrn;
}



float Graphics::Min(float minimum, float number)
{
	float rtrn = number;
	if (number < minimum) {
		rtrn = minimum;
	}
	return rtrn;
}
float Graphics::RotateHelper(float& rNumber)
{
	if (rNumber > Max(rNumber, DirectX::XM_2PI)) {
		return 0;
	}
	if (rNumber < Min(-DirectX::XM_2PI, rNumber)) {
		return 0;
	}
	return rNumber;
}

