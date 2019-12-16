#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#define BUFFER_COUNT 2

namespace xbox
{
	class DeviceResources
	{
	public:
		DeviceResources();
		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetDpi(float dpi);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void ValidateDevice();
		bool IsDeviceRemoved();
		void SetWindow(Windows::UI::Core::CoreWindow^ window);
	private:
		void CreateDeviceResources();
	private:
		UINT64 mCurrentFrame;

		// Direct3D objects
		Microsoft::WRL::ComPtr<IDXGIFactory4>			mDxgiFactory;
		Microsoft::WRL::ComPtr<ID3D12Device>			mD3dDevice;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>		mCommandQueue;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	mRtvHeap;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>	mDsvHeap;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  mCommandAllocators[BUFFER_COUNT];

		// CPU/GPU synchronization
		Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
		UINT64								mFenceValues[BUFFER_COUNT];
		HANDLE								mFenceEvent;
	};
}
