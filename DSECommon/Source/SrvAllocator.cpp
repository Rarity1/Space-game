#include <d3d12.h>
#include <vector>

//This is really stupid but I dont want to edit ImGui
class ExampleDescriptorHeapAllocator
{
	static ID3D12DescriptorHeap* Heap;
	static D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
	static D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	static D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	static UINT                        HeapHandleIncrement;
	static std::vector<int>               FreeIndices;
public:
	static void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc;
		heap->GetDesc(&desc);
		HeapType = desc.Type;
		Heap->GetCPUDescriptorHandleForHeapStart(&HeapStartCpu);
		Heap->GetGPUDescriptorHandleForHeapStart(&HeapStartGpu);
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n - 1);
	}
	static void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	static void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		//_ASSERT(FreeIndices.size() > 0);
		int idx = FreeIndices.back();
		FreeIndices.pop_back();
		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	static void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
		//_ASSERT(cpu_idx == gpu_idx);
		FreeIndices.push_back(cpu_idx);
	}
};