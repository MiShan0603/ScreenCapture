
#include <Windows.h>
#include <tchar.h>
#include <codecvt>
#include <vector>
#include <string>
#include "D3DRender/D3DRender.h"
#include "WindowCapture.h"
   
DWORD _dwMainThread = 0;
DWORD GetMainThreadId() { 
	return _dwMainThread; 
}

HINSTANCE hInst = NULL;
HWND g_hMainWnd = NULL;

std::shared_ptr<WindowCapture> g_windowCapture = nullptr;

#pragma region Child Wnd

const int g_renderId = 0x800;
const int g_staticTextId = 0x801;
const int g_comboBoxId = 0x802;
const int g_checkBoxId = 0x803;
const int g_checkBoxShowCusorId = 0x804;

HWND g_renderHwnd = NULL;
HWND g_staticTextHwnd = NULL;
HWND g_comboBoxHwnd = NULL;
HWND g_checkBoxHwnd = NULL;
HWND g_checkBoxShowCusorHwnd = NULL;

#pragma endregion Child Wnd

#pragma region Montitor

struct MiMontitor
{
    HMONITOR hMontitor;
	BOOL allMontitor;
	RECT rect;
	WCHAR monitor_desc[256];
};

std::vector<MiMontitor> g_montitors;
int g_curMontitorIdx = -1;

BOOL CALLBACK enum_monitor_props(HMONITOR handle, HDC hdc, LPRECT rect, LPARAM param);

#pragma endregion Montitor




