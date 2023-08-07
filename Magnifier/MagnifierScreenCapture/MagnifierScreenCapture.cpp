// MagnifierScreenCapture.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "MagnifierScreenCapture.h"

#include "D3DRender/D3DRender.h"
#include "D3DEngineWrap.h"
#include "MagScreenCapture.h"
#include <vector>
#include <string>

#define MAX_LOADSTRING 100



HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
HWND g_hWnd = NULL;

#pragma region Child Wnd

const int g_renderId = 0x800;
const int g_staticTextId = 0x801;
const int g_comboBoxId = 0x802;

HWND g_renderHwnd = NULL;
HWND g_staticTextHwnd = NULL;
HWND g_comboBoxHwnd = NULL;

#pragma endregion Child Wnd

#pragma region Montitor

struct MiMontitor
{
    BOOL allMontitor;
    RECT rect;
    WCHAR monitor_desc[256];
};

std::vector<MiMontitor> g_montitors;
int g_curMontitorIdx = -1;

BOOL CALLBACK enum_monitor_props(HMONITOR handle, HDC hdc, LPRECT rect, LPARAM param);

#pragma endregion Montitor


ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void CenterWindow(HWND hWnd);


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    D3DEngineWrap::Instance()->Initialize();

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_MAGNIFIERSCREENCAPTURE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
    {
        D3DEngineWrap::Instance()->Uninitialize();
        D3DEngineWrap::Release();

        return FALSE;
    }

    // filter out the current window
    std::vector<HWND> hwnds;
    hwnds.push_back(g_hWnd);
    MagScreenCapture::Instance()->SetExcludeHwnd(hwnds);
    RECT capRect = { GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), 
        GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) };
    if (g_montitors.size() > 0 && g_curMontitorIdx >= 0)
    {
        capRect = g_montitors[g_curMontitorIdx].rect;
    }

    MagScreenCapture::Instance()->SetCapRect(capRect);
    MagScreenCapture::Instance()->Init(capRect.right - capRect.left, capRect.bottom - capRect.top);

    // Specify the rendering window
    D3DRender::Instance()->Init(g_renderHwnd);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MAGNIFIERSCREENCAPTURE));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    D3DEngineWrap::Instance()->Uninitialize();
    D3DEngineWrap::Release();

    MagScreenCapture::Release();
    D3DRender::Release();

    return (int) msg.wParam;
}


ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAGNIFIERSCREENCAPTURE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL; MAKEINTRESOURCEW(IDC_MAGNIFIERSCREENCAPTURE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   g_hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
       0, 0, 1280, 720, nullptr, nullptr, hInstance, nullptr);

   if (!g_hWnd)
   {
      return FALSE;
   }

   CenterWindow(g_hWnd);
   
   ShowWindow(g_hWnd, nCmdShow);
   UpdateWindow(g_hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case g_comboBoxId:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    INT selectIdx = SendMessage(g_comboBoxHwnd, CB_GETCURSEL, 0, 0);
                    if (selectIdx != g_curMontitorIdx)
                    {
                        if (g_montitors[g_curMontitorIdx].rect.right - g_montitors[g_curMontitorIdx].rect.left == g_montitors[selectIdx].rect.right - g_montitors[selectIdx].rect.left &&
                            g_montitors[g_curMontitorIdx].rect.bottom - g_montitors[g_curMontitorIdx].rect.top == g_montitors[selectIdx].rect.bottom - g_montitors[selectIdx].rect.top)
                        {
                            g_curMontitorIdx = selectIdx;

                            MagScreenCapture::Instance()->SetCapRect(g_montitors[g_curMontitorIdx].rect);
                        }
                        else
                        {
                            g_curMontitorIdx = selectIdx;

                            MagScreenCapture::Release();

                            std::vector<HWND> hwnds;
                            hwnds.push_back(g_hWnd);
                            MagScreenCapture::Instance()->SetExcludeHwnd(hwnds);
                            RECT capRect = { GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                                GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) };
                            if (g_montitors.size() > 0 && g_curMontitorIdx >= 0)
                            {
                                capRect = g_montitors[g_curMontitorIdx].rect;
                            }

                            MagScreenCapture::Instance()->SetCapRect(capRect);
                            MagScreenCapture::Instance()->Init(capRect.right - capRect.left, capRect.bottom - capRect.top);
                        }
                    }
                    
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
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
            1, (rect.bottom - rect.top) - 29, 80, 300, hWnd, (HMENU)g_staticTextId, hInst, NULL);
        ::ShowWindow(g_staticTextHwnd, SW_SHOW);

        g_comboBoxHwnd = CreateWindow(TEXT("COMBOBOX"), TEXT("hWComBoxList"), 
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
            81, (rect.bottom - rect.top) - 29, (rect.right - rect.left) - 82, 300, hWnd, 
            (HMENU)g_comboBoxId, hInst, NULL);
        ::ShowWindow(g_comboBoxHwnd, SW_SHOW);

        EnumDisplayMonitors(NULL, NULL, enum_monitor_props, NULL);

        if (g_montitors.size() > 1)
        {
            MiMontitor miMontitor;
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
        
        break;
    }
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        ::DestroyWindow(g_comboBoxHwnd);
        ::DestroyWindow(g_renderHwnd);
        PostQuitMessage(0);
        break;
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
                    wcscmp(device, source.viewGdiDeviceName) ==  0)
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
        delete [](paths);
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
        miMontitor.rect = mi.rcMonitor;

        swprintf_s(miMontitor.monitor_desc, 256,  TEXT("%s: %dx%d @ %d,%d"), monitor_name,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            mi.rcMonitor.left, mi.rcMonitor.top);
        if (mi.dwFlags == MONITORINFOF_PRIMARY)
            wcscat(miMontitor.monitor_desc, TEXT(" (PrimaryMonitor)"));

        g_montitors.push_back(miMontitor);
    }

    return TRUE;
}

#pragma endregion MonitorProps