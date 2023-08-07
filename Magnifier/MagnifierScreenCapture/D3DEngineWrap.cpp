#include "D3DEngineWrap.h"

#include "MinHook.h"
#include "D3DRender/D3DRender.h"

#pragma comment(lib, "d3d9.lib")

#if 0
FUNCTIONHOOK(HRESULT, D3DDevice9_CreateTexture, IDirect3DDevice9Ex*, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
FUNCTIONHOOK(HRESULT, D3DDevice9_StretchRect, IDirect3DDevice9Ex* This, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
FUNCTIONHOOK(HRESULT, D3DDevice9_PresentEx, IDirect3DDevice9Ex*, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);

FUNCTIONIMPL(HRESULT, D3DDevice9_CreateTexture, IDirect3DDevice9Ex* This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
	HRESULT hr = OldD3DDevice9_CreateTexture(This, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
	if (SUCCEEDED(hr))
	{
		D3DEngineWrap::Instance()->SetSharedHandle(*pSharedHandle);
	}

	return hr;
}


FUNCTIONIMPL(HRESULT, D3DDevice9_StretchRect, IDirect3DDevice9Ex* This, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
	return S_OK;
}


FUNCTIONIMPL(HRESULT, D3DDevice9_PresentEx, IDirect3DDevice9Ex* This, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
	D3DEngineWrap::Instance()->Render();

	return S_OK;
}
#else

typedef HRESULT(WINAPI* D3DDevice9_CreateTexture)(IDirect3DDevice9Ex*, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
typedef HRESULT(WINAPI* D3DDevice9_StretchRect)(IDirect3DDevice9Ex* This, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
typedef HRESULT(WINAPI* D3DDevice9_PresentEx)(IDirect3DDevice9Ex*, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags);

D3DDevice9_CreateTexture OldD3DDevice9_CreateTexture = NULL;
D3DDevice9_StretchRect OldD3DDevice9_StretchRect = NULL;
D3DDevice9_PresentEx OldD3DDevice9_PresentEx = NULL;

HRESULT WINAPI NewD3DDevice9_CreateTexture(IDirect3DDevice9Ex* This, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle)
{
	HRESULT hr = OldD3DDevice9_CreateTexture(This, Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle);
	if (SUCCEEDED(hr))
	{
		D3DEngineWrap::Instance()->SetSharedHandle(*pSharedHandle);
	}

	return hr;
}

HRESULT WINAPI NewD3DDevice9_StretchRect(IDirect3DDevice9Ex* This, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
	return S_OK;
}

HRESULT WINAPI NewD3DDevice9_PresentEx(IDirect3DDevice9Ex*, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
	D3DEngineWrap::Instance()->Render();

	return S_OK;
}

#endif

D3DEngineWrap* D3DEngineWrap::m_instance = nullptr;
D3DEngineWrap::D3DEngineWrap()
{

}

D3DEngineWrap::~D3DEngineWrap()
{
	m_pD3D9Device = nullptr;
}

D3DEngineWrap* D3DEngineWrap::Instance()
{
	if (m_instance == nullptr)
	{
		m_instance = new D3DEngineWrap();
	}

	return m_instance;
}

void D3DEngineWrap::Release()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
	}
}

bool D3DEngineWrap::Initialize()
{
	if (m_bIntialized)
	{
		return true;
	}

	if (MH_Initialize() != MH_OK)
		return false;

	m_bIntialized = true;

	// 
	//CComPtr<IDirect3DDevice9>  pD3D9Device;
	// IDirect3DDevice9Ex* pD3D9Device = NULL;
	if (FAILED(CreateD3DEnvironment(&m_pD3D9Device)))
		return false;


	// HOOK D3D9Device

	INT64** pDevice9Vtbl = (INT64**)(m_pD3D9Device.p);

	// Hook CreateTexture，用于得到共享位面句柄
	{
		FARPROC hFunction = (FARPROC)pDevice9Vtbl[0][17 + 6];
		if (MH_CreateHook(hFunction, (LPVOID*)&NewD3DDevice9_CreateTexture, reinterpret_cast<LPVOID*>(&OldD3DDevice9_CreateTexture)) != MH_OK)
			return false;

		if (MH_EnableHook(hFunction) != MH_OK)
			return false;
	}

	// Hook StretchRect，禁用从共享位面->显示位面的复制，优化性能
	{
		FARPROC hFunction = (FARPROC)pDevice9Vtbl[0][17 + 17];
		if (MH_CreateHook(hFunction, (LPVOID*)&NewD3DDevice9_StretchRect, reinterpret_cast<LPVOID*>(&OldD3DDevice9_StretchRect)) != MH_OK)
			return false;

		if (MH_EnableHook(hFunction) != MH_OK)
			return false;
	}

	// Hook PresentEx，通知页面更新
	{
		FARPROC hFunction = (FARPROC)pDevice9Vtbl[0][17 + 104];
		if (MH_CreateHook(hFunction, (LPVOID*)&NewD3DDevice9_PresentEx, reinterpret_cast<LPVOID*>(&OldD3DDevice9_PresentEx)) != MH_OK)
			return false;

		if (MH_EnableHook(hFunction) != MH_OK)
			return false;
	}

	return true;
}


bool D3DEngineWrap::Uninitialize()
{
	if (m_bIntialized)
	{
		m_bIntialized = false;

		if (MH_Uninitialize() != MH_OK)
			return false;
	}
	
	return true;
}


HRESULT D3DEngineWrap::CreateD3DEnvironment(IDirect3DDevice9Ex** ppD3D9Device)
{
	CComPtr<IDirect3D9Ex>  pD3D9Ex;

	// Create the D3D object
	HRESULT hr = Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D9Ex);
	if (FAILED(hr))
		return hr;

	// Create the Direct3D device.
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	hr = pD3D9Ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, ::GetShellWindow(),
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, NULL, ppD3D9Device);
	if (FAILED(hr))
	{
		return E_FAIL;
	}


	return S_OK;
}


void D3DEngineWrap::SetSharedHandle(HANDLE sharedHandle)
{
	m_sharedHandle = sharedHandle;
}

void D3DEngineWrap::Render()
{
	D3DRender::Instance()->DrawFrame(m_sharedHandle);
}