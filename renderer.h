#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#define BUFFER_COUNT 2

namespace xbox
{
	class Renderer final
	{
	public:
		Renderer();
	private:
		Microsoft::WRL::ComPtr<IDXGIFactory4>				mDXGIFactory;
		Microsoft::WRL::ComPtr<IDXGIAdapter4>				mDXGIAdapter;
		Microsoft::WRL::ComPtr<ID3D12Device2>				mDevice;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			mCommandQueue;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>		mRTVHeap;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator>		mCommandAllocators[BUFFER_COUNT];
		Microsoft::WRL::ComPtr<ID3D12RootSignature>			mRootSignature;
		Microsoft::WRL::ComPtr<ID3DBlob>					mVertexShader;
		Microsoft::WRL::ComPtr<ID3DBlob>					mPixelShader;
		Microsoft::WRL::ComPtr<ID3D12PipelineState>			mPipelineState;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>	mCommandList;
		Microsoft::WRL::ComPtr<ID3D12Resource>				mVertexBuffer;

		Microsoft::WRL::ComPtr<ID3D12Fence>	mFence;
		HANDLE								mFenceEvent;

		D3D12_VIEWPORT	mViewport;
		D3D12_RECT		mScissors;
	};
}