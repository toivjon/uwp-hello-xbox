#include "renderer.h"

#include <array>
#include <chrono>
#include <d3dcompiler.h>

// undefine min macro and use the std::min from the <algorithm>
#if defined(min)
#undef min
#endif

// undefine max macro and use the std::max from the <algorithm>
#if defined(max)
#undef max
#endif

using namespace xbox;

using namespace Microsoft::WRL;
using namespace std::chrono;

// vertex structure
struct Vertex
{
	std::array<float, 3> position;
	std::array<float, 4> color;
};

// a helper macro to allow writing shader code as a multiline string.
#define SHADER(CODE) #CODE

// a helper utility to throw exception on failure HRESULTs.
inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
		throw Platform::Exception::CreateException(hr);
	}
}

// a helper utility to signal the target fence.
inline uint64_t signalFence(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& value)
{
	// increment the fence value to indicate a new signal.
	uint64_t signalValue = ++value;

	// try to signal the fence with the incremented signal value.
	ThrowIfFailed(commandQueue->Signal(fence.Get(), signalValue));

	return signalValue;
}

// a helper utility to wait for fence to receive a signal from GPU.
inline void waitFence(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE event, milliseconds duration)
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		// specify which event to trigger after fence has been finished.
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, event));

		// wait for a signal or until the given duration has elapsed.
		WaitForSingleObject(event, static_cast<DWORD>(duration.count()));
	}
}

// a helper utility to flush and wait commands to be delivered to GPU.
inline void flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& value, HANDLE event)
{
	auto signalValue = signalFence(commandQueue, fence, value);
	waitFence(fence, signalValue, event, milliseconds::max());
}

