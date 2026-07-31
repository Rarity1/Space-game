#Todo Move graphics to its own library so that the graphics pipeline is sperate from engine logic
add_library(DSECommon SHARED Source/Engine.cpp Source/EngineTime.cpp Source/ePhysics.cpp Source/Exceptions.cpp Source/FrameResource.cpp Source/Graphics.cpp Source/GraphicsErrors.cpp Source/Keyboard.cpp Source/ModelData.cpp Source/Mouse.cpp Source/ObjectTracking.cpp Source/RStorage.cpp Source/SrvAllocator.cpp Source/Threads.cpp)

set(DSEHeaders "${CMAKE_CURRENT_SOURCE_DIR}/Headers" CACHE PATH "Path To DSEHeaders")
target_include_directories(DSECommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Headers)


#get_target_property(DLLNAME DSECommon RUNTIME_OUTPUT_NAME)
add_compile_definitions(DESCDLL=0 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=0 
#DLLNAME=DLLNAME
)
set_target_properties(DSECommon PROPERTIES COMPILE_TAGS "-save-temps")


target_link_libraries(DSECommon rapidxml OpenCL::OpenCL OpenCL::Headers OpenCL::HeadersCpp dxgi
DirectX-Headers DirectXTK12 DirectX-Guids d3d12 DirectXMath ${dxclibs}/dxcompiler.lib ${slang} ImGui)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
#target_link_libraries(DSECommon dxgidebug)
endif()