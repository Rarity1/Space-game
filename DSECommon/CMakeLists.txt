add_library(DSECommon SHARED Source/Engine.cpp Source/EngineTime.cpp Source/ePhysics.cpp Source/Exceptions.cpp Source/FrameResource.cpp Source/Graphics.cpp Source/GraphicsErrors.cpp Source/Keyboard.cpp Source/ModelData.cpp Source/Mouse.cpp Source/ObjectTracking.cpp Source/RStorage.cpp Source/SrvAllocator.cpp Source/Threads.cpp)


target_include_directories(DSECommon PUBLIC ${CMAKE_SOURCE_DIR}/DSECommon/Headers ${DXCHeaders})


get_target_property(DLLNAME DSECommon RUNTIME_OUTPUT_NAME)
add_compile_definitions(DESCDLL=0 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=0 DLLNAME="${DLLNAME}")
set_target_properties(DSECommon PROPERTIES COMPILE_TAGS "-save-temps")

find_library(dxgi NAMES libdxgi.a REQUIRED CONFIG)
find_library(dxguid NAMES libdxguid.a REQUIRED CONFIG)
find_library(d3d12 NAMES libd3d12.a REQUIRED CONFIG)

find_package(DirectXMath REQUIRED CONFIG)
find_package(DirectX-Headers REQUIRED CONFIG)
find_package(DirectXTK12 REQUIRED CONFIG)

target_link_libraries(DSECommon rapidxml ImGui OpenCL::OpenCL OpenCL::Headers OpenCL::HeadersCpp dxgi dxguid d3d12 DirectXMath DirectXTK12 DirectX-Headers ${dxclibs}/dxcompiler.lib )
#dxclib
