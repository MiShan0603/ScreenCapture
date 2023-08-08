#pragma once


struct D3D11DeviceLock
{
public:
    D3D11DeviceLock(std::nullopt_t) {}
    D3D11DeviceLock(ID3D11Multithread* pMultithread)
    {
        m_multithread.copy_from(pMultithread);
        m_multithread->Enter();
    }
    ~D3D11DeviceLock()
    {
        m_multithread->Leave();
        m_multithread = nullptr;
    }
private:
    winrt::com_ptr<ID3D11Multithread> m_multithread;
};
 
inline auto
CreateD2DDevice(
    winrt::com_ptr<ID2D1Factory1> const& factory,
    winrt::com_ptr<ID3D11Device> const& device)
{
    winrt::com_ptr<ID2D1Device> result;
    winrt::check_hresult(factory->CreateDevice(device.as<IDXGIDevice>().get(), result.put()));
    return result;
}

inline auto
CreateD3DDevice(
    D3D_DRIVER_TYPE const type,
    winrt::com_ptr<ID3D11Device>& device)
{
    WINRT_ASSERT(!device);

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

//#ifdef _DEBUG
//	flags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif

    return D3D11CreateDevice(
        nullptr,
        type,
        nullptr,
        flags,
        nullptr, 0,
        D3D11_SDK_VERSION,
        device.put(),
        nullptr,
        nullptr);
}

inline auto
CreateD3DDevice()
{
    winrt::com_ptr<ID3D11Device> device;
    HRESULT hr = CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE, device);

    if (DXGI_ERROR_UNSUPPORTED == hr)
    {
        hr = CreateD3DDevice(D3D_DRIVER_TYPE_WARP, device);
    }

    winrt::check_hresult(hr);
    return device;
}
 

inline auto
CreateDXGISwapChain(
    winrt::com_ptr<ID3D11Device> const& device,
    const DXGI_SWAP_CHAIN_DESC1* desc)
{
    auto dxgiDevice = device.as<IDXGIDevice2>();
    winrt::com_ptr<IDXGIAdapter> adapter;
    winrt::check_hresult(dxgiDevice->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
    winrt::com_ptr<IDXGIFactory2> factory;
    winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

    winrt::com_ptr<IDXGISwapChain1> swapchain;
    winrt::check_hresult(factory->CreateSwapChainForComposition(
        device.get(),
        desc,
        nullptr,
        swapchain.put()));
 

    return swapchain;
}

inline auto
CreateSharedSurface(
	winrt::com_ptr<ID3D11Device> const& device, 
	uint32_t width,
	uint32_t height, 
	DXGI_FORMAT format, 
	ID3D11Texture2D** surface, 
	HANDLE &sharedHandle) 
{
	D3D11_TEXTURE2D_DESC texDesc = { 0 };

	texDesc.ArraySize = 1;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.Height = height;
	texDesc.Width = width;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
	 
	auto hr = device->CreateTexture2D(&texDesc, NULL, surface);
	if (FAILED(hr)) { 
		return hr;
	}

	sharedHandle = 0;

	// get share handle
	winrt::com_ptr<IDXGIResource> dxgi_resource = nullptr;
	if (SUCCEEDED((*surface)->QueryInterface(__uuidof(IDXGIResource), dxgi_resource.put_void())))
	{
		dxgi_resource->GetSharedHandle(&sharedHandle); 
	}

    return hr;
}
