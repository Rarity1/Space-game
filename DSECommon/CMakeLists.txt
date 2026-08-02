
#Relative source path
set(DSECppPath Source/)

set(DSESources
${DSECppPath}Engine.cpp 
${DSECppPath}InputHandler.cpp 
${DSECppPath}EngineTime.cpp 
${DSECppPath}ePhysics.cpp 
${DSECppPath}Exceptions.cpp 
${DSECppPath}FrameResource.cpp 
${DSECppPath}Graphics.cpp 
${DSECppPath}GraphicsErrors.cpp 
${DSECppPath}ModelData.cpp 
${DSECppPath}ObjectTracking.cpp 
${DSECppPath}RStorage.cpp 
${DSECppPath}SrvAllocator.cpp 
${DSECppPath}Threads.cpp
)


add_library(DSECommon SHARED ${DSESources})

set(DSEHeaders "${CMAKE_CURRENT_SOURCE_DIR}/Headers" CACHE PATH "Path To DSEHeaders")
target_include_directories(DSECommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Headers)


#get_target_property(DLLNAME DSECommon RUNTIME_OUTPUT_NAME)
add_compile_definitions(DESCDLL=0 IMGUI_DISABLE_OBSOLETE_FUNCTIONS=0 
#DLLNAME=DLLNAME
)
set_target_properties(DSECommon PROPERTIES COMPILE_TAGS "-save-temps")


target_link_libraries(DSECommon rapidxml OpenCL::OpenCL OpenCL::Headers OpenCL::HeadersCpp dxgi
DirectX-Headers DirectXTK12 DirectX-Guids d3d12 DirectXMath ${dxclibs}/dxcompiler.lib ${slang} ImGui)
