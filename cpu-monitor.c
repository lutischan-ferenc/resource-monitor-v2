#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <strsafe.h>
#include <pdh.h>
#include <winreg.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")

#ifndef PDH_CSTATUS_VALID_DATA
#define PDH_CSTATUS_VALID_DATA 0x0
#endif

#define WM_TRAYICON (WM_USER + 1)
#define TIMER_ID    1
#define ICON_SIZE   64
#define AUTO_START_REG_VALUE _T("CpuMonitor")
#define WEBSITE_URL _T("https://github.com/lutischan-ferenc/resource-monitor-v2")
#define MAX_CORES   64
#define TOLERANCE   0.1

// Menu item IDs
enum {
    ID_MENU_OPEN_WEBSITE = 1001,
    ID_MENU_CPU_BASE = 2000, // Base ID for CPU core menu items
    ID_MENU_AUTOSTART = 3000,
    ID_MENU_EXIT
};

// CPU information structure
typedef struct {
    double percents[MAX_CORES];
    int core_count;
} CPUINFO;

// Global variables
HINSTANCE g_hInst;
HWND g_hWnd;
HMENU g_hMenu;
NOTIFYICONDATA g_nid = {0};
HICON g_hIcon = NULL;
BOOL g_autoStartChecked = FALSE;
PDH_HQUERY g_pdhQuery = NULL;
PDH_HCOUNTER *g_pdhCounters = NULL;
int g_coreCount = 0;
double g_lastPercents[MAX_CORES] = {0};
HBITMAP hBmpCore = NULL;

// Function prototypes
void UpdateCpuInfo(CPUINFO *ci);
HICON GenerateBarIcon(CPUINFO *ci);
void OpenBrowser(LPCTSTR url);
BOOL IsAutoStartEnabled();
BOOL SetAutoStart(BOOL enable);
void UpdateTrayMenu(CPUINFO *ci);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL InitPdhCounter();
void CleanupPdhCounter();
void InitializeMenuWithIcons();
void CleanupMenuIcons();
HBITMAP CreateColorBitmap(COLORREF color, int width, int height);
BOOL CpuUsageChanged(CPUINFO *ci);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                       LPTSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;

    // Register window class
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc   = WndProc;
    wcex.hInstance     = hInstance;
    wcex.lpszClassName = _T("CpuMonitorHiddenWnd");
    RegisterClassEx(&wcex);

    // Create hidden window
    g_hWnd = CreateWindowEx(0, wcex.lpszClassName, _T("CPU Monitor"),
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);

    // Create menu
    g_hMenu = CreatePopupMenu();
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_OPEN_WEBSITE, _T("CPU Usage per Core v2.0.0"));
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);

    // Get CPU core count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    g_coreCount = sysInfo.dwNumberOfProcessors;
    if (g_coreCount > MAX_CORES) g_coreCount = MAX_CORES;

    // Initialize CPU core menu items
    for (int i = 0; i < g_coreCount; i++) {
        TCHAR buf[32];
        StringCchPrintf(buf, 32, _T("CPU %d: 0.00%%"), i + 1);
        AppendMenu(g_hMenu, MF_STRING, ID_MENU_CPU_BASE + i, buf);
    }

    // Add other menu items
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_AUTOSTART, _T("Start on System Startup"));
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_EXIT, _T("Exit"));

    // Initialize menu icons
    InitializeMenuWithIcons();

    // Initialize tray icon
    ZeroMemory(&g_nid, sizeof(NOTIFYICONDATA));
    g_nid.cbSize = sizeof(NOTIFYICONDATA);
    g_nid.hWnd = g_hWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    StringCchCopy(g_nid.szTip, sizeof(g_nid.szTip)/sizeof(TCHAR), _T("CPU Monitor"));
    g_hIcon = LoadIcon(NULL, IDI_APPLICATION);
    g_nid.hIcon = g_hIcon;
    Shell_NotifyIcon(NIM_ADD, &g_nid);

    // Check if auto-start is enabled
    g_autoStartChecked = IsAutoStartEnabled();

    // Initialize PDH counters
    if (!InitPdhCounter()) {
        PostQuitMessage(1);
        return 1;
    }

    // Set timer for CPU monitoring
    SetTimer(g_hWnd, TIMER_ID, 1000, NULL);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    CleanupMenuIcons();
    CleanupPdhCounter();
    Shell_NotifyIcon(NIM_DELETE, &g_nid);
    if (g_hIcon) DestroyIcon(g_hIcon);
    
    return (int)msg.wParam;
}

