#include "Render.h"
#include "nanovg/nanovg.h"
#include "nanovg/nanovg_d3d11.h"

Render::Render()
{

}

Render::~Render()
{
	m_renderThrdRunning = false;
	m_renderThrd.join();
}

bool Render::Init(HWND hWnd)
{
	HRESULT hr = S_OK;

	m_hWnd = hWnd;

	RECT rtWindow;
	::GetWindowRect(hWnd, &rtWindow);

	m_pD3D11Device = NULL;
	m_pD3D11DeviceContext = NULL;
	m_pSwapChain = NULL;
	m_pRenderTargetView = NULL;

	if (m_NVGcontext != nullptr)
	{
		nvgDeleteD3D11(m_NVGcontext);
		m_NVGcontext = NULL;
	}

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
	};

	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL  featureLevel = D3D_FEATURE_LEVEL_11_0;


	hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featureLevels, numFeatureLevels,
		D3D11_SDK_VERSION, &m_pD3D11Device, &featureLevel, &m_pD3D11DeviceContext);
	if (FAILED(hr))
	{
		return false;
	}

	///
	CComPtr<IDXGIDevice>  pDXGIDevice = NULL;
	CComPtr<IDXGIAdapter> pAdapter = NULL;
	CComPtr<IDXGIFactory> pDXGIFactory = NULL;
	UINT deviceFlags = 0;

	hr = m_pD3D11Device->QueryInterface(IID_IDXGIDevice, (void**)&pDXGIDevice);
	hr = pDXGIDevice->GetAdapter(&pAdapter);
	hr = pAdapter->GetParent(IID_IDXGIFactory, (void**)&pDXGIFactory);

	DXGI_SWAP_CHAIN_DESC swapDesc;
	ZeroMemory(&swapDesc, sizeof(swapDesc));

	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;

	swapDesc.BufferDesc.Width = rtWindow.right - rtWindow.left;
	swapDesc.BufferDesc.Height = rtWindow.bottom - rtWindow.top;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.BufferCount = 1;
	swapDesc.OutputWindow = hWnd;
	swapDesc.Windowed = TRUE;
	hr = pDXGIFactory->CreateSwapChain((IUnknown*)m_pD3D11Device.p, &swapDesc, &m_pSwapChain);

	// Create the render target view and set it on the device
	CComPtr<ID3D11Resource> pBackBufferResource = NULL;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferResource);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_pD3D11Device->CreateRenderTargetView(pBackBufferResource, NULL, &m_pRenderTargetView);
	if (FAILED(hr))
	{
		return false;
	}

	m_NVGcontext = nvgCreateD3D11(m_pD3D11Device, NVG_ANTIALIAS | NVG_STENCIL_STROKES);

	m_bIsInitD3D = true;

	m_renderThrdRunning = true;
	m_renderThrd = std::thread(&Render::RenderFrame, this);

	return true;
}

void Render::RenderFrame()
{
	while (m_renderThrdRunning)
	{
		Sleep(33);

		DoRenderFrame();
	} // while (m_renderThrdRunning)
}