void CenterWindow(HWND hWnd);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	WNDCLASSEX wx = {};
	wx.cbSize = sizeof(WNDCLASSEX);
	wx.lpfnWndProc = MainWndProc;
	wx.hInstance = hInstance;
	wx.lpszClassName = _T("D3D9ScreenCaptureClass");
	RegisterClassEx(&wx);

    hInst = hInstance;
	g_hMainWnd = CreateWindowW(wx.lpszClassName, _T("D3D9ScreenCapture"), WS_OVERLAPPEDWINDOW,
		0, 0, 1280, 720, nullptr, nullptr, hInstance, nullptr);
	if (g_hMainWnd == NULL) 
	{
		return FALSE;
	}

	CenterWindow(g_hMainWnd);
	::ShowWindow(g_hMainWnd, SW_SHOW);
	UpdateWindow(g_hMainWnd);

	_dwMainThread = ::GetCurrentThreadId();


    D3DRender::Instance()->Init(g_renderHwnd);
    g_windowCapture = std::make_shared<WindowCapture>(D3DRender::Instance()->GetD3D11Device());
    g_windowCapture->Init(g_montitors[g_curMontitorIdx].rect, 30.0);

    int checkBoxValve = (int)SendMessage(g_checkBoxShowCusorHwnd, BM_GETCHECK, 0, 0);
    g_windowCapture->ShowCusor(checkBoxValve != 0);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
 		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	}

    g_windowCapture = nullptr;
    D3DRender::Release();
	CoUninitialize();

	return (int)msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // 分析菜单选择:
        switch (wmId)
        {
        case g_comboBoxId:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                INT selectIdx = SendMessage(g_comboBoxHwnd, CB_GETCURSEL, 0, 0);
                if (selectIdx != g_curMontitorIdx)
                {
                    g_curMontitorIdx = selectIdx;
                    
                    g_windowCapture = nullptr;
                    g_windowCapture = std::make_shared<WindowCapture>(D3DRender::Instance()->GetD3D11Device());
                    g_windowCapture->Init(g_montitors[g_curMontitorIdx].rect, 30.0);
                }
            }
            break;
        case g_checkBoxId:
        {
            int checkBoxValve = (int)SendMessage(g_checkBoxHwnd, BM_GETCHECK, 0, 0);
            if (checkBoxValve != 0)
            {
                SetWindowDisplayAffinity(g_hMainWnd, WDA_NONE);
            }
            else 
            {
                SetWindowDisplayAffinity(g_hMainWnd, WDA_EXCLUDEFROMCAPTURE);
            }

            break;
        }
        case g_checkBoxShowCusorId:
        {
            int checkBoxValve = (int)SendMessage(g_checkBoxShowCusorHwnd, BM_GETCHECK, 0, 0);
            if (checkBoxValve != 0)
            {
                
            }
            else
            {
                
            }

            break;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_CREATE:
    {
        RECT rect;
        GetClientRect(hWnd, &rect);

        g_renderHwnd = CreateWindow(TEXT("Static"), TEXT(""),
            WS_CHILD | WS_VISIBLE,
            0, 0, (rect.right - rect.left), (rect.bottom - rect.top) - 30, hWnd, (HMENU)g_renderId, hInst, NULL);
        ::ShowWindow(g_renderHwnd, SW_SHOW);

        g_staticTextHwnd = CreateWindow(TEXT("Static"), TEXT("Montitor : "),
            WS_CHILD | WS_VISIBLE,
            1, (rect.bottom - rect.top) - 29, 80, 30, hWnd, (HMENU)g_staticTextId, hInst, NULL);
        ::ShowWindow(g_staticTextHwnd, SW_SHOW);

        // combox
        {
            g_comboBoxHwnd = CreateWindow(TEXT("COMBOBOX"), TEXT("hWComBoxList"),
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
                81, (rect.bottom - rect.top) - 29, (rect.right - rect.left) - 340, 300, hWnd,
                (HMENU)g_comboBoxId, hInst, NULL);
            ::ShowWindow(g_comboBoxHwnd, SW_SHOW);

            EnumDisplayMonitors(NULL, NULL, enum_monitor_props, NULL);

            if (g_montitors.size() > 1)
            {
                MiMontitor miMontitor;
                miMontitor.hMontitor = NULL;
                miMontitor.rect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
                miMontitor.rect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
                miMontitor.rect.right = miMontitor.rect.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
                miMontitor.rect.bottom = miMontitor.rect.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
                swprintf_s(miMontitor.monitor_desc, 256,
                    TEXT("All Montitors: %dx%d @ %d,%d"),
                    miMontitor.rect.right - miMontitor.rect.left,
                    miMontitor.rect.bottom - miMontitor.rect.top,
                    miMontitor.rect.left, miMontitor.rect.top);

                miMontitor.allMontitor = TRUE;
                g_montitors.push_back(miMontitor);
            }

            for (auto itr = g_montitors.begin(); itr != g_montitors.end(); ++itr)
            {
                SendMessage(g_comboBoxHwnd, CB_ADDSTRING, 0, (LPARAM)itr->monitor_desc);
            }

            if (g_montitors.size() > 0)
            {
                g_curMontitorIdx = 0;
                SendMessage(g_comboBoxHwnd, CB_SETCURSEL, 0, 0);
            }
        }
        

        g_checkBoxHwnd = CreateWindow(TEXT("button"), TEXT("Capture MySelf"),
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            (rect.right - rect.left) - 255, (rect.bottom - rect.top) - 29, 120, 30,
            hWnd, (HMENU)g_checkBoxId, hInst, NULL);
        ::ShowWindow(g_checkBoxHwnd, SW_SHOW);
        SendMessage(g_checkBoxHwnd, BM_SETCHECK, 1, 0);

        g_checkBoxShowCusorHwnd = CreateWindow(TEXT("button"), TEXT("Show Cusor"),
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            (rect.right - rect.left) - 123, (rect.bottom - rect.top) - 29, 120, 30,
            hWnd, (HMENU)g_checkBoxShowCusorId, hInst, NULL);
        ::ShowWindow(g_checkBoxShowCusorHwnd, SW_SHOW);
        SendMessage(g_checkBoxShowCusorHwnd, BM_SETCHECK, 1, 0);

        break;
    }
	case WM_DESTROY:
    {
        ::DestroyWindow(g_checkBoxHwnd);
        ::DestroyWindow(g_comboBoxHwnd);
        ::DestroyWindow(g_renderHwnd);
        PostQuitMessage(0);
        break;
    }
	case WM_TIMER:
	{
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


void CenterWindow(HWND hWnd)
{
    RECT rect{ };
    GetWindowRect(hWnd, &rect);
    const auto width{ rect.right - rect.left };
    const auto height{ rect.bottom - rect.top };
    const auto cx{ GetSystemMetrics(SM_CXFULLSCREEN) }; // 取显示器屏幕高宽
    const auto cy{ GetSystemMetrics(SM_CYFULLSCREEN) };
    const auto x{ cx / 2 - width / 2 };
    const auto y{ cy / 2 - height / 2 };
    MoveWindow(hWnd, x, y, width, height, false); // 移动窗口位置居中
}

#pragma region MonitorProps

bool GetMonitorTarget(LPCWSTR device, DISPLAYCONFIG_TARGET_DEVICE_NAME* target)
{
    bool found = false;

    UINT32 numPath, numMode;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &numPath, &numMode) == ERROR_SUCCESS)
    {
        DISPLAYCONFIG_PATH_INFO* paths = new DISPLAYCONFIG_PATH_INFO[numPath];
        DISPLAYCONFIG_MODE_INFO* modes = new DISPLAYCONFIG_MODE_INFO[numMode];
        if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPath, paths, &numMode, modes, NULL) == ERROR_SUCCESS)
        {
            for (size_t i = 0; i < numPath; ++i)
            {
                const DISPLAYCONFIG_PATH_INFO* const path = &paths[i];

                DISPLAYCONFIG_SOURCE_DEVICE_NAME source;
                source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                source.header.size = sizeof(source);
                source.header.adapterId = path->sourceInfo.adapterId;
                source.header.id = path->sourceInfo.id;
                if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
                    wcscmp(device, source.viewGdiDeviceName) == 0)
                {
                    target->header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                    target->header.size = sizeof(*target);
                    target->header.adapterId = path->sourceInfo.adapterId;
                    target->header.id = path->targetInfo.id;
                    found = DisplayConfigGetDeviceInfo(&target->header) == ERROR_SUCCESS;
                    break;
                }
            }
        }

        delete[](modes);
        delete[](paths);
    }

    return found;
}