Renderer::Renderer() : mScissors{0, 0, LONG_MAX, LONG_MAX}, mBufferIndex(0)
{
	// create a factory or DXGI item instances.
	ThrowIfFailed(CreateDXGIFactory2(0u, IID_PPV_ARGS(&mDXGIFactory)));

	// find the first suitable graphics adapter for D3D12.
	ComPtr<IDXGIAdapter1> adapter;
	for (auto i = 0u; !mDXGIAdapter && mDXGIFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		// get the adapter descriptor info item.
		DXGI_ADAPTER_DESC1 descriptor;
		adapter->GetDesc1(&descriptor);

		// skip software emulation based adapters.
		if ((descriptor.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
			continue;

		// skip adapters that cannot be created.
		if (FAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
			continue;

		// we found a good candidate as the selected adapter.
		ThrowIfFailed(adapter.As(&mDXGIAdapter));
	}

	// create a new D3D12 device with the target graphics adapter.
	ThrowIfFailed(D3D12CreateDevice(mDXGIAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));

	// create a command queue to send commands to pipeline.
	D3D12_COMMAND_QUEUE_DESC queueDescriptor = {};
	queueDescriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDescriptor.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	queueDescriptor.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDescriptor.NodeMask = 0;
	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDescriptor, IID_PPV_ARGS(&mCommandQueue)));

	// create a heap descriptor for the render target view (RTV).
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescriptor = {};
	rtvHeapDescriptor.NumDescriptors = BUFFER_COUNT;
	rtvHeapDescriptor.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDescriptor, IID_PPV_ARGS(&mRTVHeap)));

	// create a command allocator for each buffer.
	for (auto i = 0u; i < BUFFER_COUNT; i++) {
		ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocators[i])));
	}

	// create a new root signature.
	ComPtr<ID3DBlob> signature, error;
	D3D12_ROOT_SIGNATURE_DESC signatureDesc = {};
	signatureDesc.NumParameters = 0;
	signatureDesc.pParameters = nullptr;
	signatureDesc.NumStaticSamplers = 0;
	signatureDesc.pStaticSamplers = nullptr;
	signatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ThrowIfFailed(D3D12SerializeRootSignature(&signatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
	ThrowIfFailed(mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));

	// compile vertex and pixel shader.
	auto shaderSrc = SHADER(
		struct PSInput
		{
			float4 position : SV_POSITION;
			float4 color : COLOR;
		};

		PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
		{
			PSInput result;
			result.position = position;
			result.color = color;
			return result;
		}

		float4 PSMain(PSInput input) : SV_TARGET
		{
		  return input.color;
		}
	);
	ThrowIfFailed(D3DCompile(shaderSrc, strlen(shaderSrc), "", nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &mVertexShader, &error));
	ThrowIfFailed(D3DCompile(shaderSrc, strlen(shaderSrc), "", nullptr, nullptr, "VSMain", "ps_5_0", 0, 0, &mPixelShader, &error));

	// define the layout for the input vertex data.
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputDescriptor = {
	  {
		"POSITION",
		0,
		DXGI_FORMAT_R32G32B32_FLOAT,
		0,
		0,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
	  },
	  {
		"COLOR",
		0,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		0,
		12,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA
	  }
	};

	// create a descriptor for the rasterizer state (derived from CD3DX12_RASTERIZER_DESC(CD3DX12_DEFAULT))
	D3D12_RASTERIZER_DESC rasterizerDescriptor = {};
	rasterizerDescriptor.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDescriptor.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDescriptor.FrontCounterClockwise = false;
	rasterizerDescriptor.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizerDescriptor.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizerDescriptor.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizerDescriptor.DepthClipEnable = true;
	rasterizerDescriptor.MultisampleEnable = false;
	rasterizerDescriptor.AntialiasedLineEnable = false;
	rasterizerDescriptor.ForcedSampleCount = 0;
	rasterizerDescriptor.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// create a descriptor for the blend state (derived from CD3DX12_BLEND_DESC(CD3DX12_DEFAULT))
	D3D12_BLEND_DESC blendDescriptor = {};
	blendDescriptor.AlphaToCoverageEnable = false;
	blendDescriptor.IndependentBlendEnable = false;
	blendDescriptor.RenderTarget[0].BlendEnable = false;
	blendDescriptor.RenderTarget[0].LogicOpEnable = false;
	blendDescriptor.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDescriptor.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDescriptor.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDescriptor.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDescriptor.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDescriptor.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDescriptor.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDescriptor.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// create a desciptor for the pipeline state object.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStatedescriptor = {};
	pipelineStatedescriptor.InputLayout = { &inputDescriptor[0], (UINT)inputDescriptor.size() };
	pipelineStatedescriptor.pRootSignature = mRootSignature.Get();
	pipelineStatedescriptor.VS = { reinterpret_cast<UINT8*>(mVertexShader->GetBufferPointer()), mVertexShader->GetBufferSize() };
	pipelineStatedescriptor.PS = { reinterpret_cast<UINT8*>(mPixelShader->GetBufferPointer()), mPixelShader->GetBufferSize() };
	pipelineStatedescriptor.RasterizerState = rasterizerDescriptor;
	pipelineStatedescriptor.BlendState = blendDescriptor;
	pipelineStatedescriptor.DepthStencilState.DepthEnable = false;
	pipelineStatedescriptor.DepthStencilState.StencilEnable = false;
	pipelineStatedescriptor.SampleMask = UINT_MAX;
	pipelineStatedescriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStatedescriptor.NumRenderTargets = 1;
	pipelineStatedescriptor.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStatedescriptor.SampleDesc.Count = 1;

	// create a new graphics pipeline state with the above descriptors.
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&pipelineStatedescriptor, IID_PPV_ARGS(&mPipelineState)));

	// create a command list and ensure that it's closed to stop immediate command recording.
	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocators[0].Get(), mPipelineState.Get(), IID_PPV_ARGS(&mCommandList)));
	ThrowIfFailed(mCommandList->Close());

	// construct the required vertices for a simple triangle.
	std::vector<Vertex> vertices = {
	  {{  0.0f,  0.5f, 0.0f }, { 1.f, 0.f, 0.f, 1.f }},
	  {{  0.5f, -0.5f, 0.0f }, { 0.f, 1.f, 0.f, 1.f }},
	  {{ -0.5f, -0.5f, 0.0f }, { 0.f, 0.f, 1.f, 1.f }}
	};

	// construct properties for the upload heap.
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// construct a descriptor for a vertex buffer (derived from CD3DX12_RESOURCE_DESC).
	D3D12_RESOURCE_DESC resourceDescriptor = {};
	resourceDescriptor.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDescriptor.Alignment = 0;
	resourceDescriptor.Width = sizeof(Vertex) * vertices.size();
	resourceDescriptor.Height = 1;
	resourceDescriptor.DepthOrArraySize = 1;
	resourceDescriptor.MipLevels = 1;
	resourceDescriptor.Format = DXGI_FORMAT_UNKNOWN;
	resourceDescriptor.SampleDesc.Count = 1;
	resourceDescriptor.SampleDesc.Quality = 0;
	resourceDescriptor.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDescriptor.Flags = D3D12_RESOURCE_FLAG_NONE;

	// create the vertex buffer resource and assign vertices into the buffer.unsigned char* data(0);
	unsigned char* data(0);
	D3D12_RANGE range = {};
	ThrowIfFailed(mDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDescriptor, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mVertexBuffer)));
	ThrowIfFailed(mVertexBuffer->Map(0, &range, reinterpret_cast<void**>(&data)));
	memcpy(data, &vertices[0], sizeof(Vertex)* vertices.size());
	mVertexBuffer->Unmap(0, nullptr);

	// wait until the provided vertices have been uploaded to GPU.
	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	uint64_t fenceValue = 1;
	mFenceEvent = CreateEvent(nullptr, false, false, nullptr);
	flush(mCommandQueue, mFence, fenceValue, mFenceEvent);
	mFenceValue = 0;

	// create a vertex buffer view from the vertex buffer definitions.
	mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
	mVertexBufferView.StrideInBytes = sizeof(Vertex);
	mVertexBufferView.SizeInBytes = sizeof(Vertex) * 3;
}

