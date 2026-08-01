#pragma once
#include "CWin.h"
#include <directx/d3dx12.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <wrl/client.h>
#include <vector>

#ifdef DESCGDLL
#define DLLG __declspec( dllexport )
#else
#define DLLG __declspec( dllimport )
#endif
#ifndef IMGUI_DISABLE
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class DLLG imguid {
public:
	imguid(HWND hWnd, ImGui_ImplDX12_InitInfo* DX12);
	~imguid();
	void imPrepare();
	void imPopulateCommand(CD3DX12_CPU_DESCRIPTOR_HANDLE RTV, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pCbvSrvDescriptorHeap, ID3D12Resource* rtvResource, ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator);
	LRESULT ImGuiProcHndl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

#endif