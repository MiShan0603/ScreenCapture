#pragma once

#include <d3d9.h> 
#include <atlbase.h>

// 根据一大神的代码修改的，但是我找不到这个源码的出处了

class D3DEngineWrap
{
public:
	~D3DEngineWrap();

	static D3DEngineWrap* Instance();
	static void Release();

private:
	D3DEngineWrap();

	static D3DEngineWrap* m_instance;

public:
	 bool Initialize();
	 bool Uninitialize();

	 void SetSharedHandle(HANDLE sharedHandle);
	 void Render();

private:
	HRESULT CreateD3DEnvironment(IDirect3DDevice9Ex** ppD3D9Device);

private:
	bool m_bIntialized = false;
	HANDLE m_sharedHandle = NULL;
	CComPtr<IDirect3DDevice9Ex> m_pD3D9Device = nullptr;
};


#define FUNCTIONHOOK(result, name, ...) \
	typedef result ( WINAPI * td_##name )( __VA_ARGS__ ); \
extern td_##name Old##name; \
	result WINAPI New##name( __VA_ARGS__ );

#define FUNCTIONIMPL(result, name, ...) \
	td_##name Old##name = NULL; \
	result WINAPI New##name( __VA_ARGS__ )

