#include "MagScreenCapture.h"

LRESULT CALLBACK MDWinCap_WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_TIMER:
	{
		MagScreenCapture::Instance()->UpdateMagWindow();

		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

MagScreenCapture* MagScreenCapture::m_instance = nullptr;
MagScreenCapture::MagScreenCapture()
{

}

MagScreenCapture::~MagScreenCapture()
{
	::KillTimer(m_mapCaptureHwnd, m_timer);

	::DestroyWindow(m_hwndMag);
	::DestroyWindow(m_mapCaptureHwnd);
	MagUninitialize();
}

MagScreenCapture* MagScreenCapture::Instance()
{
	if (m_instance == nullptr)
	{
		m_instance = new MagScreenCapture();
	}

	return m_instance;
}

void MagScreenCapture::Release()
{
	if (m_instance)
	{
		delete m_instance;
		m_instance = nullptr;
	}
}

void MagScreenCapture::SetExcludeHwnd(std::vector<HWND> hWnds)
{
	m_ExcludeHwnds.clear();
	for (auto hwnd : hWnds)
	{
		m_ExcludeHwnds.push_back(hwnd);
	}
}

bool MagScreenCapture::Init()
{
	if (!MagInitialize())
	{
		return false;
	}

	int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	TCHAR class_name[MAX_PATH] = { 0 };
	TCHAR window_name[MAX_PATH] = { 0 };
	wsprintf(class_name, TEXT("mag_capture_class_%d"), GetTickCount());
	wsprintf(window_name, TEXT("mag_capture_win_%d"), GetTickCount());

	WNDCLASSEX wx = {};
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = MDWinCap_WndProc;        // function which will handle messages
	wx.hInstance = NULL;//  afxCurrentInstanceHandle();
	wx.lpszClassName = class_name;
	if (!RegisterClassEx(&wx))
	{
		return false;
	}

	m_mapCaptureHwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, class_name, window_name, WS_POPUP | WS_CLIPSIBLINGS | WS_OVERLAPPED, 0, 0,
		screenWidth, screenHeight, NULL, NULL, NULL, NULL);

	::ShowWindow(m_mapCaptureHwnd, SW_HIDE);
	SetLayeredWindowAttributes(m_mapCaptureHwnd, 0, 255, LWA_ALPHA);

	HINSTANCE hInstance = GetModuleHandle(0);
	m_hwndMag = CreateWindow(WC_MAGNIFIER, TEXT("MagnifierWindow"),
		WS_CHILD | WS_VISIBLE,
		0, 0, screenWidth, screenHeight,
		m_mapCaptureHwnd, NULL, hInstance, NULL);
	if (m_hwndMag == NULL)
	{
		int error = GetLastError();

		return false;
	}

	// Set the magnification factor.
	MAGTRANSFORM matrix;
	memset(&matrix, 0, sizeof(matrix));
	matrix.v[0][0] = MAGFACTOR;
	matrix.v[1][1] = MAGFACTOR;
	matrix.v[2][2] = 1.0f;

	BOOL ret = MagSetWindowTransform(m_hwndMag, &matrix);

	m_timer = ::SetTimer(m_mapCaptureHwnd, 0, m_timerInterval, NULL);
}

void MagScreenCapture::UpdateMagWindow()
{
	if (m_ExcludeHwnds.size() > 0)
	{
		HWND filters[100] = { 0 };
		int filterCount = m_ExcludeHwnds.size();
		int index = 0;
		for (auto hwnd : m_ExcludeHwnds)
		{
			filters[index] = hwnd;
			index++;
		}

		MagSetWindowFilterList(m_hwndMag, MW_FILTERMODE_EXCLUDE, filterCount, filters);
	}

	int screenWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	RECT sourceRect = { 0 };
	sourceRect.right = screenWidth;
	sourceRect.bottom = screenHeight;
	MagSetWindowSource(m_hwndMag, sourceRect);
}