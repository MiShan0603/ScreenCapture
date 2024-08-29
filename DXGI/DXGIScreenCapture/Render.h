#pragma once

#include "DDAImpl.h"
#include <vector>
#include <memory>
#include <mutex>
#include <atlbase.h>
#include <d3d11.h>
#include <thread>
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "windowscodecs.lib")

struct NVGcontext;
class Render
{
public:
	Render();
	~Render();

	bool Init(HWND hWnd);

	void Start(std::vector<HMONITOR> monitors, RECT rect);
	void Stop();

private:
	void RenderFrame();
	bool DoRenderFrame();

private:
	bool m_bIsInitD3D = false;
	HWND m_hWnd = NULL;

	NVGcontext* m_NVGcontext = nullptr;

	CComPtr<ID3D11Device>					m_pD3D11Device;
	CComPtr<ID3D11DeviceContext>			m_pD3D11DeviceContext;
	CComPtr<IDXGISwapChain>					m_pSwapChain;
	CComPtr<ID3D11RenderTargetView>			m_pRenderTargetView;
	
	RECT m_sharedTextureRect;
	CComPtr<ID3D11Texture2D> m_pSharedTexture = nullptr;
	HANDLE m_hSharedHandle = NULL;

	HANDLE m_hRenderFrameHandle = NULL;
	int m_imageID = 0;

	bool m_renderThrdRunning = false;
	std::thread m_renderThrd;

	struct DDAInfo
	{
		CComPtr<ID3D11Texture2D> sharedTexture = nullptr;
		std::shared_ptr<DDAImpl> ddaImpl = nullptr;
	};
	std::mutex m_ddasMutex;
	// std::vector<std::shared_ptr<DDAImpl>> m_ddaImpls;
	std::vector<std::shared_ptr<DDAInfo>> m_ddas;

	// NVGcontext* m_NVGcontext = nullptr;
};