void Renderer::SetWindow(Windows::UI::Core::CoreWindow^ window)
{
	// release old swap chain if exists.
	if (mSwapchain) {
		mSwapchain->Release();
	}

	// release old render targets if any exists.
	if (!mRenderTargets.empty()) {
		mRenderTargets.clear();
	}

	// create a swap chain for the target core window instance.
	ComPtr<IDXGISwapChain1> swapChain;
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = (UINT)window->Bounds.Width;
	swapChainDesc.Height = (UINT)window->Bounds.Height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = BUFFER_COUNT;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;
	ThrowIfFailed(mDXGIFactory->CreateSwapChainForCoreWindow(mCommandQueue.Get(), reinterpret_cast<IUnknown*>(window), &swapChainDesc, nullptr, &swapChain));
	ThrowIfFailed(swapChain.As(&mSwapchain));

	// resize viewport to match with the window size.
	mViewport = {};
	mViewport.TopLeftX = 0;
	mViewport.TopLeftY = 0;
	mViewport.MaxDepth = D3D12_MAX_DEPTH;
	mViewport.MinDepth = D3D12_MIN_DEPTH;
	mViewport.Width = window->Bounds.Width;
	mViewport.Height = window->Bounds.Height;

	// get the size of the render target descriptor and the position where heap starts.
	auto rtvSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto rtvHeap = mRTVHeap->GetCPUDescriptorHandleForHeapStart();

	// construct a new render target view for each rendering buffer.
	for (auto i = 0; i < BUFFER_COUNT; i++) {
		ComPtr<ID3D12Resource> buffer;
		ThrowIfFailed(mSwapchain->GetBuffer(i, IID_PPV_ARGS(&buffer)));
		mDevice->CreateRenderTargetView(buffer.Get(), nullptr, rtvHeap);
		mRenderTargets.push_back(buffer);
		rtvHeap.ptr += rtvSize;
	}
}

void Renderer::Render()
{
	// reset the memory associated with the command allocator.
	ThrowIfFailed(mCommandAllocators[mBufferIndex]->Reset());

	// reset the state of the command list.
	ThrowIfFailed(mCommandList->Reset(mCommandAllocators[mBufferIndex].Get(), mPipelineState.Get()));

	// assign the root signature.
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// configure rasterizer state viewport and scissors.
	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissors);

	// switch resources to be used as rendering targets.
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = mRenderTargets[mBufferIndex].Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	mCommandList->ResourceBarrier(1, &barrier);

	// assign the back buffer as the rendering target.
	auto rtvSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto rtvHeap = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
	if (mBufferIndex == 1) {
		rtvHeap.ptr += rtvSize;
	}
	mCommandList->OMSetRenderTargets(1, &rtvHeap, false, nullptr);

	// perform the actual rendering on the rendering target.
	float clearColor[] = { 0.5f, 0.5f, 0.5f, 0.5f };
	mCommandList->ClearRenderTargetView(rtvHeap, clearColor, 0, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	mCommandList->DrawInstanced(3, 1, 0, 0);

	// switch resources to be used as presentation items.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	mCommandList->ResourceBarrier(1, &barrier);

	// close the command list to finalize rendering.
	ThrowIfFailed(mCommandList->Close());

	// submit the command list into the command queue for the execution.
	std::vector<ID3D12CommandList*> const commandList = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(1, &commandList[0]);

	// present the current back buffer onto screen.
	ThrowIfFailed(mSwapchain->Present(1, 0));

	// wait until the GPU has completed rendering.
	signalFence(mCommandQueue, mFence, mFenceValue);
	waitFence(mFence, mFenceValue, mFenceEvent, milliseconds::max());

	// proceed to next buffer in a round-robin manner.
	mBufferIndex = (mBufferIndex + 1) % BUFFER_COUNT;
}