// Initialize performance data helper counters
BOOL InitPdhCounter()
{
    PDH_STATUS status;
    status = PdhOpenQuery(NULL, 0, &g_pdhQuery);
    if (status != ERROR_SUCCESS) return FALSE;

    g_pdhCounters = (PDH_HCOUNTER *)malloc(g_coreCount * sizeof(PDH_HCOUNTER));
    if (!g_pdhCounters) return FALSE;

    for (int i = 0; i < g_coreCount; i++) {
        TCHAR counterPath[128];
        StringCchPrintf(counterPath, 128, _T("\\Processor(%d)\\%% Processor Time"), i);
        status = PdhAddEnglishCounter(g_pdhQuery, counterPath, 0, &g_pdhCounters[i]);
        if (status != ERROR_SUCCESS) {
            // Clean up on failure
            while (--i >= 0) PdhRemoveCounter(g_pdhCounters[i]);
            free(g_pdhCounters);
            PdhCloseQuery(g_pdhQuery);
            return FALSE;
        }
    }
    return TRUE;
}

// Clean up PDH counters
void CleanupPdhCounter()
{
    if (g_pdhCounters) {
        for (int i = 0; i < g_coreCount; i++) {
            if (g_pdhCounters[i]) PdhRemoveCounter(g_pdhCounters[i]);
        }
        free(g_pdhCounters);
    }
    if (g_pdhQuery) PdhCloseQuery(g_pdhQuery);
}

// Create a solid color bitmap
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

// Initialize menu icons
void InitializeMenuWithIcons() {
    hBmpCore = CreateColorBitmap(RGB(100, 100, 100), 16, 16);
    
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = hBmpCore;
    
    for (int i = 0; i < g_coreCount; i++) {
        SetMenuItemInfo(g_hMenu, ID_MENU_CPU_BASE + i, FALSE, &mii);
    }
}

// Clean up menu icons
void CleanupMenuIcons() {
    if (hBmpCore) DeleteObject(hBmpCore);
}

// Update CPU usage information
void UpdateCpuInfo(CPUINFO *ci)
{
    PDH_STATUS status = PdhCollectQueryData(g_pdhQuery);
    if (status != ERROR_SUCCESS) return;

    ci->core_count = g_coreCount;
    for (int i = 0; i < g_coreCount; i++) {
        PDH_FMT_COUNTERVALUE counterValue;
        if (PdhGetFormattedCounterValue(g_pdhCounters[i], PDH_FMT_DOUBLE, NULL, &counterValue) == ERROR_SUCCESS &&
            counterValue.CStatus == PDH_CSTATUS_VALID_DATA) {
            ci->percents[i] = counterValue.doubleValue;
        } else {
            ci->percents[i] = 0.0;
        }
    }
}

// Check if CPU usage changed significantly
BOOL CpuUsageChanged(CPUINFO *ci) {
    for (int i = 0; i < ci->core_count; i++) {
        if (fabs(g_lastPercents[i] - ci->percents[i]) > TOLERANCE) {
            return TRUE;
        }
    }
    return FALSE;
}

// Generate icon showing CPU usage as bars
HICON GenerateBarIcon(CPUINFO *ci)
{
    int numCores = ci->core_count;
    int maxWidth = ICON_SIZE;
    
    // Group cores if there are too many
    int groupSize = 1;
    if (numCores > maxWidth) {
        groupSize = numCores / maxWidth;
        if (numCores % maxWidth != 0) groupSize++;
    }

    // Calculate average usage for groups
    int numGroups = (numCores + groupSize - 1) / groupSize;
    double *groupPercents = (double *)calloc(numGroups, sizeof(double));
    for (int i = 0; i < numCores; i++) {
        groupPercents[i / groupSize] += ci->percents[i];
    }
    for (int i = 0; i < numGroups; i++) {
        groupPercents[i] /= groupSize;
    }

    // Calculate bar width
    int barWidth = maxWidth / numGroups;
    if (barWidth < 1) barWidth = 1;

    // Image dimensions
    int imgWidth = barWidth * numGroups;
    int imgHeight = ICON_SIZE;

    // Create DIB section
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = imgWidth;
    bmi.bmiHeader.biHeight = -imgHeight; // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    void *pBits;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HGDIOBJ hOldBmp = SelectObject(hdcMem, hBmp);

    // Fill with transparent background
    BYTE *pPixel = (BYTE *)pBits;
    for (int y = 0; y < imgHeight; y++) {
        for (int x = 0; x < imgWidth; x++) {
            // BGRA format
            pPixel[0] = 0; // B
            pPixel[1] = 0; // G
            pPixel[2] = 0; // R
            pPixel[3] = 0; // A (transparent)
            pPixel += 4;
        }
    }

    // Create mask bitmap
    HBITMAP hMonoBmp = CreateBitmap(imgWidth, imgHeight, 1, 1, NULL);
    HDC hdcMask = CreateCompatibleDC(NULL);
    HGDIOBJ hOldMaskBmp = SelectObject(hdcMask, hMonoBmp);
    PatBlt(hdcMask, 0, 0, imgWidth, imgHeight, WHITENESS); // Start with all transparent

    // Draw bars and update mask
    for (int i = 0; i < numGroups; i++) {
        int barHeight = (int)(groupPercents[i] * imgHeight / 100.0);
        int xStart = i * barWidth;
        int xEnd = xStart + barWidth;

        // Set color pixels - Red when load is high, gray when low
        pPixel = (BYTE *)pBits + (imgHeight - barHeight) * imgWidth * 4 + xStart * 4;
        for (int y = imgHeight - barHeight; y < imgHeight; y++) {
            for (int x = xStart; x < xEnd; x++) {
                BYTE *pixel = pPixel + (x - xStart) * 4;
                pixel[0] = (barHeight > 32) ? 0 : 100;   // B
                pixel[1] = (barHeight > 32) ? 0 : 100;   // G
                pixel[2] = (barHeight > 32) ? 139 : 100; // R
                pixel[3] = 255;                          // A (opaque)
            }
            pPixel += imgWidth * 4;
        }

        // Update mask (black for opaque areas)
        RECT rect = { xStart, imgHeight - barHeight, xEnd, imgHeight };
        HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdcMask, &rect, hBlackBrush);
        DeleteObject(hBlackBrush);
    }

    // Create icon
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmMask = hMonoBmp;
    ii.hbmColor = hBmp;
    HICON hIcon = CreateIconIndirect(&ii);

    // Cleanup
    SelectObject(hdcMem, hOldBmp);
    SelectObject(hdcMask, hOldMaskBmp);
    DeleteDC(hdcMem);
    DeleteDC(hdcMask);
    ReleaseDC(NULL, hdcScreen);
    DeleteObject(hBmp);
    DeleteObject(hMonoBmp);
    free(groupPercents);

    return hIcon;
}

