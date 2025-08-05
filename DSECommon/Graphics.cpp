
#include "Graphics.h"



Graphics::Graphics(HWND& hWnd, RECT& WindowRect)
	:
	//width(w),
	//height(h),
	windowResolution(WindowRect),
	hwnd(hWnd),
	cframeIndex(0),
	rStorage(std::make_unique<RStorage>())
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
	//Just some stuff
	XMStoreFloat4x4(&fovPerspective, DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV2, float(windowResolution.right - windowResolution.left) / float(windowResolution.bottom - windowResolution.top), 0.1f, 100000.0f));
	scissorRect = CD3DX12_RECT(0, 0, windowResolution.right, windowResolution.bottom);
	viewport = CD3DX12_VIEWPORT(0.f, 0.f, windowResolution.right, windowResolution.bottom);

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
	{
		DXGI_SWAP_CHAIN_DESC1 sd = {};
		sd.BufferCount = bufferCount;
		sd.Width = 0;
		sd.Height = 0;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		sd.SampleDesc.Count = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		dxgiFactory->CreateSwapChainForHwnd(
			commandQueue.Get(),
			hwnd,
			&sd,
			nullptr,
			nullptr,
			(IDXGISwapChain1**)swapChain.GetAddressOf()
		) >> chk;
	}


	dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER) >> chk;
	cframeIndex = swapChain->GetCurrentBackBufferIndex();






	//RTV descriptors and buffer references
	dsvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	;

	rtvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV);







	//Sampler descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
		dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		dsc.NumDescriptors = 1;
		dsc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		pDevice->CreateDescriptorHeap(&dsc, IID_PPV_ARGS(&pSamplerDescriptorHeap)) >> chk;
		D3D12_SAMPLER_DESC sDesc = {};

		//why so blurry anisotropic? :c
		sDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		sDesc.MaxAnisotropy = 16;
		sDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		sDesc.MipLODBias = 0;
		//sDesc.MinLOD = 0;
		//sDesc.MaxLOD = 100;
		pDevice->CreateSampler(&sDesc, pSamplerDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
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
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_NONE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[3]{};
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

		const D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;

		rootSignatureDesc.Init_1_1(rootSignatureDesc, (UINT)std::size(rootParameters), rootParameters,
			0, nullptr, rootSignatureFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
		Microsoft::WRL::ComPtr<ID3D10Blob> errorBlob;
		D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_1, &signatureBlob, &errorBlob) >> chk;

		pDevice->CreateRootSignature(0, signatureBlob->GetBufferPointer(),
			signatureBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSignature)) >> chk;
		NAME_D3D12_OBJECT(pRootSignature);
	}

	{
		//shader compiler
		Microsoft::WRL::ComPtr<IDxcCompiler3> compiler(nullptr);
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)) >> chk;
		Microsoft::WRL::ComPtr<IDxcUtils> utils(nullptr);
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)) >> chk;
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> handler(nullptr);
		utils->CreateDefaultIncludeHandler(&handler) >> chk;

		Microsoft::WRL::ComPtr<IDxcBlob> vsShaderblob(nullptr);
		Microsoft::WRL::ComPtr<IDxcBlob> psShaderblob(nullptr);


		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceblobvs(nullptr);
		Microsoft::WRL::ComPtr<IDxcBlobEncoding> sourceblobps(nullptr);

		utils->LoadFile(L"shaders\\VertexShader.hlsl", nullptr, &sourceblobvs) >> chk;
		utils->LoadFile(L"shaders\\PixelShader.hlsl", nullptr, &sourceblobps) >> chk;

		{
			auto compilefunc = [this](Microsoft::WRL::ComPtr<IDxcBlobEncoding>& sourceblob, Microsoft::WRL::ComPtr<IDxcIncludeHandler>& handler, Microsoft::WRL::ComPtr<IDxcCompiler3>& compiler, std::wstring CompileVersion) {
				LPCWSTR args[]{
					L"",
					L"-E", L"main",
					L"-T", CompileVersion.c_str(),
					DXC_ARG_ALL_RESOURCES_BOUND,
#ifdef _DEBUG
					DXC_ARG_DEBUG,
					DXC_ARG_SKIP_OPTIMIZATIONS,
#else 
					DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
					DXC_ARG_WARNINGS_ARE_ERRORS,
					L"-Qstrip_reflect",
					L"-Qstrip_debug",
				};


				DxcBuffer buffer{};
				buffer.Encoding = DXC_CP_ACP;
				buffer.Ptr = sourceblob->GetBufferPointer();
				buffer.Size = sourceblob->GetBufferSize();
				Microsoft::WRL::ComPtr<IDxcResult> results(nullptr);
				compiler->Compile(&buffer, args, _countof(args), handler.Get(), IID_PPV_ARGS(&results)) >> chk;
				Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors(nullptr);

				results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr) >> chk;
				if (errors && errors->GetStringLength()) {
					OutputDebugStringA("\n Shader compilation error: \n");
					OutputDebugStringA(errors->GetStringPointer());
				}
				else {
					OutputDebugStringA("\n Shader compilation Succeeded \n");

				}
				HRESULT status{ S_OK };
				results->GetStatus(&status) >> chk;
				status >> chk;

				Microsoft::WRL::ComPtr<IDxcBlob> Shaderblob(nullptr);

				results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&Shaderblob), nullptr) >> chk;
				return Shaderblob;
			};

			vsShaderblob = compilefunc(sourceblobvs, handler, compiler, L"vs_6_6");
			psShaderblob = compilefunc(sourceblobps, handler, compiler, L"ps_6_6");

		}

		//Input layout and shaders
		D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
					{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
					{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA , 0 },
		};

		Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderBlob;
		Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderBlob;
		//Load vertex shader
		//D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderBlob) >> chk;
		//Load Pixel shader
		//D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderBlob) >> chk;
		// filling pso structure 
		pipelineStateStream.RootSignature = pRootSignature.Get();
		pipelineStateStream.InputLayout = { inputLayout, (UINT)std::size(inputLayout)};
		pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE((ID3DBlob*)vsShaderblob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE((ID3DBlob*)psShaderblob.Get());
		pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		pipelineStateStream.RTVFormats = {
				.RTFormats{ DXGI_FORMAT_R8G8B8A8_UNORM },
				.NumRenderTargets = 1,
		};
		const D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
				sizeof(PipelineStateStream), &pipelineStateStream
		};
		pDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState)) >> chk;
		NAME_D3D12_OBJECT(pPipelineState);


	}




	//ImGuiSetup req
	//imHAllocator = std::make_shared<DescriptorHeapAllocator>(pDevice, 64);
	objectAllocator = std::make_unique<DescriptorHeapAllocator>(pDevice);


	pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&imGuicommandAllocator)) >> chk;
	pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		imGuicommandAllocator.Get(), nullptr, IID_PPV_ARGS(&imGuicommandList)) >> chk;
	NAME_D3D12_OBJECT(imGuicommandList);
	//Close the command list so it can be reset at top of draw loop
	imGuicommandList->Close() >> chk;
	
	ImGuiInfo.Device = pDevice.Get();
	ImGuiInfo.CommandQueue = commandQueue.Get();
	ImGuiInfo.NumFramesInFlight = bufferCount;
	ImGuiInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	//ImGuiInfo.SrvDescriptorHeap = imHAllocator->SrvDescHeap;
	//std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE*, D3D12_GPU_DESCRIPTOR_HANDLE*)> Alloc = std::bind(&DescriptorHeapAllocator::Alloc, imHAllocator, std::placeholders::_1, std::placeholders::_2);
	std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE*, D3D12_GPU_DESCRIPTOR_HANDLE*)> Alloc{[this](D3D12_CPU_DESCRIPTOR_HANDLE* cpuh, D3D12_GPU_DESCRIPTOR_HANDLE* gpuh){objectAllocator->Alloc(cpuh, gpuh);}};

	//std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)> Free = std::bind(&DescriptorHeapAllocator::Free, imHAllocator, std::placeholders::_1, std::placeholders::_2);
	std::function<void(D3D12_CPU_DESCRIPTOR_HANDLE, D3D12_GPU_DESCRIPTOR_HANDLE)> Free{ [this](D3D12_CPU_DESCRIPTOR_HANDLE cpuh, D3D12_GPU_DESCRIPTOR_HANDLE gpuh) {objectAllocator->Free(cpuh, gpuh); }};

	ImGuiInfo.SrvDescriptorAllocFn = Alloc;
	ImGuiInfo.SrvDescriptorFreeFn = Free;


	iGui = std::make_unique<imguid>(hwnd, &ImGuiInfo);
	CreateFrameResources();

}


