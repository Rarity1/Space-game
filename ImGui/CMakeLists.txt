add_library(ImGui SHARED Source/DLLGui.cpp Source/imgui.cpp Source/imgui_demo.cpp Source/imgui_draw.cpp Source/imgui_impl_dx12.cpp Source/imgui_impl_win32.cpp Source/imgui_tables.cpp Source/imgui_widgets.cpp)
target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Headers ${DSEHeaders})
add_compile_definitions(DESCGDLL=0 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=0)


target_link_libraries(ImGui dwmapi dxgi DirectX-Headers DirectXTK12 DirectX-Guids d3d12 DirectXMath d3dcompiler)
