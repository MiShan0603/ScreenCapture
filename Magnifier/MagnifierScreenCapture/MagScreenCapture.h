#pragma once

#include <vector>
#include <magnification.h>
#pragma comment(lib, "Magnification.lib")

#define MAGFACTOR  1.0f

class MagScreenCapture
{
public:
	~MagScreenCapture();

	static MagScreenCapture* Instance();
	static void Release();

private:
	MagScreenCapture();
	static MagScreenCapture* m_instance;

public:
	void SetCapRect(RECT rect);
	void SetExcludeHwnd(std::vector<HWND> hWnds);
	bool Init(int width, int height);

	void UpdateMagWindow();

private:
	HWND m_mapCaptureHwnd = NULL;
	HWND m_hwndMag = NULL;
	RECT m_capRect = { GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
		GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) };

	UINT_PTR m_timer = 0;
	UINT m_timerInterval = 32; // 16; // close to the refresh rate @60hz

	std::vector<HWND> m_ExcludeHwnds;
};

