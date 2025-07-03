#include "DLLGui.h"


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
void imguid::imStart()
{

	bool show_demo_window = true;
// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::ShowDemoWindow(&show_demo_window); // Show demo window! :)
	ImGui::Render();
}

// Rendering
void imguid::imEnd(ID3D12GraphicsCommandList* cmdLst, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& commandAllocator, UINT frameID, std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>& renderTargets, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& rtvDescriptorHeap, UINT rtvDescriptorSize, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> pCbvSrvDescriptorHeap)
{
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 0.00f);

// (Your code clears your framebuffer, renders your other stuff etc.)



	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[frameID].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		cmdLst->ResourceBarrier(1, &barrier);
	}
	// Render Dear ImGui graphics
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameID, rtvDescriptorSize);
	//cmdLst->ClearRenderTargetView(rtv, clear_color_with_alpha, 0, nullptr);
	cmdLst->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

	cmdLst->SetDescriptorHeaps(1, pCbvSrvDescriptorHeap.GetAddressOf());

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdLst);

	{
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			renderTargets[frameID].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
		);
		cmdLst->ResourceBarrier(1, &barrier);
	}

	// (Your code calls swapchain's Present() function)
}

LRESULT imguid::ImGuiProcHndl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
