# CMakeList.txt : CMake project for Space Game (cmake), include source and define
# project specific logic here.
#

cmake_minimum_required(VERSION 4.3.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/game)    # For .exe and .dll (Windows)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/game)    # For .so/.dylib/.dll
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/game)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_LIBRARY_PATH "C:\\msys64\\ucrt64\\lib")



project ("Space Game")

include(FetchContent)
include(cmake/CPM.cmake)


FetchContent_Declare(
DXCNuget
URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DXC/1.9.2602.24
)
FetchContent_MakeAvailable(DXCNuget)
find_library(dxil NAMES dxil.lib REQUIRED CONFIG)
set(dxclibs "${dxcnuget_SOURCE_DIR}/build/native/lib/x64/")

set(DXCHeaders ${dxcnuget_SOURCE_DIR}/build/native/include)
set(DIRECTX_DXC_TOOL ${dxcnuget_SOURCE_DIR}/build/native/bin/x64/dxc.exe)

CPMAddPackage("gh:CodeFinder2/rapidxml@1.13")
CPMAddPackage("gh:microsoft/DirectX-Headers#main")
CPMAddPackage("gh:microsoft/DirectXMath#main")
add_library(Microsoft::DirectXMath ALIAS DirectXMath)
#CPMAddPackage("gh:microsoft/DirectXShaderCompiler#release-1.9.2602" )
CPMAddPackage("gh:microsoft/DirectXTK12#main")

CPMAddPackage("gh:KhronosGroup/OpenCL-Headers@2026.05.29")
CPMAddPackage("gh:KhronosGroup/OpenCL-ICD-Loader@2026.05.29")
CPMAddPackage("gh:KhronosGroup/OpenCL-CLHPP@2026.05.29")


add_subdirectory(ImGui)
add_subdirectory(DSECommon)
add_subdirectory(Editor)


file(COPY "OpenCL" DESTINATION ${CMAKE_BINARY_DIR}/game)
file(COPY "models" DESTINATION ${CMAKE_BINARY_DIR}/game)
file(COPY "shaders" DESTINATION ${CMAKE_BINARY_DIR}/game)
file(COPY "textures" DESTINATION ${CMAKE_BINARY_DIR}/game)

