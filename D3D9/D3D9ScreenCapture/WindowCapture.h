#pragma once

#include <d3d11.h>
#include <d3d9.h>
#include <atlbase.h>
#include <vector>
#include <memory>

#pragma comment(lib, "d3d9.lib")

class WindowCapture
{
public:
	WindowCapture(ID3D11Device *pD3D11Device);
	~WindowCapture();

	BOOL Init(RECT monitorRect, float fps = 30.0);
	void Release();

	void ShowCusor(BOOL show);

private:
	BOOL m_bCaptureThrdRunning;
	HANDLE m_hCaptureThrd;
	static DWORD WINAPI CaptureThrd(LPVOID lpParam);
	void DoCapture();

	/// <summary>
	/// 计算rect1 2是否重合以及重合的范围
	/// </summary>
	/// <param name="rect1"></param>
	/// <param name="rect2"></param>
	/// <param name="overlapRect"></param>
	/// <returns></returns>
	BOOL RectIsOverlap(RECT rect1, RECT rect2, RECT& overlapRect);

private:
	CComPtr<ID3D11Device> m_d3d11Device;
	CComPtr<ID3D11DeviceContext> m_d3d11Context;
	CComPtr<ID3D11Texture2D> m_sharedTexture;
	HANDLE m_sharedHandle;

	CComPtr<IDirect3D9Ex> m_pD3D9Ex;

	struct D3D9Dev
	{
		CComPtr<IDirect3DDevice9Ex> pD3D9Device;
		CComPtr<IDirect3DTexture9>  pScreenTexture;

		CComPtr<ID3D11Texture2D> pSharedTexture;
		HANDLE hSharedHandle;

		RECT rect;
	};

	std::vector <std::shared_ptr<D3D9Dev>> m_d3d9Devs;
	
	BOOL m_showCursor = FALSE;

	RECT m_monitorRect;
	float m_fps = 30.0;
};