// Open website in default browser
void OpenBrowser(LPCTSTR url)
{
    ShellExecute(NULL, _T("open"), url, NULL, NULL, SW_SHOWNORMAL);
}

// Check if autostart is enabled in registry
BOOL IsAutoStartEnabled()
{
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    DWORD dwSize = sizeof(szValue);
    BOOL result = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                     0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, AUTO_START_REG_VALUE, NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            if (_tcslen(szValue) > 0) result = TRUE;
        }
        RegCloseKey(hKey);
    }
    return result;
}

// Enable or disable autostart in registry
BOOL SetAutoStart(BOOL enable)
{
    HKEY hKey;
    LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                             0, KEY_WRITE, &hKey);
    if (lRes != ERROR_SUCCESS) return FALSE;

    BOOL result = FALSE;
    if (enable) {
        // Add program path to registry
        TCHAR szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
            lRes = RegSetValueEx(hKey, AUTO_START_REG_VALUE, 0, REG_SZ, 
                               (const BYTE *)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
            result = (lRes == ERROR_SUCCESS);
        }
    } else {
        // Remove from registry
        lRes = RegDeleteValue(hKey, AUTO_START_REG_VALUE);
        result = (lRes == ERROR_SUCCESS) || (lRes == ERROR_FILE_NOT_FOUND);
    }
    RegCloseKey(hKey);
    return result;
}

// Update tray icon and menu with current CPU usage
void UpdateTrayMenu(CPUINFO *ci)
{
    // Update menu text for each CPU
    TCHAR buf[64];
    double avg = 0.0;
    for (int i = 0; i < ci->core_count; i++) {
        StringCchPrintf(buf, 64, _T("CPU %d: %.2f%%"), i + 1, ci->percents[i]);
        ModifyMenu(g_hMenu, ID_MENU_CPU_BASE + i, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_BASE + i, buf);
        avg += ci->percents[i];
    }
    avg /= ci->core_count;

    // Update tooltip with average CPU usage
    StringCchPrintf(g_nid.szTip, sizeof(g_nid.szTip)/sizeof(TCHAR), _T("CPU Avg: %.2f%%"), avg);
    Shell_NotifyIcon(NIM_MODIFY, &g_nid);

    // Update tray icon if usage changed significantly
    if (CpuUsageChanged(ci)) {
        // Save current values
        for (int i = 0; i < ci->core_count; i++) {
            g_lastPercents[i] = ci->percents[i];
        }
        
        // Create and set new icon
        HICON hNewIcon = GenerateBarIcon(ci);
        if (hNewIcon) {
            if (g_hIcon) DestroyIcon(g_hIcon);
            g_hIcon = hNewIcon;
            g_nid.hIcon = g_hIcon;
            Shell_NotifyIcon(NIM_MODIFY, &g_nid);
        }
    }
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static CPUINFO cpuinfo;
    
    switch (message) {
    case WM_TIMER:
        if (wParam == TIMER_ID) {
            // Update CPU usage and tray icon
            UpdateCpuInfo(&cpuinfo);
            UpdateTrayMenu(&cpuinfo);
        }
        break;
        
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            // Show context menu on right-click
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