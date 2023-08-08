#include <Unknwn.h>

#undef INTERFACE
#define INTERFACE IGraphicsCaptureItemInterop
DECLARE_INTERFACE_IID_(IGraphicsCaptureItemInterop, IUnknown, "3628E81B-3CAC-4C60-B7F4-23CE0E0C3356")
{
	IFACEMETHOD(CreateForWindow)(
		HWND window,
		REFIID riid,
		_COM_Outptr_ void** result
		) PURE;

	IFACEMETHOD(CreateForMonitor)(
		HMONITOR monitor,
		REFIID riid,
		_COM_Outptr_ void** result
		) PURE;

};

#include <inspectable.h>
#include "WindowCapture.h"
#include <winrt/Windows.Foundation.Collections.h>
#include <windows.graphics.capture.h>
#include "d3dHelpers.h"
#include "direct3d11.interop.h"
#include "D3DRender/D3DRender.h"
 
using namespace winrt;
using namespace Windows;
using namespace Windows::Foundation;
using namespace Windows::System;
using namespace Windows::Graphics;
using namespace Windows::Graphics::Capture;
using namespace Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
using namespace Windows::Foundation::Numerics; 

 
WindowCapture::WindowCapture(ID3D11Device* pDevice)
{
	// m_d3dDevice = CreateD3DDevice();
	m_d3dDevice.copy_from(pDevice);
	m_d3dDevice->GetImmediateContext(m_d3dContext.put());

	auto dxgiDevice = m_d3dDevice.as<IDXGIDevice>();
	m_device = CreateDirect3DDevice(dxgiDevice.get());

	m_hwnd = NULL;
	m_lastFrameArrived = 0;
}


void WindowCapture::CaptureWindow(HWND hWnd)
{
	m_hwnd = hWnd;
	auto activation_factory = winrt::get_activation_factory<GraphicsCaptureItem>();
	auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
	
	interop_factory->CreateForWindow(hWnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(m_item)));

	// 
	auto size = m_item.Size();

	//create shared surface
	CreateSharedSurface(m_d3dDevice, size.Width, size.Height, 
		static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized),
		m_sharedSurface.put(), m_sharedHandle);

	// Create framepool, define pixel format (DXGI_FORMAT_B8G8R8A8_UNORM), and frame size. 
	m_framePool = Direct3D11CaptureFramePool::Create(
		m_device,
		DirectXPixelFormat::B8G8R8A8UIntNormalized,
		2,
		size);
	m_session = m_framePool.CreateCaptureSession(m_item);
	m_lastSize = size;
	m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &WindowCapture::OnFrameArrived });
}

void WindowCapture::CaptureMonitor(HMONITOR monitor)
{
	// CreateCaptureItem  
	auto activation_factory = winrt::get_activation_factory<GraphicsCaptureItem>();
	auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
 
	interop_factory->CreateForMonitor(monitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(m_item)));

	// 
	auto size = m_item.Size();

	//create shared surface
	CreateSharedSurface(m_d3dDevice, size.Width, size.Height, 
		static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 
		m_sharedSurface.put(), m_sharedHandle);

	// Create framepool, define pixel format (DXGI_FORMAT_B8G8R8A8_UNORM), and frame size. 
	m_framePool = Direct3D11CaptureFramePool::Create(
		m_device,
		DirectXPixelFormat::B8G8R8A8UIntNormalized,
		2,
		size);
	m_session = m_framePool.CreateCaptureSession(m_item);
	m_lastSize = size;
	m_frameArrived = m_framePool.FrameArrived(auto_revoke, { this, &WindowCapture::OnFrameArrived });
	 
}
 

// Start sending capture frames
void WindowCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture(); 
}
  
// Process captured frames
void WindowCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
		m_frameArrived.revoke();
		m_framePool.Close();
        m_session.Close();

		m_sharedSurface = nullptr;
        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;

		m_d3dDevice = nullptr;
		m_d3dContext = nullptr;
    }
}


int g_test = 0;

void WindowCapture::OnFrameArrived(
    Direct3D11CaptureFramePool const& sender,
    winrt::Windows::Foundation::IInspectable const&)
{
    auto newSize = false;

 
    auto frame = sender.TryGetNextFrame();
	auto frameContentSize = frame.ContentSize();

    if (frameContentSize.Width != m_lastSize.Width ||
		frameContentSize.Height != m_lastSize.Height)
    {
        // The thing we have been capturing has changed size.
        // We need to resize our swap chain first, then blit the pixels.
        // After we do that, retire the frame and then recreate our frame pool.
        newSize = true;
        m_lastSize = frameContentSize;

		m_sharedSurface = nullptr;
		m_sharedHandle = nullptr;

		CreateSharedSurface(m_d3dDevice, m_lastSize.Width, m_lastSize.Height, 
			static_cast<DXGI_FORMAT>(DirectXPixelFormat::B8G8R8A8UIntNormalized), 
			m_sharedSurface.put(), m_sharedHandle);

		m_lastFrameArrived = 0;
    }

	 
	DWORD dwTime = ::timeGetTime();
	if(dwTime - m_lastFrameArrived > 30)
    {  
		auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

        m_d3dContext->CopyResource(m_sharedSurface.get(), frameSurface.get());
		m_d3dContext->Flush();

		D3DRender::Instance()->DrawFrame(m_sharedHandle);

		m_lastFrameArrived = dwTime;
    }
   
    if (newSize)
    {
        m_framePool.Recreate(
            m_device,
            DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            m_lastSize);
    }
}

