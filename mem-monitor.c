#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <shellapi.h>
#include <math.h>
#include <tchar.h>
#include <strsafe.h>
#include <commctrl.h>
#include <winreg.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")

#ifndef PDH_CSTATUS_VALID_DATA
#define PDH_CSTATUS_VALID_DATA 0x0
#endif

#define WM_TRAYICON (WM_USER + 1)
#define TIMER_ID    1
#define ICON_SIZE 64
#define AUTO_START_REG_VALUE _T("MemMonitor")
#define WEBSITE_URL _T("https://github.com/lutischan-ferenc/resource-monitor-v2")

enum {
    ID_MENU_OPEN_WEBSITE = 1001,
    ID_MENU_USED,
    ID_MENU_FREE,
    ID_MENU_CACHED,
    ID_MENU_ACTIVE, // Meghagyjuk az enumot, de nem használjuk
    ID_MENU_SWAP,
    ID_MENU_AUTOSTART,
    ID_MENU_EXIT
};

typedef struct {
    DWORDLONG totalPhys;
    DWORDLONG availPhys;
    DWORDLONG usedPhys;
    DWORDLONG cachedBytes;
    DWORDLONG committedBytes;
    DWORDLONG swapUsed;
    double usedPercent;
} MEMINFO;

HINSTANCE    g_hInst;
HWND         g_hWnd;
HMENU        g_hMenu;
HBITMAP      hBmpUsed = NULL;
HBITMAP      hBmpFree = NULL;
NOTIFYICONDATA g_nid = {0};
HICON        g_hIcon = NULL;
BOOL         g_autoStartChecked = FALSE;
DWORDLONG    g_lastUsedMB = 0;
PDH_HQUERY   g_pdhQuery = NULL;
PDH_HCOUNTER g_pdhCacheBytes = NULL;
PDH_HCOUNTER g_pdhModifiedPageListBytes = NULL;
PDH_HCOUNTER g_pdhStandbyCoreBytes = NULL;
PDH_HCOUNTER g_pdhStandbyNormalBytes = NULL;
PDH_HCOUNTER g_pdhStandbyReserveBytes = NULL;
PDH_HCOUNTER g_pdhCommittedBytes = NULL;

void UpdateMemoryInfo(MEMINFO *mi);
HICON GeneratePieIcon(MEMINFO *mi);
void OpenBrowser(LPCTSTR url);
BOOL IsAutoStartEnabled();
BOOL SetAutoStart(BOOL enable);
void UpdateTrayMenu(MEMINFO *mi);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitPdhCounter();
void CleanupPdhCounter();
void InitializeMenuWithIcons();
void CleanupMenuIcons();
HBITMAP CreateColorBitmap(COLORREF color, int width, int height);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                       LPTSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = _T("MemMonitorHiddenWnd");
    RegisterClassEx(&wcex);

    g_hWnd = CreateWindowEx(0, wcex.lpszClassName, _T("Mem Monitor"),
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    g_hMenu = CreatePopupMenu();
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_OPEN_WEBSITE, _T("Mem Monitor v2.0.0"));
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_USED, _T("Used: 0 MB"));
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_FREE, _T("Free: 0 MB"));
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_CACHED, _T("Cached: 0 MB"));
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_SWAP, _T("Swap: 0 MB"));
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_AUTOSTART, _T("Start on System Startup"));
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_EXIT, _T("Exit"));

    InitializeMenuWithIcons();

    ZeroMemory(&g_nid, sizeof(NOTIFYICONDATA));
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    StringCchCopy(g_nid.szTip, sizeof(g_nid.szTip)/sizeof(TCHAR), _T("Memory Monitor"));
    g_hIcon = LoadIcon(NULL, IDI_APPLICATION);
    g_nid.hIcon = g_hIcon;
    Shell_NotifyIcon(NIM_ADD, &g_nid);

    g_autoStartChecked = IsAutoStartEnabled();

    if (!InitPdhCounter()) {
        PostQuitMessage(1);
        return 1;
    }

    SetTimer(g_hWnd, TIMER_ID, 2000, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupMenuIcons();
    CleanupPdhCounter();
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    return (int)msg.wParam;
}

BOOL InitPdhCounter()
{
    PDH_STATUS status;
    status = PdhOpenQuery(NULL, 0, &g_pdhQuery);
    if (status != ERROR_SUCCESS) return FALSE;

    status = PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Cache Bytes"), 0, &g_pdhCacheBytes);
    if (status != ERROR_SUCCESS) return FALSE;

    status = PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Modified Page List Bytes"), 0, &g_pdhModifiedPageListBytes);
    if (status != ERROR_SUCCESS) return FALSE;

    status = PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Standby Cache Core Bytes"), 0, &g_pdhStandbyCoreBytes);
    if (status != ERROR_SUCCESS) return FALSE;

    status = PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Standby Cache Normal Priority Bytes"), 0, &g_pdhStandbyNormalBytes);
    if (status != ERROR_SUCCESS) return FALSE;

    status = PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Standby Cache Reserve Bytes"), 0, &g_pdhStandbyReserveBytes);
    if (status != ERROR_SUCCESS) return FALSE;

    status = PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Committed Bytes"), 0, &g_pdhCommittedBytes);
    if (status != ERROR_SUCCESS) return FALSE;

    return TRUE;
}

