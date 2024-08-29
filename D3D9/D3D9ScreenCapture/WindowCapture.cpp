#include "WindowCapture.h"
#include "D3DRender/D3DRender.h"

#pragma comment(lib, "Msimg32.lib")

WindowCapture::WindowCapture(ID3D11Device* pD3D11Device)
{
	m_d3d11Device = pD3D11Device;
	m_d3d11Device->GetImmediateContext(&m_d3d11Context);

	m_bCaptureThrdRunning = FALSE;
	m_hCaptureThrd = NULL;
}

WindowCapture::~WindowCapture()
{
	Release();
}

BOOL WindowCapture::Init(RECT monitorRect, float fps/* = 30.0*/)
{
	Release();

	m_monitorRect = monitorRect;
	m_fps = fps;

	m_bCaptureThrdRunning = TRUE;
	m_hCaptureThrd = CreateThread(NULL, 0, CaptureThrd, this, 0, NULL);

	return TRUE;
}

void WindowCapture::Release()
{
	m_pD3D9Ex = NULL;
	m_bCaptureThrdRunning = FALSE;

	if (m_hCaptureThrd)
	{
		while (WaitForSingleObject(m_hCaptureThrd, 0) != WAIT_OBJECT_0)
		{
			MSG msg;
			if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		CloseHandle(m_hCaptureThrd);
		m_hCaptureThrd = NULL;
	}
}

void WindowCapture::ShowCusor(BOOL show)
{
	m_showCursor = show;
}

DWORD WINAPI WindowCapture::CaptureThrd(LPVOID lpParam)
{
	WindowCapture* pCap = (WindowCapture*)lpParam;

	pCap->DoCapture();

	return 0;
}

void WindowCapture::DoCapture()
{
	// just for test...
	// m_fps = 10;
	int sleepTime = 1000.0 / m_fps;

	int width = m_monitorRect.right - m_monitorRect.left;
	int height = m_monitorRect.bottom - m_monitorRect.top;

	HRESULT hr = S_OK;
	while (m_bCaptureThrdRunning)
	{
		Sleep(sleepTime);

		if (!m_pD3D9Ex)
		{
			Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D9Ex);
		}
		
		if (!m_d3d11Device || !m_pD3D9Ex)
			continue;

		if (m_sharedTexture)
		{
			D3D11_TEXTURE2D_DESC desc;
			m_sharedTexture->GetDesc(&desc);

			if (desc.Width != m_monitorRect.right - m_monitorRect.left ||
				desc.Height != m_monitorRect.bottom - m_monitorRect.top)
			{
				m_d3d9Devs.clear();
				m_sharedTexture = NULL;
			}
		}

		if (!m_sharedTexture)
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));

			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED | D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			if (FAILED(hr = m_d3d11Device->CreateTexture2D(&desc, NULL, &m_sharedTexture)))
			{
				m_sharedTexture = NULL;
				continue;
			}

			m_sharedHandle = NULL;
			CComPtr<IDXGIResource> pDXGIResource;
			hr |= m_sharedTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
			if (FAILED(hr))
			{
				m_sharedTexture = NULL;
				continue;
			}

			hr |= pDXGIResource->GetSharedHandle(&m_sharedHandle);
			if (FAILED(hr))
			{
				m_sharedTexture = NULL;
				continue;
			}
		}

		// We only consider the situation where the Rect area and screen area are consistent, and other situations are omitted.
		if (m_d3d9Devs.size() == 0)
		{
			D3DPRESENT_PARAMETERS parameters = { 0 };
			parameters.Windowed = TRUE;
			parameters.BackBufferCount = 1;
			parameters.BackBufferHeight = 1;
			parameters.BackBufferWidth = 1;
			parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
			parameters.hDeviceWindow = NULL;

			UINT adapterCount = m_pD3D9Ex->GetAdapterCount();
			for (UINT adapterIndex = 0; adapterIndex < adapterCount; adapterIndex++)
			{
				HMONITOR hMonitor = m_pD3D9Ex->GetAdapterMonitor(adapterIndex);
				MONITORINFO monitorInfo;
				monitorInfo.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(hMonitor, &monitorInfo);
				
				RECT overlapRect;
				if (!RectIsOverlap(m_monitorRect, monitorInfo.rcMonitor, overlapRect))
				{
					continue;
				}

				D3DDISPLAYMODE mode;
				hr = m_pD3D9Ex->GetAdapterDisplayMode(adapterIndex, &mode);
				if (FAILED(hr))
				{
					continue;
				}

				std::shared_ptr<D3D9Dev> pD3D9Dev = std::make_shared<D3D9Dev>();
				pD3D9Dev->rect = monitorInfo.rcMonitor;

				if (FAILED(hr = m_pD3D9Ex->CreateDeviceEx(adapterIndex, D3DDEVTYPE_HAL, NULL,
					D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
					&parameters, NULL, &pD3D9Dev->pD3D9Device)))
				{
					pD3D9Dev = NULL;
					continue;
				}

				if (FAILED(hr = pD3D9Dev->pD3D9Device->CreateTexture(mode.Width, mode.Height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pD3D9Dev->pScreenTexture, 0)))
				{
					pD3D9Dev = NULL;
					continue;
				}


				D3D11_TEXTURE2D_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Width = mode.Width;
				desc.Height = mode.Height;
				desc.MipLevels = 1;
				desc.ArraySize = 1;
				desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				desc.SampleDesc.Count = 1;
				desc.Usage = D3D11_USAGE_DEFAULT;
				desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
				desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
				if (FAILED(hr = m_d3d11Device->CreateTexture2D(&desc, NULL, &pD3D9Dev->pSharedTexture)))
				{
					pD3D9Dev = NULL;
					continue;
				}

				pD3D9Dev->hSharedHandle = NULL;
				CComPtr<IDXGIResource> pDXGIResource;
				hr |= pD3D9Dev->pSharedTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
				if (FAILED(hr))
				{
					pD3D9Dev = NULL;
					continue;
				}

				hr |= pDXGIResource->GetSharedHandle(&pD3D9Dev->hSharedHandle);
				if (FAILED(hr))
				{
					pD3D9Dev = NULL;
					continue;
				}

				m_d3d9Devs.push_back(pD3D9Dev);
			}
		}

		// 测试设备是否有效， 无效重新创建
		for (auto dev : m_d3d9Devs)
		{
			hr = dev->pD3D9Device->TestCooperativeLevel();
			if (hr == D3DERR_DEVICENOTRESET)
			{
				m_d3d9Devs.clear();
				m_sharedTexture = NULL;
				m_pD3D9Ex = NULL;
			}

			break;
		}


		bool bGetCursorInfoOK = false;
		CURSORINFO cursorInfo = { 0 };
		cursorInfo.cbSize = sizeof(CURSORINFO);
		if (m_showCursor && GetCursorInfo(&cursorInfo))
		{
			if (cursorInfo.flags == CURSOR_SHOWING)
			{
				bGetCursorInfoOK = true;
			}
		}

		// 创建共享位面（复制用：从system memory -> video memory）
		for (auto dev : m_d3d9Devs)
		{
			CComPtr<IDirect3DTexture9> pD3D9SharedTexture = NULL;
			if (FAILED(hr = dev->pD3D9Device->CreateTexture(dev->rect.right - dev->rect.left, dev->rect.bottom - dev->rect.top, 1, 
				D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pD3D9SharedTexture, &dev->hSharedHandle)))
			{
				continue;
			}

			CComPtr<IDirect3DSurface9> pScreenSurface = NULL;
			hr = dev->pScreenTexture->GetSurfaceLevel(0, &pScreenSurface);
			if (FAILED(hr))
			{
				continue;
			}

			hr = dev->pD3D9Device->GetFrontBufferData(0, pScreenSurface);
			if (FAILED(hr))
			{
				continue;
			}

			hr = dev->pD3D9Device->UpdateTexture(dev->pScreenTexture, pD3D9SharedTexture);
			if (FAILED(hr))
			{
				continue;
			}

		}


		if (m_d3d9Devs.size() > 0)
		{
			for (auto dev : m_d3d9Devs)
			{
				m_d3d11Context->CopySubresourceRegion(m_sharedTexture, 0, dev->rect.left, dev->rect.top, 0, dev->pSharedTexture, 0, NULL);
			}


			if (bGetCursorInfoOK &&
				cursorInfo.ptScreenPos.x >= m_monitorRect.left && cursorInfo.ptScreenPos.x < m_monitorRect.right &&
				cursorInfo.ptScreenPos.y >= m_monitorRect.top && cursorInfo.ptScreenPos.y < m_monitorRect.bottom)
			{
				int cursorWidth = 0;
				int cursorHeight = 0;
				ICONINFO iconInfo;
				if (GetIconInfo(cursorInfo.hCursor, &iconInfo))
				{
					BITMAP bmpCursor;
					if (GetObject(iconInfo.hbmMask, sizeof(BITMAP), &bmpCursor))
					{
						cursorWidth = bmpCursor.bmWidth;
						cursorHeight = bmpCursor.bmHeight; // hbmMask 包含两个位图，AND 和 XOR 掩码
					}

					// 释放位图资源
					DeleteObject(iconInfo.hbmMask);
					DeleteObject(iconInfo.hbmColor);
				}

				

				// 文本光标（I-beam cursor） 的时候，取出来是一个透明的正方形，所以下面做一次叠加
				CComPtr<IDXGISurface1> surface1;
				hr = m_sharedTexture->QueryInterface(__uuidof(IDXGISurface1), (void**)&surface1);
				if (SUCCEEDED(hr))
				{
					auto cursorPosition = cursorInfo.ptScreenPos;
					HDC hdc = NULL;
					surface1->GetDC(FALSE, &hdc);
					if (hdc)
					{
#if 0
						DrawIconEx(hdc, cursorPosition.x - m_monitorRect.left, cursorPosition.y - m_monitorRect.top,
							cursorInfo.hCursor, 0, 0, 0, 0, DI_IMAGE | DI_DEFAULTSIZE);
						// DrawIcon(hdc, cursorPosition.x - m_monitorRect.left, cursorPosition.y - m_monitorRect.top, cursorInfo.hCursor);
#else
						// 创建一个兼容的内存DC
						HDC memDC = CreateCompatibleDC(hdc);
						HBITMAP hBitmap = CreateCompatibleBitmap(hdc, cursorWidth, cursorHeight);
						SelectObject(memDC, hBitmap);

						// 绘制光标到内存DC
						DrawIconEx(memDC, 0, 0, cursorInfo.hCursor, cursorWidth, cursorHeight, 0, 0, DI_NORMAL | DI_DEFAULTSIZE);

						// 使用AlphaBlend将光标叠加到目标DC
						BLENDFUNCTION blendFunc = { 0 };
						blendFunc.BlendOp = AC_SRC_OVER;
						blendFunc.BlendFlags = 0;
						blendFunc.SourceConstantAlpha = 255;
						blendFunc.AlphaFormat = AC_SRC_ALPHA;

						AlphaBlend(hdc, cursorPosition.x - m_monitorRect.left, cursorPosition.y - m_monitorRect.top,
							cursorWidth, cursorHeight, memDC, 0, 0, cursorWidth, cursorHeight, blendFunc);

						// 清理资源
						DeleteObject(hBitmap);
						DeleteDC(memDC);
#endif

						surface1->ReleaseDC(nullptr);

					}
				}
			}
			
			D3DRender::Instance()->DrawFrame(m_sharedHandle);
		}
		

	} // while (m_bCaptureThrdRunning)
}

/// <summary>
/// 计算rect1 2是否重合以及重合的范围
/// </summary>
/// <param name="rect1"></param>
/// <param name="rect2"></param>
/// <param name="overlapRect"></param>
/// <returns></returns>
BOOL WindowCapture::RectIsOverlap(RECT rect1, RECT rect2, RECT& overlapRect)
{
	if (rect1.right <= rect2.left || rect1.left >= rect2.right)
	{
		return FALSE;
	}

	if (rect1.bottom <= rect2.top || rect1.top >= rect2.bottom)
	{
		return FALSE;
	}

	POINT ltPt, rbPt;
	ltPt.x = max(rect1.left, rect2.left);
	ltPt.y = max(rect1.top, rect2.top);
	rbPt.x = min(rect1.right, rect2.right);
	rbPt.y = min(rect1.bottom, rect2.bottom);

	overlapRect.left = ltPt.x;
	overlapRect.top = ltPt.y;
	overlapRect.right = rbPt.x;
	overlapRect.bottom = rbPt.y;

	return TRUE;
}