//Rework to use loaded models instead of tracked objects
void Graphics::CreateBuffers(std::list<Object>& trackedObjects) {
	DirectX::ResourceUploadBatch upload(pDevice.Get());
	upload.Begin();

	//This is wasteful if multiple objects share the same model also this can be multithreaded
	for (auto& bm : trackedObjects) {
		if (bm.model->vbuffer == nullptr) {
			UINT vbuffSize = bm.model->uData->sIndex.size() * sizeof(ReadXML::Vertex);
			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vbuffSize);
				pDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&bm.model->vbuffer)
				) >> chk;
				bm.model->vbuffView = D3D12_VERTEX_BUFFER_VIEW{
					.BufferLocation = bm.model->vbuffer->GetGPUVirtualAddress(),
					.SizeInBytes = vbuffSize,
					.StrideInBytes = (UINT)sizeof(ReadXML::Vertex)
				};
			}

			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vbuffSize);
				pDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr, IID_PPV_ARGS(&bm.model->uvbuffer)
				) >> chk;
			}

			UINT ibuffSize = bm.model->uData->sIndex.size() * sizeof(uint32_t);

			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_DEFAULT };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(ibuffSize);
				pDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_COMMON,
					nullptr,
					IID_PPV_ARGS(&bm.model->ibuffer)
				) >> chk;
				bm.model->ibuffView = D3D12_INDEX_BUFFER_VIEW{
					.BufferLocation = bm.model->ibuffer->GetGPUVirtualAddress(),
					.SizeInBytes = (UINT)std::size(bm.model->uData->sIndex) * (UINT)sizeof(uint32_t),
					.Format = DXGI_FORMAT_R32_UINT
				};
			}

			{
				const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
				const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(ibuffSize);
				pDevice->CreateCommittedResource(
					&heapProps,
					D3D12_HEAP_FLAG_NONE,
					&resourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr, IID_PPV_ARGS(&bm.model->uibuffer)
				) >> chk;
			}
			{
				{

					ReadXML::Vertex* mappedVertexData = nullptr;
					bm.model->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
					WORD* mappedIndexData = nullptr;
					bm.model->uibuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndexData)) >> chk;

					memcpy(mappedVertexData, bm.model->uData->MappedVertices.data(), vbuffSize);
					memcpy(mappedIndexData, bm.model->uData->sIndex.data(), ibuffSize);

				}
				bm.model->uvbuffer->Unmap(0, nullptr);
				bm.model->uibuffer->Unmap(0, nullptr);


			}


		}
		if (bm.curTexture.string() != "") {
			CreateDDSTextureFromFile(pDevice.Get(), upload, bm.curTexture.wstring().c_str(), bm.tbuffer.ReleaseAndGetAddressOf()) >> chk;
		}
		else {
			CreateDDSTextureFromMemory(pDevice.Get(), upload, missing_dds, missing_dds_size, bm.tbuffer.ReleaseAndGetAddressOf()) >> chk;
		}

		UpdBuffer(bm, commandList, pDevice, commandAllocator, commandQueue);

		{
			const CD3DX12_HEAP_PROPERTIES heapProps{ D3D12_HEAP_TYPE_UPLOAD };
			const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(bm.cbvData) + (256 - (sizeof(bm.cbvData) % 256)));
			pDevice->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&bm.cbvwriteBuffer)) >> chk;
		}

		CD3DX12_RANGE readRange(0, 0);
		bm.cbvwriteBuffer->Map(0, &readRange, reinterpret_cast<void**>(std::addressof(bm.ptrcbvData))) >> chk;
	}
	upload.End(commandQueue.Get());


	//Need to rewrite this so that similar models can have CBV in order in memory.
	for (auto & tO : trackedObjects) {
		// Describe and create a constant buffer view (CBV).
		auto size = sizeof(tO.cbvData) + (256 - (sizeof(tO.cbvData) % 256));
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = tO.cbvwriteBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = size;
		objectAllocator->Alloc(&tO.cbvCpuHandle, &tO.cbvGpuHandle);
		pDevice->CreateConstantBufferView(&cbvDesc, tO.cbvCpuHandle);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//Remember to change to bc7 textures when gimp gets support
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = -1;

		objectAllocator->Alloc(&tO.srvCpuHandle, &tO.srvGpuHandle);
		pDevice->CreateShaderResourceView(tO.tbuffer.Get(), &srvDesc, tO.srvCpuHandle);


	}
}