void CleanupPdhCounter()
{
    if (g_pdhCacheBytes) PdhRemoveCounter(g_pdhCacheBytes);
    if (g_pdhModifiedPageListBytes) PdhRemoveCounter(g_pdhModifiedPageListBytes);
    if (g_pdhStandbyCoreBytes) PdhRemoveCounter(g_pdhStandbyCoreBytes);
    if (g_pdhStandbyNormalBytes) PdhRemoveCounter(g_pdhStandbyNormalBytes);
    if (g_pdhStandbyReserveBytes) PdhRemoveCounter(g_pdhStandbyReserveBytes);
    if (g_pdhCommittedBytes) PdhRemoveCounter(g_pdhCommittedBytes);
    if (g_pdhQuery) PdhCloseQuery(g_pdhQuery);
}

HBITMAP CreateColorBitmap(COLORREF color, int width, int height) {
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hBitmap);
    
    HBRUSH hBrush = CreateSolidBrush(color);
    RECT rect = {0, 0, width, height};
    FillRect(hdcMem, &rect, hBrush);
    
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBrush);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    
    return hBitmap;
}

void InitializeMenuWithIcons() {
    hBmpUsed = CreateColorBitmap(RGB(200, 200, 200), 16, 16);
    hBmpFree = CreateColorBitmap(RGB(100, 100, 100), 16, 16);
    
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_BITMAP;
    
    mii.hbmpItem = hBmpUsed;
    SetMenuItemInfo(g_hMenu, ID_MENU_USED, FALSE, &mii);
    
    mii.hbmpItem = hBmpFree;
    SetMenuItemInfo(g_hMenu, ID_MENU_FREE, FALSE, &mii);
}

void CleanupMenuIcons() {
    if (hBmpUsed) DeleteObject(hBmpUsed);
    if (hBmpFree) DeleteObject(hBmpFree);
}

void UpdateMemoryInfo(MEMINFO *mi)
{
    MEMORYSTATUSEX statex = { sizeof(MEMORYSTATUSEX) };
    GlobalMemoryStatusEx(&statex);
    mi->totalPhys = statex.ullTotalPhys;
    mi->availPhys = statex.ullAvailPhys;
    mi->usedPhys = mi->totalPhys - mi->availPhys;
    mi->usedPercent = (double)mi->usedPhys * 100.0 / (double)mi->totalPhys;

    PDH_STATUS status;
    PDH_FMT_COUNTERVALUE counterValue;
    DWORDLONG totalCached = 0;

    status = PdhCollectQueryData(g_pdhQuery);
    if (status == ERROR_SUCCESS) {
        if (PdhGetFormattedCounterValue(g_pdhCacheBytes, PDH_FMT_LARGE, NULL, &counterValue) == ERROR_SUCCESS &&
            counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            totalCached += counterValue.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhModifiedPageListBytes, PDH_FMT_LARGE, NULL, &counterValue) == ERROR_SUCCESS &&
            counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            totalCached += counterValue.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhStandbyCoreBytes, PDH_FMT_LARGE, NULL, &counterValue) == ERROR_SUCCESS &&
            counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            totalCached += counterValue.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhStandbyNormalBytes, PDH_FMT_LARGE, NULL, &counterValue) == ERROR_SUCCESS &&
            counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            totalCached += counterValue.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhStandbyReserveBytes, PDH_FMT_LARGE, NULL, &counterValue) == ERROR_SUCCESS &&
            counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            totalCached += counterValue.largeValue;
        }
        mi->cachedBytes = totalCached;
    }

    if (PdhGetFormattedCounterValue(g_pdhCommittedBytes, PDH_FMT_LARGE, NULL, &counterValue) == ERROR_SUCCESS &&
        counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
        mi->committedBytes = counterValue.largeValue;
    } else {
        mi->committedBytes = 0;
    }

    mi->swapUsed = (mi->committedBytes > mi->usedPhys) ? (mi->committedBytes - mi->usedPhys) : 0;
}