bool Render::DoRenderFrame()
{
	if (m_bIsInitD3D == false)
	{
		return false;
	}

	std::lock_guard<std::mutex> lck(m_ddasMutex);
	
	// if only one monitor

	// else ...

	if (m_ddas.size() > 0)
	{
		if (m_ddas.size() == 1)
		{
			std::shared_ptr<DDAInfo> dda = m_ddas[0];
			dda->ddaImpl->GetCapturedFrame(&(dda->sharedTexture.p));

			m_pD3D11DeviceContext->CopySubresourceRegion(m_pSharedTexture, 0,
				0, 0,
				0, dda->sharedTexture, 0, NULL);
		}
		else
		{
			for (auto dda : m_ddas)
			{
				dda->ddaImpl->GetCapturedFrame(&(dda->sharedTexture.p));

				m_pD3D11DeviceContext->CopySubresourceRegion(m_pSharedTexture, 0,
					dda->ddaImpl->GetDxgiOutputDesc().DesktopCoordinates.left, dda->ddaImpl->GetDxgiOutputDesc().DesktopCoordinates.top,
					0, dda->sharedTexture, 0, NULL);
			}
		}
		

		if (m_hRenderFrameHandle != m_hSharedHandle)
		{
			if (m_hRenderFrameHandle != NULL)
			{
				nvgDeleteImage(m_NVGcontext, m_imageID);
			}

			m_hRenderFrameHandle = m_hSharedHandle;
		}
	}
	
	
	RECT rtWindow;
	::GetWindowRect(m_hWnd, &rtWindow);

	int nViewW = rtWindow.right - rtWindow.left;
	int nViewH = rtWindow.bottom - rtWindow.top;

	m_pD3D11DeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView.p, NULL);

	D3D11_VIEWPORT viewport;
	viewport.Width = (float)nViewW;
	viewport.Height = (float)nViewH;
	viewport.MaxDepth = 1.0f;
	viewport.MinDepth = 0.0f;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	m_pD3D11DeviceContext->RSSetViewports(1, &viewport);

	nvgBeginFrame(m_NVGcontext, nViewW, nViewH, 1.0);

	nvgBeginPath(m_NVGcontext);
	nvgFillColor(m_NVGcontext, nvgRGBA(0, 0, 0, 255));
	nvgFill(m_NVGcontext);

	if (m_hRenderFrameHandle)
	{
		do
		{
			m_imageID = nvd3dCreateImageFromHandle(m_NVGcontext, m_hRenderFrameHandle, 1);
			if (!m_imageID)
				break;

			int showW = rtWindow.right - rtWindow.left;
			int showH = rtWindow.bottom - rtWindow.top;

			int imgw, imgh;
			nvgImageSize(m_NVGcontext, m_imageID, &imgw, &imgh);

			NVGpaint imgPaint = nvgImagePattern(m_NVGcontext, 0, 0, showW, showH, 0.0f, m_imageID, 1.0f);
			nvgBeginPath(m_NVGcontext);
			nvgRoundedRect(m_NVGcontext, 0, 0, showW, showH, 0);
			nvgFillPaint(m_NVGcontext, imgPaint);
			nvgFill(m_NVGcontext);

		} while (false);

	}

	nvgEndFrame(m_NVGcontext);

	// Don't wait for VBlank
	m_pSwapChain->Present(0, 0);

	return true;
}

void Render::Start(std::vector<HMONITOR> monitors, RECT rect)
{
	std::lock_guard<std::mutex> lck(m_ddasMutex);
	m_ddas.clear();
	
	HRESULT hr = S_OK;

	for (int index = 0; index < monitors.size(); index++)
	{
		// std::shared_ptr<DDAImpl> dda = std::make_shared<DDAImpl>(m_pD3D11Device, m_pD3D11DeviceContext);
		std::shared_ptr<DDAInfo> dda = std::make_shared<DDAInfo>();
		dda->ddaImpl = std::make_shared<DDAImpl>(m_pD3D11Device, m_pD3D11DeviceContext);
		if (dda->ddaImpl->Init(monitors[index]) == S_OK)
		{
			m_ddas.push_back(dda);
		}
	}

	if (m_sharedTextureRect.right - m_sharedTextureRect.left != rect.right - rect.left ||
		m_sharedTextureRect.bottom - m_sharedTextureRect.top != rect.bottom - rect.top)
	{
		m_pSharedTexture = nullptr;
		m_hSharedHandle = NULL;
	}

	m_sharedTextureRect = rect;

	{
		if (!m_pSharedTexture)
		{
			do
			{
				int width = m_sharedTextureRect.right - m_sharedTextureRect.left;
				int height = m_sharedTextureRect.bottom - m_sharedTextureRect.top;

				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));

				desc.Width = width;
				desc.Height = height;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
				desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				if (FAILED(hr = m_pD3D11Device->CreateTexture2D(&desc, NULL, &m_pSharedTexture)))
				{
					m_pSharedTexture = nullptr;
					break;
				}

				m_hSharedHandle = NULL;
				CComPtr<IDXGIResource> pDXGIResource;
				hr = m_pSharedTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
				if (FAILED(hr))
				{
					m_pSharedTexture = nullptr;
					break;
				}

				hr = pDXGIResource->GetSharedHandle(&m_hSharedHandle);
				if (FAILED(hr))
				{
					m_pSharedTexture = nullptr;
					break;
				}

			} while (false);

		}
	}
	

}

void Render::Stop()
{
	std::lock_guard<std::mutex> lck(m_ddasMutex);
	m_ddas.clear();
	m_sharedTextureRect = { 0 };
	m_pSharedTexture = nullptr;
}