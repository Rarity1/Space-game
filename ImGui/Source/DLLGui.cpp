#include "DLLGui.h"

#ifndef IMGUI_DISABLE
imguid::imguid(HWND hWnd, ImGui_ImplDX12_InitInfo* DX12)
{
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));




	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();

	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);



	ImGui_ImplDX12_Init(DX12);

}

imguid::~imguid()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// (Your code process and dispatch Win32 messages)
void imguid::imPrepare()
{

	bool show_demo_window = true;
// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	
	
	ImGui::ShowDemoWindow(&show_demo_window); // Show demo window! :)
	
	
	ImGui::Render();
}

// (Your code clears your framebuffer, renders your other stuff etc.)
// Rendering
void imguid::imPopulateCommand(CD3DX12_CPU_DESCRIPTOR_HANDLE RTV, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pCbvSrvDescriptorHeap, ID3D12Resource* rtvResource, ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator)
{
	//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);
	commandAllocator->Reset();
	commandList->Reset(commandAllocator, ((ImGui_ImplDX12_Data*)ImGui::GetIO().BackendRendererUserData)->pPipelineState);
	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			rtvResource,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ResourceBarrier(1, &barrier);
	}

	// Render Dear ImGui graphics
	//const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

	//CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameID, rtvDescriptorSize);
	//cmdLst->ClearRenderTargetView(rtv, clear_color_with_alpha, 0, nullptr);
	commandList->OMSetRenderTargets(1, &RTV, FALSE, nullptr);

	commandList->SetDescriptorHeaps(1, pCbvSrvDescriptorHeap.GetAddressOf());

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			rtvResource,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		commandList->ResourceBarrier(1, &barrier);
	}
	commandList->Close();

	// (Your code calls swapchain's Present() function)
}

LRESULT imguid::ImGuiProcHndl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
#endif