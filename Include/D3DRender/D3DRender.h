#pragma once

#include <atlbase.h>
#include <d3d11.h>
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "windowscodecs.lib")

struct NVGcontext;
class D3DRender
{
public:
	~D3DRender();

	static D3DRender* Instance();
	static void Release();

public:
	bool Init(HWND hWnd);
	bool DrawFrame(HANDLE sharedHanle);

	CComPtr<ID3D11Device> GetD3D11Device();

private:
	D3DRender();
	static D3DRender* m_instance;

	bool m_bIsInitD3D = false;
	HWND m_hWnd = NULL;

	NVGcontext* m_NVGcontext = nullptr;

	HANDLE m_sharedHandle = NULL;
	int m_imageID = 0;

	CComPtr<ID3D11Device>					m_pD3D11Device;
	CComPtr<ID3D11DeviceContext>			m_pD3D11DeviceContext;
	CComPtr<IDXGISwapChain>					m_pSwapChain;
	CComPtr<ID3D11RenderTargetView>			m_pRenderTargetView;
};