HICON GeneratePieIcon(MEMINFO *mi)
{
    const int supersample = 2;
    const int ss_size = ICON_SIZE * supersample;
    const int center = ss_size / 2;
    const int radius = ss_size / 2 - 1;

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = ss_size;
    bmi.bmiHeader.biHeight = -ss_size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    void *pBits;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HGDIOBJ hOldBmp = SelectObject(hdcMem, hBmp);

    RECT rect = {0, 0, ss_size, ss_size};
    HBRUSH hbrBackground = CreateSolidBrush(RGB(200, 200, 200));
    FillRect(hdcMem, &rect, hbrBackground);

    HBRUSH hbrUsed = CreateSolidBrush(RGB(200, 200, 200));
    HBRUSH hbrFree = CreateSolidBrush(RGB(100, 100, 100));

    double usedPercent = mi->usedPercent;
    double freePercent = 100.0 - usedPercent;

    double startAngle = -90.0;
    struct {
        double percent;
        HBRUSH brush;
    } slices[] = {
        {usedPercent, hbrUsed},
        {freePercent, hbrFree}
    };
    int sliceCount = sizeof(slices) / sizeof(slices[0]);

    for (int i = 0; i < sliceCount; i++) {
        if (slices[i].percent > 0.0) {
            double endAngle = startAngle + slices[i].percent * 3.6;
            double radStart = startAngle * (3.14159265 / 180.0);
            double radEnd = endAngle * (3.14159265 / 180.0);

            POINT points[100];
            int pointCount = 0;
            double angleStep = (radEnd - radStart) / 99.0;
            for (int j = 0; j < 100; j++) {
                double angle = radStart + j * angleStep;
                points[pointCount].x = center + (int)(radius * cos(angle));
                points[pointCount].y = center + (int)(radius * sin(angle));
                pointCount++;
            }

            points[pointCount].x = center;
            points[pointCount].y = center;
            pointCount++;

            SelectObject(hdcMem, slices[i].brush);
            SelectObject(hdcMem, GetStockObject(NULL_PEN));
            Polygon(hdcMem, points, pointCount);

            startAngle = endAngle;
        }
    }

    DeleteObject(hbrUsed);
    DeleteObject(hbrFree);

    HDC hdcFinal = CreateCompatibleDC(hdcScreen);
    BITMAPINFO bmi_final = {0};
    bmi_final.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi_final.bmiHeader.biWidth = ICON_SIZE;
    bmi_final.bmiHeader.biHeight = -ICON_SIZE;
    bmi_final.bmiHeader.biPlanes = 1;
    bmi_final.bmiHeader.biBitCount = 32;
    void *pBitsFinal;
    HBITMAP hBmpFinal = CreateDIBSection(hdcFinal, &bmi_final, DIB_RGB_COLORS, &pBitsFinal, NULL, 0);
    HGDIOBJ hOldFinalBmp = SelectObject(hdcFinal, hBmpFinal);

    SetStretchBltMode(hdcFinal, HALFTONE);
    StretchBlt(hdcFinal, 0, 0, ICON_SIZE, ICON_SIZE,
               hdcMem, 0, 0, ss_size, ss_size, SRCCOPY);

    BYTE *pPixel = (BYTE *)pBitsFinal;
    for (int y = 0; y < ICON_SIZE; y++) {
        for (int x = 0; x < ICON_SIZE; x++) {
            int dx = x - ICON_SIZE / 2;
            int dy = y - ICON_SIZE / 2;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > ICON_SIZE / 2) {
                pPixel[3] = 0x00;
            } else if (dist > ICON_SIZE / 2 - 1.5f) {
                float alpha = 1.0f - (dist - (ICON_SIZE / 2 - 1.5f));
                pPixel[3] = (BYTE)(alpha * 255);
            } else {
                pPixel[3] = 0xFF;
            }
            pPixel += 4;
        }
    }

    HBITMAP hMonoBmp = CreateBitmap(ICON_SIZE, ICON_SIZE, 1, 1, NULL);
    HDC hdcMask = CreateCompatibleDC(NULL);
    HGDIOBJ hOldMaskBmp = SelectObject(hdcMask, hMonoBmp);
    PatBlt(hdcMask, 0, 0, ICON_SIZE, ICON_SIZE, BLACKNESS);

    HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    SelectObject(hdcMask, hWhiteBrush);
    Ellipse(hdcMask, 0, 0, ICON_SIZE, ICON_SIZE);

    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmMask = hMonoBmp;
    ii.hbmColor = hBmpFinal;
    HICON hIcon = CreateIconIndirect(&ii);

    SelectObject(hdcMem, hOldBmp);
    SelectObject(hdcFinal, hOldFinalBmp);
    SelectObject(hdcMask, hOldMaskBmp);
    DeleteObject(hbrBackground);
    DeleteObject(hWhiteBrush);
    DeleteDC(hdcMem);
    DeleteDC(hdcFinal);
    DeleteDC(hdcMask);
    ReleaseDC(NULL, hdcScreen);
    DeleteObject(hBmp);
    DeleteObject(hBmpFinal);
    DeleteObject(hMonoBmp);

    return hIcon;
}