void Graphics::UpdBuffer(Object& bm, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Device> pDevice, Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue) {
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->vbuffer.Get(),
			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->ibuffer.Get(),
			D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->ResourceBarrier(1, &barrier);
	}
	commandList->CopyResource(bm.model->vbuffer.Get(), bm.model->uvbuffer.Get());
	commandList->CopyResource(bm.model->ibuffer.Get(), bm.model->uibuffer.Get());
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->vbuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
	{
		const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			bm.model->ibuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		commandList->ResourceBarrier(1, &barrier);
	}
}


void Graphics::lModel(Object& obj, UINT umID) noexcept
{

		auto ReadData = rStorage->GetModel(umID);
		obj.model = rStorage->loadModel(*ReadData);
		obj.curTexture = rStorage->getTexture(ReadData->name);
}

void Graphics::LoadResources(std::list<Object>& trackedObjects, Tracker& oTracker)
{
	

	std::for_each(trackedObjects.begin(), trackedObjects.end(), [this, &oTracker](Object& e) {
		lModel(e, oTracker.getModelID(e.UOID));
	});

	commandAllocator->Reset() >> chk;
	commandList->Reset(commandAllocator.Get(), nullptr) >> chk;

	CreateBuffers(trackedObjects);


	commandList->Close() >> chk;

	{
		ID3D12CommandList* const commandLists[] = { commandList.Get() };
		commandQueue->ExecuteCommandLists(std::size(commandLists), commandLists);
		// insert fence to detect when upload is complete 
		commandQueue->Signal(fence.Get(), ++fenceValue) >> chk;
		fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
		if (WaitForSingleObject(fenceEvent, INFINITE) == WAIT_FAILED) {
			GetLastError() >> chk;
		}
	}
}

