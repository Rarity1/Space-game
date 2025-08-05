#pragma once
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3dx12.h>
#include <wrl/client.h>
#include <vector>


#ifdef DESCGDLL
#define DLLG __declspec( dllexport )
#else
#define DLLG __declspec( dllimport )
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


class DLLG imguid {
public:
	imguid(HWND hWnd, ImGui_ImplDX12_InitInfo* DX12);
	~imguid();
	void imPrepare();
	void imPopulateCommand(ID3D12GraphicsCommandList* cmdLst);
	LRESULT ImGuiProcHndl(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