void OpenBrowser(LPCTSTR url)
{
    ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
}

BOOL IsAutoStartEnabled()
{
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    DWORD dwSize = sizeof(szValue);
    BOOL result = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, AUTO_START_REG_VALUE, NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            if (_tcslen(szValue) > 0)
                result = TRUE;
        }
        RegCloseKey(hKey);
    }
    return result;
}

BOOL SetAutoStart(BOOL enable)
{
    HKEY hKey;
    LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                             0, KEY_WRITE, &hKey);
    if (lRes != ERROR_SUCCESS)
        return FALSE;

    BOOL result = FALSE;
    if (enable) {
        TCHAR szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
            lRes = RegSetValueEx(hKey, AUTO_START_REG_VALUE, 0, REG_SZ, (const BYTE *)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
            result = (lRes == ERROR_SUCCESS);
        }
    } else {
        lRes = RegDeleteValue(hKey, AUTO_START_REG_VALUE);
        result = (lRes == ERROR_SUCCESS) || (lRes == ERROR_FILE_NOT_FOUND);
    }
    RegCloseKey(hKey);
    return result;
}

void UpdateTrayMenu(MEMINFO *mi)
{
    TCHAR buf[64];
    DWORD usedMB = (DWORD)(mi->usedPhys / (1024 * 1024));
    DWORD freeMB = (DWORD)(mi->availPhys / (1024 * 1024));
    DWORD cachedMB = (DWORD)(mi->cachedBytes / (1024 * 1024));
    DWORD swapMB = (DWORD)(mi->swapUsed / (1024 * 1024));

    StringCchPrintf(buf, 64, _T("Used: %lu MB"), usedMB);
    ModifyMenu(g_hMenu, ID_MENU_USED, MF_BYCOMMAND | MF_STRING, ID_MENU_USED, buf);
    
    StringCchPrintf(buf, 64, _T("Free: %lu MB"), freeMB);
    ModifyMenu(g_hMenu, ID_MENU_FREE, MF_BYCOMMAND | MF_STRING, ID_MENU_FREE, buf);
    
    StringCchPrintf(buf, 64, _T("Cached: %lu MB"), cachedMB);
    ModifyMenu(g_hMenu, ID_MENU_CACHED, MF_BYCOMMAND | MF_STRING, ID_MENU_CACHED, buf);
    
    StringCchPrintf(buf, 64, _T("Swap: %lu MB"), swapMB);
    ModifyMenu(g_hMenu, ID_MENU_SWAP, MF_BYCOMMAND | MF_STRING, ID_MENU_SWAP, buf);

    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_BITMAP;
    
    mii.hbmpItem = hBmpUsed;
    SetMenuItemInfo(g_hMenu, ID_MENU_USED, FALSE, &mii);
    
    mii.hbmpItem = hBmpFree;
    SetMenuItemInfo(g_hMenu, ID_MENU_FREE, FALSE, &mii);

    StringCchPrintf(g_nid.szTip, sizeof(g_nid.szTip)/sizeof(TCHAR),
                    _T("Memory Usage: %.2f%%"), mi->usedPercent);
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);

    if (abs((int)usedMB - (int)g_lastUsedMB) > 50) {
        g_lastUsedMB = usedMB;
        HICON hNewIcon = GeneratePieIcon(mi);
        if (hNewIcon) {
            if (g_hIcon) DestroyIcon(g_hIcon);
            g_hIcon = hNewIcon;
            g_nid.hIcon = g_hIcon;
            Shell_NotifyIcon(NIM_MODIFY, &g_nid);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static MEMINFO meminfo;
    switch (message) {
    case WM_TIMER:
        if (wParam == TIMER_ID) {
            UpdateMemoryInfo(&meminfo);
            UpdateTrayMenu(&meminfo);
        }
        break;
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            ModifyMenu(g_hMenu, ID_MENU_AUTOSTART, 
                      MF_BYCOMMAND | MF_STRING | (g_autoStartChecked ? MF_CHECKED : 0),
                      ID_MENU_AUTOSTART, _T("Start on System Startup"));
            SetForegroundWindow(hWnd);
            TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MENU_OPEN_WEBSITE:
            OpenBrowser(WEBSITE_URL);
            break;
        case ID_MENU_AUTOSTART:
            g_autoStartChecked = !g_autoStartChecked;
            SetAutoStart(g_autoStartChecked);
            break;
        case ID_MENU_EXIT:
            PostQuitMessage(0);
            break;
        }
        break;
    case WM_DESTROY:
        CleanupMenuIcons();
        KillTimer(hWnd, TIMER_ID);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}