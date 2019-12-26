#pragma once

#include <agile.h>
#include <dxgi1_6.h>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

#define BUFFER_COUNT 2

// ============================================================================
// A renderer object to draw stuff on the screen.
//
// Renderer is used to perform draw operations to make things visible. Here we
// have a super simple DX12 renderer that draws a simple triangle on the view.
// ============================================================================
ref class Renderer sealed
{
public:
	Renderer();
	void SetWindow(Windows::UI::Core::CoreWindow^ window);
	void Render();
	void WaitForGPU();
private:
	D3D12_RESOURCE_BARRIER RTVBarrier(D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to);
	D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView();
	void CreateSizeDependentResources();
private:
	Microsoft::WRL::ComPtr<IDXGIFactory4>				mDXGIFactory;
	Microsoft::WRL::ComPtr<IDXGIAdapter4>				mDXGIAdapter;
	Microsoft::WRL::ComPtr<ID3D12Device2>				mDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>			mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		mRTVHeap;
	UINT												mRTVDescriptorSize;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		mCommandAllocators[BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D12RootSignature>			mRootSignature;
	Microsoft::WRL::ComPtr<ID3DBlob>					mVertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob>					mPixelShader;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			mPipelineState;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	mCommandList;
	Microsoft::WRL::ComPtr<ID3D12Resource>				mVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW							mVertexBufferView;

	// ===================================
	// CPU<->GPU synchronization resources
	// ===================================

	Microsoft::WRL::ComPtr<ID3D12Fence>	mFence;
	HANDLE								mFenceEvent;
	uint64_t							mFenceValue;

	// ==========================
	// window dependent resources
	// ==========================

	Platform::Agile<Windows::UI::Core::CoreWindow>		mWindow;
	Microsoft::WRL::ComPtr<IDXGISwapChain4>				mSwapchain;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>	mRenderTargets;
	D3D12_VIEWPORT										mViewport;
	D3D12_RECT											mScissors;

	unsigned int mBufferIndex;
};