void Graphics::UpdateLocalTransform(Object& bm)
{
	using namespace DirectX;
	if (std::strstr(bm.model->uData->bdata[0].name.c_str(), "placeholder"))
		return;
	XMFLOAT4X4 temp{ 1.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,1.f,0.f,0.f,0.f,0.f,1.f };
	XMStoreFloat4x4(&bm.model->uData->ndata.LocalTransform, XMLoadFloat4x4(&temp) * XMLoadFloat4x4(&bm.model->uData->ndata.matrix));

	for (auto& c : bm.model->uData->ndata.children) {
		RecurLTrans(c, &bm.model->uData->ndata);
	}
	
	for (auto& P : bm.model->uData->ndata.aChildren) {
		for (auto& n : P->aChildren) {
			RecurLTrans(n, P);
		}
	}
}

void Graphics::RecurLTrans(ReadXML::Node* n, ReadXML::Node* P) {
	XMStoreFloat4x4(&n->LocalTransform, XMMatrixMultiply(XMLoadFloat4x4(&P->LocalTransform), XMLoadFloat4x4(&n->matrix)));
}

//Fix model updates
void Graphics::UpdateModel(Object* bm) {
	auto GlobITrans = XMMatrixInverse(nullptr, XMLoadFloat4x4(&bm->model->uData->ndata.matrix));
	if (!std::strstr(bm->model->uData->bdata[0].name.c_str(), "placeholder"))
	for (auto& b : bm->model->uData->bdata) {
		//XMStoreFloat4x4(&b.finalTransform, XMLoadFloat4x4(&b.matrix) * XMLoadFloat4x4(&b.node->LocalTransform) * GlobITrans);
	}
	//auto& vdata = bm->model->uData->Vertdata;
	//auto& idata = bm->model->uData->idata;
	ReadXML::Vertex* mappedVertexData = nullptr;
	bm->model->uvbuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertexData)) >> chk;
	//Fix animations
	/*for (auto& m : modelVect) {
		//UpdateLocalTransform(m);
		if (m.model->animate.load()) {
			lModels->UpdBuffer(m, commandList, pDevice, commandAllocator, commandQueue);
			m.model->animate.store(false);
		}
	}
	if (std::size(bm->uData->bdata) > 0) {
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
	}
		for (auto i = 0; i < std::size(vdata); i++) {
		//memcpy(&mappedVertexData[i], &vdata[i], sizeof(ReadXML::Vertex));
	}
	
	*/
	


	bm->model->uvbuffer->Unmap(0, nullptr);
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
	backBuffers.resize(0);
	scissorRect = CD3DX12_RECT(0, 0, windowResolution.right, windowResolution.bottom);
	viewport = CD3DX12_VIEWPORT(0.f, 0.f, windowResolution.right, windowResolution.bottom);

	//rtv Descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsc = {};
		dsc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		dsc.NumDescriptors = bufferCount;
		pDevice->CreateDescriptorHeap(&dsc, IID_PPV_ARGS(&rtvDescriptorHeap)) >> chk;

	}
	//DSV des heap
	{
		const D3D12_DESCRIPTOR_HEAP_DESC desc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.NumDescriptors = bufferCount,
		};
		pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvDescriptorHeap)) >> chk;
	}

	for (uint8_t i = 0; i < bufferCount; i++)
	{
		backBuffers.emplace_back(std::make_unique<FrameResource>(
			this, i
		));

		//pFrameResource.InitBundle(pDevice.Get(), pipelineState.Get(), i, srvDescriptorHeap.Get(), srvDescriptorSize, samplerDescriptorHeap.Get(), samplerDescriptorSize, rootSignature.Get(), modelVect);

	}
	updateResolution.store(false);

}

