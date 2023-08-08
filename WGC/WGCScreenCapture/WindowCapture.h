#pragma once

 
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include <atomic>
#include <memory>

#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d2d1_3.h>
#include <wincodec.h>

class WindowCapture
{
public:
	WindowCapture(ID3D11Device *pDevice);

	/// <summary>
	/// Capture window
	/// </summary>
	/// <param name="hWnd"></param>
	void CaptureWindow(HWND hWnd);

	/// <summary>
	/// Capture monitor
	/// </summary>
	/// <param name="monitor"></param>
	void CaptureMonitor(HMONITOR monitor);
	 
    ~WindowCapture() { Close(); }

    void StartCapture(); 
    void Close();

    bool IsCursorEnabled() { CheckClosed(); return m_session.IsCursorCaptureEnabled(); }
    void IsCursorEnabled(bool value) { CheckClosed(); m_session.IsCursorCaptureEnabled(value); }

private:
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    inline void CheckClosed()
    {
        if (m_closed.load() == true)
        {
            throw winrt::hresult_error(RO_E_CLOSED);
        }
    }

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 m_lastSize;
	 
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::com_ptr<ID3D11Device>		m_d3dDevice{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
	
	winrt::com_ptr<ID3D11Texture2D>		m_sharedSurface{ nullptr };
	HANDLE							    m_sharedHandle{ nullptr };
	HWND				m_hwnd = nullptr;

	DWORD               m_lastFrameArrived;

    std::atomic<bool> m_closed = false;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker m_frameArrived;
};