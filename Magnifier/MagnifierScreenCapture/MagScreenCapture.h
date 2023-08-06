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
	void SetExcludeHwnd(std::vector<HWND> hWnds);
	bool Init();

	void UpdateMagWindow();

private:
	HWND m_mapCaptureHwnd = NULL;
	HWND m_hwndMag = NULL;

	UINT_PTR m_timer = 0;
	UINT m_timerInterval = 32; // 16; // close to the refresh rate @60hz

	std::vector<HWND> m_ExcludeHwnds;
};