void Graphics::UpdateFrameResources()
{
	if (updateResolution.load()) {
		//Swapchain buffers wont resize until all buffers are unused.
		for (auto& b : backBuffers) {
			b->renderTarget.Reset();
		}
		swapChain->ResizeBuffers(bufferCount, windowResolution.right, windowResolution.bottom, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) >> chk;
		scissorRect = CD3DX12_RECT(0, 0, windowResolution.right, windowResolution.bottom);
		viewport = CD3DX12_VIEWPORT(0.f, 0.f, windowResolution.right, windowResolution.bottom);

		for (auto& b : backBuffers) {
			b->UpdateResolution(this);
		}
		updateResolution.store(false);

	}

}

void Graphics::UpdateConstantBuffers(DirectX::FXMMATRIX view, DirectX::CXMMATRIX projection, std::list<Object>& trackedObjects)
{

	using namespace DirectX;
	auto tO = trackedObjects.begin();
	for (auto i = 0; i < std::size(trackedObjects); i++)
	{
		tO->viewMtx.lock();
		XMStoreFloat4x4(&tO->ptrcbvData->cbvMatrix, XMMatrixTranspose(XMLoadFloat4x4(&tO->vMatrix) * view * projection));
		tO->viewMtx.unlock();
		// Copy this matrix into the appropriate location in the upload heap subresource.
		//memcpy(cbvbuff[i], &mvp, sizeof(mvp));
		tO++;
	}
}


Graphics::~Graphics() {
	//iGui Is not very graceful to shutdown. Need to close it first before graphics unload.
	iGui.reset();
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

		backBuffers.resize(0);
		pDevice.Reset();
	}


}

void Graphics::Update(std::list<Object>& trackedObjects) {
	UpdateFrameResources();

	XMStoreFloat4x4(&fovPerspective, DirectX::XMMatrixPerspectiveFovRH(DirectX::XM_PIDIV2, float(windowResolution.right - windowResolution.left) / float(windowResolution.bottom - windowResolution.top), 0.1f, 1000000.0f));

	iGui->imPrepare();
	curCamera.posMtx->lock();
	curCamera.cmatrix = DirectX::XMMatrixLookToRH(XMLoadFloat3(curCamera.position), XMLoadFloat4(&curCamera.rotation), XMLoadFloat4(&curCamera.upDirection));
	curCamera.posMtx->unlock();
	UpdateConstantBuffers(curCamera.cmatrix, XMLoadFloat4x4(&fovPerspective), trackedObjects);
}


void Graphics::RenderFrame(std::list<Object>& trackedObjects) {



	fenceValue = fence->GetCompletedValue();

	auto lastFrame = cframeIndex;
	cframeIndex = swapChain->GetCurrentBackBufferIndex();


	auto& cframeBuffer = *backBuffers[cframeIndex];

	cframeBuffer.commandAllocator->Reset() >> chk;
	cframeBuffer.pCommandList->Reset(cframeBuffer.commandAllocator.Get(), pPipelineState.Get()) >> chk;


	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			cframeBuffer.renderTarget.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cframeBuffer.pCommandList->ResourceBarrier(1, &barrier);
	}




	cframeBuffer.PopulateCommandList( scissorRect, viewport, trackedObjects);



	iGui->imPopulateCommand(cframeBuffer.pCommandList.Get());

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			cframeBuffer.renderTarget.Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		cframeBuffer.pCommandList->ResourceBarrier(1, &barrier);
	}

	cframeBuffer.pCommandList->Close() >> chk;




	std::vector<ID3D12CommandList*> commandLists = { cframeBuffer.pCommandList.Get()};
	commandQueue->ExecuteCommandLists(commandLists.size(), commandLists.data());

	//Vsync off. add toggle here for changing vsync
	swapChain->Present(0, 512) >> chk;

	//Something something prevent something idk
	if (backBuffers[lastFrame]->fenceValue != fenceValue) {
		fence->SetEventOnCompletion(backBuffers[lastFrame]->fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}


	cframeBuffer.fenceValue = fenceValue + 1;
	commandQueue->Signal(fence.Get(), cframeBuffer.fenceValue) >> chk;
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