void GetMonitorName(HMONITOR handle, WCHAR* name, size_t count)
{
    MONITORINFOEXW mi;
    DISPLAYCONFIG_TARGET_DEVICE_NAME target;

    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(handle, (LPMONITORINFO)&mi) && GetMonitorTarget(mi.szDevice, &target))
    {
        wcscpy_s(name, count, target.monitorFriendlyDeviceName);
    }
    else
    {
        wcscpy_s(name, count, L"[Unknown]");
    }
}

BOOL CALLBACK enum_monitor_props(HMONITOR handle, HDC hdc, LPRECT rect, LPARAM param)
{
    WCHAR monitor_name[64] = { 0 };
    ZeroMemory(&monitor_name, sizeof(monitor_name));
    GetMonitorName(handle, monitor_name, sizeof(monitor_name) / sizeof(WCHAR));

    MONITORINFOEX mi;
    ZeroMemory(&mi, sizeof(MONITORINFOEX));
    mi.cbSize = sizeof(MONITORINFOEX);
    if (GetMonitorInfo(handle, &mi))
    {
        MiMontitor miMontitor;
        ZeroMemory(&miMontitor, sizeof(MiMontitor));
        miMontitor.hMontitor = handle;
        miMontitor.rect = mi.rcMonitor;

        swprintf_s(miMontitor.monitor_desc, 256, TEXT("%s: %dx%d @ %d,%d"), monitor_name,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            mi.rcMonitor.left, mi.rcMonitor.top);
        if (mi.dwFlags == MONITORINFOF_PRIMARY)
            wcscat_s(miMontitor.monitor_desc, TEXT(" (PrimaryMonitor)"));

        g_montitors.push_back(miMontitor);
    }

    return TRUE;
}

#pragma endregion MonitorProps