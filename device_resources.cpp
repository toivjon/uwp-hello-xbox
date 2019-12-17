#include "device_resources.h"

using namespace xbox;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		// Set a breakpoint on this line to catch Win32 API errors.
		throw Platform::Exception::CreateException(hr);
	}
}

DeviceResources::DeviceResources() : mCurrentFrame(0)
{
	CreateDeviceResources();
}

void DeviceResources::SetLogicalSize(Size logicalSize)
{
	// recreate window size dependent resources if logical size changes.
	if (logicalSize != mLogicalSize) {
		mLogicalSize = logicalSize;
		CreateWindowResources();
	}
}

void DeviceResources::SetDpi(float dpi)
{
	// ...
}

void DeviceResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	// ...
}

void DeviceResources::ValidateDevice()
{
	// ...
}

bool DeviceResources::IsDeviceRemoved()
{
	return false; // TODO
}

void DeviceResources::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
	// store window reference.
	mWindow = window;

	// store logical size dimensions.
	mLogicalSize = Size(window->Bounds.Width, window->Bounds.Height);

	// (re)create window size dependent resources.
	CreateWindowResources();
}

void DeviceResources::CreateDeviceResources()
{
	// try to create a factory for DXGI instances.
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory)));

	// enumerate adapters and find the one with most dedicated video memory.
	auto maxVideoMemory = 0u;
	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIAdapter4> adapter4;
	for (auto i = 0u; mDxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		// get the adapter descriptor info item.
		DXGI_ADAPTER_DESC1 descriptor;
		adapter->GetDesc1(&descriptor);

		// skip software emulation based adapters.
		if ((descriptor.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			continue;

		// skip adapters that cannot be created.
		if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
			continue;

		// skip adapters which have less memory than the current maximum.
		if (descriptor.DedicatedVideoMemory < maxVideoMemory)
			continue;

		// we found a good candidate as the selected adapter.
		ThrowIfFailed(adapter.As(&adapter4));
	}

	// try to create a new DX12 from the provided adapter.
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mD3dDevice)));

	// create a descriptor for the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDescriptor = {};
	queueDescriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDescriptor.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDescriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDescriptor.NodeMask = 0;

	// try to create a new command queue for the target device.
	ThrowIfFailed(mD3dDevice->CreateCommandQueue(&queueDescriptor, IID_PPV_ARGS(&mCommandQueue)));

	// try to create a new render target view (RTV) heap.
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptor = {};
	rtvDescriptor.NumDescriptors = BUFFER_COUNT;
	rtvDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&rtvDescriptor, IID_PPV_ARGS(&mRtvHeap)));

	// try to create a new depth stencil view (DSV) heap.
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptor = {};
	dsvDescriptor.NumDescriptors = 1;
	dsvDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvDescriptor.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(mD3dDevice->CreateDescriptorHeap(&dsvDescriptor, IID_PPV_ARGS(&mDsvHeap)));

	// initialize a command allocator for each buffer.
	for (auto i = 0; i < BUFFER_COUNT; i++) {
		ThrowIfFailed(mD3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i])));
	}

	// create fence for CPU/GPU synchronization.
	ThrowIfFailed(mD3dDevice->CreateFence(mFenceValues[mCurrentFrame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	mFenceValues[mCurrentFrame]++;

	// create event for CPU/GPU synchronization notification.
	mFenceEvent = CreateEvent(nullptr, false, false, nullptr);
	if (mFenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}
}

void DeviceResources::CreateWindowResources()
{
	// assign viewport definitions based on the assigned window.
	mViewport = {};
	mViewport.MaxDepth = D3D12_MAX_DEPTH;
	mViewport.MinDepth = D3D12_MIN_DEPTH;
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.Width = mWindow->Bounds.Width;
	mViewport.Height = mWindow->Bounds.Height;
}