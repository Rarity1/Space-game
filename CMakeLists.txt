# CMakeList.txt : CMake project for Space Game (cmake), include source and define
# project specific logic here.
#

cmake_minimum_required(VERSION 4.3.3)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/game)    # For .exe and .dll (Windows)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/game)    # For .so/.dylib/.dll
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/game)
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/out/game")
set(CMAKE_INSTALL_BINDIR "${CMAKE_SOURCE_DIR}/out/game")
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_CXX_SCAN_FOR_MODULES ON)
#set(CMAKE_LIBRARY_PATH "C:\\msys64\\ucrt64\\lib")

if(NOT CMAKE_BUILD_TYPE)
  message("DEBUG BUILD")
  set(CMAKE_BUILD_TYPE Debug)
endif()

project ("Space Game")

include(FetchContent)
include(cmake/CPM.cmake)

#This is only for Directxtk12
FetchContent_Declare(
DXCNuget
URL https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DXC/1.10.2605.24-preview
)
FetchContent_MakeAvailable(DXCNuget)
#find_library(dxil NAMES dxil.lib REQUIRED CONFIG)
set(dxclibs "${dxcnuget_SOURCE_DIR}/build/native/lib/x64/")
set(DXCHeaders ${dxcnuget_SOURCE_DIR}/build/native/include)
include_directories(${DXCHeaders})

set(DIRECTX_DXC_TOOL ${dxcnuget_SOURCE_DIR}/build/native/bin/x64/dxc.exe)


CPMAddPackage("gh:CodeFinder2/rapidxml@1.13")
CPMAddPackage("gh:microsoft/DirectX-Headers#main")
CPMAddPackage("gh:microsoft/DirectXMath#main")
add_library(Microsoft::DirectXMath ALIAS DirectXMath)
#CPMAddPackage("gh:microsoft/DirectXShaderCompiler#release-1.9.2602" )
#CPMAddPackage("gh:shader-slang/slang@2026.12.0.1" )
CPMAddPackage("gh:microsoft/DirectXTK12#main")

include_directories(${Vulkan_INCLUDE_DIRS} ${Vulkan_LIBRARIES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/slang/include/")
find_library(slang NAMES "slang.lib" PATHS ${CMAKE_CURRENT_SOURCE_DIR}/slang/lib/ REQUIRED CONFIG)
find_library(dxcompiler NAMES "dxcompiler.lib" PATHS ${dxclibs} REQUIRED CONFIG)
find_library(DirectX-Guids NAMES libdxguid.a PATHS "C:\\msys64\\ucrt64\\lib" REQUIRED CONFIG)
find_library(dxgi NAMES libdxgi.a PATHS "C:\\msys64\\ucrt64\\lib" REQUIRED CONFIG)
#add_idl(dxgidebug NAMES dxgidebug.idl REQUIRED CONFIG)
find_library(dxguid NAMES libdxguid.a PATHS "C:\\msys64\\ucrt64\\lib" REQUIRED CONFIG)
find_library(d3d12 NAMES libd3d12.a PATHS "C:\\msys64\\ucrt64\\lib" REQUIRED CONFIG)
#find_library(vulkan NAMES libvulkan-1.dll.a PATHS "C:\\msys64\\ucrt64\\lib" REQUIRED CONFIG)

find_package(DirectXMath REQUIRED CONFIG)
find_package(DirectX-Headers REQUIRED CONFIG)

find_package(DirectXTK12 REQUIRED CONFIG)
CPMAddPackage("gh:KhronosGroup/OpenCL-Headers@2026.05.29")
CPMAddPackage("gh:KhronosGroup/OpenCL-ICD-Loader@2026.05.29")
CPMAddPackage("gh:KhronosGroup/OpenCL-CLHPP@2026.05.29")

add_subdirectory(DSECommon)
add_subdirectory(ImGui)
add_subdirectory(Editor) 
install(TARGETS Editor DSECommon ImGui)

#file(COPY "OpenCL" DESTINATION ${CMAKE_BINARY_DIR}/game)
#file(COPY "models" DESTINATION ${CMAKE_BINARY_DIR}/game)
#file(COPY "shaders" DESTINATION ${CMAKE_BINARY_DIR}/game)
#file(COPY "textures" DESTINATION ${CMAKE_BINARY_DIR}/game)
file(GLOB SLANG "slang/bin/*")
file(COPY ${SLANG} DESTINATION ${CMAKE_INSTALL_PREFIX})
file(COPY "OpenCL" DESTINATION ${CMAKE_INSTALL_PREFIX})
file(COPY "models" DESTINATION ${CMAKE_INSTALL_PREFIX})
#file(COPY "shaders" DESTINATION ${CMAKE_INSTALL_PREFIX})
file(COPY "textures" DESTINATION ${CMAKE_INSTALL_PREFIX})
