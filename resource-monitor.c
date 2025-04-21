// Minimum Windows version: Windows Vista
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <shellapi.h>
#include <pdh.h>
#include <tchar.h>
#include <strsafe.h>
#include <winreg.h>
#include <math.h>

#pragma comment(lib, "pdh.lib")

// System tray and menu constants
#define WM_TRAYICON (WM_USER + 1)
#define ID_MENU_ABOUT 100
#define ID_MENU_USED 101
#define ID_MENU_FREE 102
#define ID_MENU_CACHED 103
#define ID_MENU_SWAP 104
#define IDD_ABOUT 105
#define IDC_WEBSITE 106
#define ID_MENU_CPU_BASE 200
#define ID_MENU_AUTOSTART 300
#define ID_MENU_LANGUAGE 301
#define ID_MENU_LANG_EN 302
#define ID_MENU_LANG_HU 303
#define ID_MENU_LANG_DE 304
#define ID_MENU_LANG_IT 305
#define ID_MENU_LANG_ES 306
#define ID_MENU_LANG_FR 307
#define ID_MENU_LANG_RU 308
#define ID_MENU_EXIT 400
#define ID_MENU_SHOW_MEMORY_ICON 500
#define ID_MENU_SHOW_CPU_ICON 501
#define MAX_CORES 64
#define ICON_SIZE 32
#define AUTO_START_REG_VALUE _T("ResourceMonitor")

// Structure to hold memory usage information
typedef struct {
    DWORDLONG totalPhys;
    DWORDLONG availPhys;
    DWORDLONG usedPhys;
    DWORDLONG cachedBytes;
    DWORDLONG totalSwap;
    DWORDLONG swapUsed;
    DWORDLONG committedBytes;
    double usedPercent;
} MEMINFO;

// Structure to hold CPU usage information
typedef struct {
    int core_count;
    double percents[MAX_CORES];
} CPUINFO;

// Structure for localized menu and tooltip strings
typedef struct {
    TCHAR menu_about[64];
    TCHAR menu_used[64];
    TCHAR menu_free[64];
    TCHAR menu_cached[64];
    TCHAR menu_swap[64];
    TCHAR cpu_core_format[64];
    TCHAR menu_autostart[64];
    TCHAR menu_language[64];
    TCHAR menu_exit[64];
    TCHAR tooltip_memory_usage[64];
    TCHAR tooltip_cpu_avg[64];
    TCHAR menu_show_memory_icon[64];
    TCHAR menu_show_cpu_icon[64];
} LANG;

// Localization data
static LANG lang_en = {
    _T("About Resource Monitor"),
    _T("Used: %u MB"),
    _T("Free: %u MB"),
    _T("Cached: %u MB"),
    _T("Swap: %u MB"),
    _T("CPU %d: %.2f%%"),
    _T("Start on System Startup"),
    _T("Language"),
    _T("Exit"),
    _T("Memory Usage: %.1f%%"),
    _T("CPU Avg: %.1f%%"),
    _T("Show Memory Icon"),
    _T("Show CPU Icon")
};

static LANG lang_hu = {
    _T("A Resource Monitor névjegye"),
    _T("Használt: %u MB"),
    _T("Szabad: %u MB"),
    _T("Gyorsítótár: %u MB"),
    _T("Csere: %u MB"),
    _T("CPU %d: %.2f%%"),
    _T("Rendszerindításkor indul"),
    _T("Nyelv"),
    _T("Kilépés"),
    _T("Memóriahasználat: %.1f%%"),
    _T("CPU Átlag: %.1f%%"),
    _T("Memória ikon megjelenítése"),
    _T("CPU ikon megjelenítése")
};

static LANG lang_de = {
    _T("Über Resource Monitor"),
    _T("Verwendet: %u MB"),
    _T("Frei: %u MB"),
    _T("Zwischengespeichert: %u MB"),
    _T("Auslagerungsdatei: %u MB"),
    _T("CPU %d: %.2f%%"),
    _T("Beim Systemstart starten"),
    _T("Sprache"),
    _T("Beenden"),
    _T("Speichernutzung: %.1f%%"),
    _T("CPU Durchschnitt: %.1f%%"),
    _T("Speicher-Symbol anzeigen"),
    _T("CPU-Symbol anzeigen")
};

static LANG lang_it = {
    _T("Info su Resource Monitor"),
    _T("Memoria Usata: %u MB"),
    _T("Memoria libera: %u MB"),
    _T("Memoria cache: %u MB"),
    _T("Memoria swap: %u MB"),
    _T("Uso CPU %d: %.2f%%"),
    _T("Esegui all'avvio del sistema"),
    _T("Lingua interfaccia"),
    _T("Esci"),
    _T("Uso memoria: %.1f%%"),
    _T("Uso medio CPU: %.1f%%"),
    _T("Visualizza icona memoria"),
    _T("Visualizza icona CPU")
};

static LANG lang_es = {
    _T("Acerca de Resource Monitor"),
    _T("Usado: %u MB"),
    _T("Libre: %u MB"),
    _T("En caché: %u MB"),
    _T("Intercambio: %u MB"),
    _T("CPU %d: %.2f%%"),
    _T("Iniciar al encender el sistema"),
    _T("Idioma"),
    _T("Salir"),
    _T("Uso de memoria: %.1f%%"),
    _T("Promedio CPU: %.1f%%"),
    _T("Mostrar icono de memoria"),
    _T("Mostrar icono de CPU")
};

static LANG lang_fr = {
    _T("À propos de Resource Monitor"),
    _T("Utilisé: %u MB"),
    _T("Libre: %u MB"),
    _T("En cache: %u MB"),
    _T("Échange: %u MB"),
    _T("CPU %d: %.2f%%"),
    _T("Démarrer au démarrage du système"),
    _T("Langue"),
    _T("Quitter"),
    _T("Utilisation de la mémoire: %.1f%%"),
    _T("Moyenne CPU: %.1f%%"),
    _T("Afficher l'icône de mémoire"),
    _T("Afficher l'icône de CPU")
};

static LANG lang_ru = {
    _T("О Resource Monitor"),
    _T("Использовано: %u MB"),
    _T("Свободно: %u MB"),
    _T("Кэшировано: %u MB"),
    _T("Файл подкачки: %u MB"),
    _T("CPU %d: %.2f%%"),
    _T("Запуск при старте системы"),
    _T("Язык"),
    _T("Выход"),
    _T("Использование памяти: %.1f%%"),
    _T("Средняя загрузка CPU: %.1f%%"),
    _T("Показать значок памяти"),
    _T("Показать значок CPU")
};

// Function prototypes
void UpdateMemoryInfo(MEMINFO *mi);
HICON GeneratePieIcon(MEMINFO *mi);
void UpdateTrayMemory(MEMINFO *mi);
void UpdateCpuInfo(CPUINFO *ci);
BOOL CpuUsageChanged(CPUINFO *ci);
HICON GenerateBarIcon(CPUINFO *ci);
void UpdateTrayCpu(CPUINFO *ci);
void SetLanguage(LANG *newLang);
void InitializeMenuWithIcons(void);
void CleanupMenuIcons(void);
HBITMAP CreateColorBitmap(COLORREF color, int width, int height);
void SaveLanguageSelectionToRegistry(void);
void LoadLanguageSelectionFromRegistry(void);
void RefreshMenuText(void);
BOOL IsAutoStartEnabled(void);
void SetAutoStart(BOOL enable);
void UpdateIconVisibility(void);
void SaveMenuStateToRegistry(void);
void LoadMenuStateFromRegistry(void);
INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowAboutDialog(HWND hwndParent);

// Global variables
static LANG *g_lang = &lang_en;
static HINSTANCE g_hInst;
static HWND g_hWnd;
static HMENU g_hMenu;
static HMENU g_hLangMenu;
static NOTIFYICONDATA g_nidMemory = {0};
static NOTIFYICONDATA g_nidCpu = {0};
static HBITMAP g_hBmpUsed;
static HBITMAP g_hBmpFree;
static HICON g_hIconMemory;
static HICON g_hIconCpu;
static BOOL g_autoStartChecked;
static DWORDLONG g_lastUsedMB;
static double g_lastPercents[MAX_CORES];
static PDH_HQUERY g_pdhQueryMemory;
static PDH_HCOUNTER g_pdhCounterMemory;
static PDH_HCOUNTER g_pdhCounterModifiedPageListBytes;
static PDH_HCOUNTER g_pdhCounterStandbyCoreBytes;
static PDH_HCOUNTER g_pdhCounterStandbyNormalBytes;
static PDH_HCOUNTER g_pdhCounterStandbyReserveBytes;
static PDH_HCOUNTER g_pdhCounterCommittedBytes;
static PDH_HQUERY g_pdhQueryCpu;
static PDH_HCOUNTER *g_pdhCountersCpu;
static int g_coreCount;
static BOOL g_showMemoryIcon = TRUE;
static BOOL g_showCpuIcon = TRUE;
static int screenWidth = 0, screenHeight = 0;

// Updates memory usage information using system and PDH counters
void UpdateMemoryInfo(MEMINFO *mi) {
    MEMORYSTATUSEX ms = { sizeof(ms) };
    GlobalMemoryStatusEx(&ms);
    mi->totalPhys = ms.ullTotalPhys;
    mi->availPhys = ms.ullAvailPhys;
    mi->usedPhys = mi->totalPhys - mi->availPhys;
    mi->usedPercent = (double)mi->usedPhys / mi->totalPhys * 100.0;
    mi->totalSwap = ms.ullTotalPageFile;

    if (!g_pdhQueryMemory) return;

    if (PdhCollectQueryData(g_pdhQueryMemory) == ERROR_SUCCESS) {
        PDH_FMT_COUNTERVALUE cv;
        DWORDLONG totalCached = 0;

        if (PdhGetFormattedCounterValue(g_pdhCounterMemory, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS) {
            totalCached += cv.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhCounterModifiedPageListBytes, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS) {
            totalCached += cv.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhCounterStandbyCoreBytes, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS) {
            totalCached += cv.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhCounterStandbyNormalBytes, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS) {
            totalCached += cv.largeValue;
        }
        if (PdhGetFormattedCounterValue(g_pdhCounterStandbyReserveBytes, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS) {
            totalCached += cv.largeValue;
        }
        mi->cachedBytes = totalCached;

        mi->committedBytes = (PdhGetFormattedCounterValue(g_pdhCounterCommittedBytes, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS)
            ? cv.largeValue : 0;
    }

    mi->swapUsed = (mi->committedBytes > mi->usedPhys) ? (mi->committedBytes - mi->usedPhys) : 0;
}

// Generates a pie chart icon representing memory usage
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

// Initializes menu icons for memory usage indicators
void InitializeMenuWithIcons(void) {
    g_hBmpUsed = CreateColorBitmap(RGB(200, 200, 200), 16, 16);
    g_hBmpFree = CreateColorBitmap(RGB(100, 100, 100), 16, 16);

    MENUITEMINFO mii = { sizeof(MENUITEMINFO), MIIM_BITMAP };
    mii.hbmpItem = g_hBmpUsed;
    SetMenuItemInfo(g_hMenu, ID_MENU_USED, FALSE, &mii);
    mii.hbmpItem = g_hBmpFree;
    SetMenuItemInfo(g_hMenu, ID_MENU_FREE, FALSE, &mii);
}

// Cleans up menu icons
void CleanupMenuIcons(void) {
    if (g_hBmpUsed) DeleteObject(g_hBmpUsed);
    if (g_hBmpFree) DeleteObject(g_hBmpFree);
    g_hBmpUsed = NULL;
    g_hBmpFree = NULL;
}

// Creates a solid color bitmap for menu icons
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

// Updates the system tray memory icon and menu
// Updates the system tray memory icon and menu
void UpdateTrayMemory(MEMINFO *mi) {
    TCHAR buf[64];
    DWORD usedMB = (DWORD)(mi->usedPhys / (1024 * 1024));
    DWORD freeMB = (DWORD)(mi->availPhys / (1024 * 1024));
    DWORD cachedMB = (DWORD)(mi->cachedBytes / (1024 * 1024));
    DWORD swapMB = (DWORD)(mi->swapUsed / (1024 * 1024));

    StringCchPrintf(buf, 64, g_lang->menu_used, usedMB);
    ModifyMenu(g_hMenu, ID_MENU_USED, MF_BYCOMMAND | MF_STRING, ID_MENU_USED, buf);
    StringCchPrintf(buf, 64, g_lang->menu_free, freeMB);
    ModifyMenu(g_hMenu, ID_MENU_FREE, MF_BYCOMMAND | MF_STRING, ID_MENU_FREE, buf);
    StringCchPrintf(buf, 64, g_lang->menu_cached, cachedMB);
    ModifyMenu(g_hMenu, ID_MENU_CACHED, MF_BYCOMMAND | MF_STRING, ID_MENU_CACHED, buf);
    StringCchPrintf(buf, 64, g_lang->menu_swap, swapMB);
    ModifyMenu(g_hMenu, ID_MENU_SWAP, MF_BYCOMMAND | MF_STRING, ID_MENU_SWAP, buf);

    // Reapply bitmap icons for Used and Free menu items
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = g_hBmpUsed;
    SetMenuItemInfo(g_hMenu, ID_MENU_USED, FALSE, &mii);
    mii.hbmpItem = g_hBmpFree;
    SetMenuItemInfo(g_hMenu, ID_MENU_FREE, FALSE, &mii);

    StringCchPrintf(g_nidMemory.szTip, sizeof(g_nidMemory.szTip)/sizeof(TCHAR), g_lang->tooltip_memory_usage, mi->usedPercent);
    Shell_NotifyIcon(NIM_MODIFY, &g_nidMemory);

    if (abs((int)usedMB - (int)g_lastUsedMB) > 100) {
        g_lastUsedMB = usedMB;
        HICON hNewIcon = GeneratePieIcon(mi);
        if (hNewIcon) {
            if (g_hIconMemory) DestroyIcon(g_hIconMemory);
            g_hIconMemory = hNewIcon;
            g_nidMemory.hIcon = g_hIconMemory;
            Shell_NotifyIcon(NIM_MODIFY, &g_nidMemory);
        }
    }
}

// Updates CPU usage information
void UpdateCpuInfo(CPUINFO *ci) {
    ci->core_count = g_coreCount;
    if (!g_pdhQueryCpu || !g_pdhCountersCpu) return;

    PdhCollectQueryData(g_pdhQueryCpu);
    Sleep(100);

    for (int i = 0; i < g_coreCount; i++) {
        PDH_FMT_COUNTERVALUE cv;
        if (PdhGetFormattedCounterValue(g_pdhCountersCpu[i], PDH_FMT_DOUBLE, NULL, &cv) == ERROR_SUCCESS) {
            ci->percents[i] = max(0.0, min(100.0, cv.doubleValue));
        }
    }
}

// Checks if CPU usage has changed significantly
BOOL CpuUsageChanged(CPUINFO *ci) {
    for (int i = 0; i < ci->core_count; i++) {
        if (fabs(ci->percents[i] - g_lastPercents[i]) > 5.0) return TRUE;
    }
    return FALSE;
}

// Generates a bar chart icon for CPU usage
HICON GenerateBarIcon(CPUINFO *ci) {
    int numCores = ci->core_count;
    int maxWidth = ICON_SIZE;
    int groupSize = numCores > maxWidth ? (numCores + maxWidth - 1) / maxWidth : 1;
    int numGroups = (numCores + groupSize - 1) / groupSize;

    double *groupPercents = (double *)calloc(numGroups, sizeof(double));
    for (int i = 0; i < numCores; i++) {
        groupPercents[i / groupSize] += ci->percents[i];
    }
    for (int i = 0; i < numGroups; i++) {
        groupPercents[i] /= groupSize;
    }

    int barWidth = maxWidth / numGroups;
    if (barWidth < 1) barWidth = 1;
    int imgWidth = barWidth * numGroups;
    int imgHeight = ICON_SIZE;

    BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), imgWidth, -imgHeight, 1, 32, BI_RGB };
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    void *pBits;
    HBITMAP hBmp = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HGDIOBJ hOldBmp = SelectObject(hdcMem, hBmp);

    BYTE *pPixel = (BYTE *)pBits;
    for (int y = 0; y < imgHeight; y++) {
        for (int x = 0; x < imgWidth; x++) {
            pPixel[0] = pPixel[1] = pPixel[2] = pPixel[3] = 0;
            pPixel += 4;
        }
    }

    HBITMAP hMonoBmp = CreateBitmap(imgWidth, imgHeight, 1, 1, NULL);
    HDC hdcMask = CreateCompatibleDC(NULL);
    HGDIOBJ hOldMaskBmp = SelectObject(hdcMask, hMonoBmp);
    PatBlt(hdcMask, 0, 0, imgWidth, imgHeight, WHITENESS);

    for (int i = 0; i < numGroups; i++) {
        int barHeight = (int)(groupPercents[i] * imgHeight / 100.0);
        int xStart = i * barWidth;
        int xEnd = xStart + barWidth;

        pPixel = (BYTE *)pBits + (imgHeight - barHeight) * imgWidth * 4 + xStart * 4;
        for (int y = imgHeight - barHeight; y < imgHeight; y++) {
            for (int x = xStart; x < xEnd; x++) {
                BYTE *pixel = pPixel + (x - xStart) * 4;
                pixel[0] = (barHeight > 16) ? 0 : 100;
                pixel[1] = (barHeight > 16) ? 0 : 100;
                pixel[2] = (barHeight > 16) ? 139 : 100;
                pixel[3] = 255;
            }
            pPixel += imgWidth * 4;
        }

        RECT rect = { xStart, imgHeight - barHeight, xEnd, imgHeight };
        HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdcMask, &rect, hBlackBrush);
        DeleteObject(hBlackBrush);
    }

    ICONINFO ii = { TRUE, 0, 0, hMonoBmp, hBmp };
    HICON hIcon = CreateIconIndirect(&ii);

    SelectObject(hdcMem, hOldBmp);
    SelectObject(hdcMask, hOldMaskBmp);
    DeleteDC(hdcMem);
    DeleteDC(hdcMask);
    DeleteObject(hBmp);
    DeleteObject(hMonoBmp);
    ReleaseDC(NULL, hdcScreen);
    free(groupPercents);

    return hIcon;
}

// Updates the system tray CPU icon and menu
void UpdateTrayCpu(CPUINFO *ci) {
    TCHAR buf[64];
    double avg = 0.0;
    for (int i = 0; i < ci->core_count; i++) {
        StringCchPrintf(buf, 64, g_lang->cpu_core_format, i + 1, ci->percents[i]);
        ModifyMenu(g_hMenu, ID_MENU_CPU_BASE + i, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_BASE + i, buf);
        avg += ci->percents[i];
    }
    avg /= ci->core_count ? ci->core_count : 1;

    StringCchPrintf(g_nidCpu.szTip, sizeof(g_nidCpu.szTip)/sizeof(TCHAR), g_lang->tooltip_cpu_avg, avg);
    Shell_NotifyIcon(NIM_MODIFY, &g_nidCpu);

    if (CpuUsageChanged(ci)) {
        for (int i = 0; i < ci->core_count; i++) g_lastPercents[i] = ci->percents[i];
        HICON hNewIcon = GenerateBarIcon(ci);
        if (hNewIcon) {
            if (g_hIconCpu) DestroyIcon(g_hIconCpu);
            g_hIconCpu = hNewIcon;
            g_nidCpu.hIcon = g_hIconCpu;
            Shell_NotifyIcon(NIM_MODIFY, &g_nidCpu);
        }
    }
}

// Sets the application language and refreshes the UI
void SetLanguage(LANG *newLang) {
    g_lang = newLang;
    RefreshMenuText();
}

// Saves the selected language to the registry
void SaveLanguageSelectionToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD langId = g_lang == &lang_en ? ID_MENU_LANG_EN :
                       g_lang == &lang_hu ? ID_MENU_LANG_HU :
                       g_lang == &lang_de ? ID_MENU_LANG_DE :
                       g_lang == &lang_it ? ID_MENU_LANG_IT :
                       g_lang == &lang_es ? ID_MENU_LANG_ES :
                       g_lang == &lang_fr ? ID_MENU_LANG_FR : ID_MENU_LANG_RU;
        RegSetValueEx(hKey, _T("Language"), 0, REG_DWORD, (const BYTE *)&langId, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// Loads the selected language from the registry
void LoadLanguageSelectionFromRegistry(void) {
    HKEY hKey;
    g_lang = &lang_en;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD langId, dataSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, _T("Language"), NULL, NULL, (LPBYTE)&langId, &dataSize) == ERROR_SUCCESS) {
            switch (langId) {
                case ID_MENU_LANG_HU: g_lang = &lang_hu; break;
                case ID_MENU_LANG_DE: g_lang = &lang_de; break;
                case ID_MENU_LANG_IT: g_lang = &lang_it; break;
                case ID_MENU_LANG_ES: g_lang = &lang_es; break;
                case ID_MENU_LANG_FR: g_lang = &lang_fr; break;
                case ID_MENU_LANG_RU: g_lang = &lang_ru; break;
                default: g_lang = &lang_en; break;
            }
        }
        RegCloseKey(hKey);
    }
}

// Refreshes menu text with current language
void RefreshMenuText(void) {
    ModifyMenu(g_hMenu, ID_MENU_ABOUT, MF_BYCOMMAND | MF_STRING, ID_MENU_ABOUT, g_lang->menu_about);
    ModifyMenu(g_hMenu, ID_MENU_AUTOSTART, MF_BYCOMMAND | MF_STRING | (g_autoStartChecked ? MF_CHECKED : 0),
        ID_MENU_AUTOSTART, g_lang->menu_autostart);
    ModifyMenu(g_hMenu, ID_MENU_SHOW_MEMORY_ICON, MF_BYCOMMAND | MF_STRING | (g_showMemoryIcon ? MF_CHECKED : 0),
        ID_MENU_SHOW_MEMORY_ICON, g_lang->menu_show_memory_icon);
    ModifyMenu(g_hMenu, ID_MENU_SHOW_CPU_ICON, MF_BYCOMMAND | MF_STRING | (g_showCpuIcon ? MF_CHECKED : 0),
        ID_MENU_SHOW_CPU_ICON, g_lang->menu_show_cpu_icon);
    ModifyMenu(g_hMenu, ID_MENU_EXIT, MF_BYCOMMAND | MF_STRING, ID_MENU_EXIT, g_lang->menu_exit);
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_EN, MF_BYCOMMAND | (g_lang == &lang_en ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_DE, MF_BYCOMMAND | (g_lang == &lang_de ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_IT, MF_BYCOMMAND | (g_lang == &lang_it ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_ES, MF_BYCOMMAND | (g_lang == &lang_es ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_FR, MF_BYCOMMAND | (g_lang == &lang_fr ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_HU, MF_BYCOMMAND | (g_lang == &lang_hu ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hLangMenu, ID_MENU_LANG_RU, MF_BYCOMMAND | (g_lang == &lang_ru ? MF_CHECKED : MF_UNCHECKED));
    
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_STRING | MIIM_SUBMENU;
    mii.dwTypeData = g_lang->menu_language;
    mii.hSubMenu = g_hLangMenu;
    SetMenuItemInfo(g_hMenu, (UINT_PTR)g_hLangMenu, FALSE, &mii);
}

// Checks if autostart is enabled
BOOL IsAutoStartEnabled(void) {
    HKEY hKey;
    TCHAR szValue[MAX_PATH];
    DWORD dwSize = sizeof(szValue);
    BOOL result = FALSE;

    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, AUTO_START_REG_VALUE, NULL, NULL, (LPBYTE)szValue, &dwSize) == ERROR_SUCCESS) {
            result = _tcslen(szValue) > 0;
        }
        RegCloseKey(hKey);
    }
    return result;
}

// Enables or disables autostart
void SetAutoStart(BOOL enable) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return;
    }

    if (enable) {
        TCHAR szPath[MAX_PATH];
        if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
            RegSetValueEx(hKey, AUTO_START_REG_VALUE, 0, REG_SZ, (const BYTE *)szPath, (lstrlen(szPath) + 1) * sizeof(TCHAR));
        }
    } else {
        RegDeleteValue(hKey, AUTO_START_REG_VALUE);
    }
    RegCloseKey(hKey);
}

// Updates visibility of system tray icons
void UpdateIconVisibility(void) {
    Shell_NotifyIcon(g_showMemoryIcon ? NIM_ADD : NIM_DELETE, &g_nidMemory);
    Shell_NotifyIcon(g_showCpuIcon ? NIM_ADD : NIM_DELETE, &g_nidCpu);
}

// Saves icon visibility settings to the registry
void SaveMenuStateToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD showMemoryIcon = g_showMemoryIcon;
        DWORD showCpuIcon = g_showCpuIcon;
        RegSetValueEx(hKey, _T("ShowMemoryIcon"), 0, REG_DWORD, (const BYTE *)&showMemoryIcon, sizeof(DWORD));
        RegSetValueEx(hKey, _T("ShowCpuIcon"), 0, REG_DWORD, (const BYTE *)&showCpuIcon, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// Loads icon visibility settings from the registry
void LoadMenuStateFromRegistry(void) {
    HKEY hKey;
    g_showMemoryIcon = g_showCpuIcon = TRUE;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD showMemoryIcon = 1, showCpuIcon = 1, dataSize = sizeof(DWORD);
        RegQueryValueEx(hKey, _T("ShowMemoryIcon"), NULL, NULL, (LPBYTE)&showMemoryIcon, &dataSize);
        RegQueryValueEx(hKey, _T("ShowCpuIcon"), NULL, NULL, (LPBYTE)&showCpuIcon, &dataSize);
        g_showMemoryIcon = showMemoryIcon != 0;
        g_showCpuIcon = showCpuIcon != 0;
        RegCloseKey(hKey);
    }
}

// Show about dialog
void ShowAboutDialog(HWND hwndParent) {
    DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hwndParent, AboutDlgProc);
}

// About dialog procedure
INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HFONT hLinkFont = NULL;
    static HBRUSH hBrush = NULL;

    switch (uMsg) {
        case WM_INITDIALOG: {
            // Create underlined font for link
            HFONT hFont = (HFONT)SendDlgItemMessageW(hwndDlg, IDC_WEBSITE, WM_GETFONT, 0, 0);
            LOGFONTW lf;
            GetObjectW(hFont, sizeof(LOGFONTW), &lf);
            lf.lfUnderline = TRUE;
            hLinkFont = CreateFontIndirectW(&lf);
            SendDlgItemMessageW(hwndDlg, IDC_WEBSITE, WM_SETFONT, (WPARAM)hLinkFont, TRUE);

            // Set link text
            SetDlgItemTextW(hwndDlg, IDC_WEBSITE, L"Visit our website");

            // Create brush for WM_CTLCOLORSTATIC
            hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

            // Center dialog on screen
            RECT rect;
            GetWindowRect(hwndDlg, &rect);
            int dlgWidth = rect.right - rect.left;
            int dlgHeight = rect.bottom - rect.top;
            int xPos = (screenWidth - dlgWidth) / 2;
            int yPos = (screenHeight - dlgHeight) / 2;
            SetWindowPos(hwndDlg, NULL, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            return TRUE;
        }
        case WM_CTLCOLORSTATIC: {
            HWND hwndStatic = (HWND)lParam;
            if (GetDlgCtrlID(hwndStatic) == IDC_WEBSITE) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(0, 0, 255));
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)hBrush;
            }
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_WEBSITE && HIWORD(wParam) == STN_CLICKED) {
                ShellExecuteW(NULL, L"open", L"https://github.com/lutischan-ferenc/resource-monitor-v2", NULL, NULL, SW_SHOWNORMAL);
                return TRUE;
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                // Clean up resources and close dialog
                if (hLinkFont) {
                    DeleteObject(hLinkFont);
                    hLinkFont = NULL;
                }
                if (hBrush) {
                    DeleteObject(hBrush);
                    hBrush = NULL;
                }
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
        case WM_DESTROY:
        case WM_CLOSE:
            // Clean up resources
            if (hLinkFont) {
                DeleteObject(hLinkFont);
                hLinkFont = NULL;
            }
            if (hBrush) {
                DeleteObject(hBrush);
                hBrush = NULL;
            }
            
            if (uMsg == WM_CLOSE)
                EndDialog(hwndDlg, IDCANCEL);
                
            return TRUE;
    }
    return FALSE;
}

// Window procedure for handling system tray and menu events
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static MEMINFO mi;
    static CPUINFO ci;

    switch (msg) {
    case WM_CREATE:
        SetTimer(hWnd, 1, 1000, NULL);
        break;

    case WM_TIMER:
        UpdateMemoryInfo(&mi);
        UpdateTrayMemory(&mi);
        UpdateCpuInfo(&ci);
        UpdateTrayCpu(&ci);
        break;

    case WM_TRAYICON:
        if ((wParam == 1 || wParam == 2) && lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MENU_ABOUT:
            ShowAboutDialog(hWnd);
            break;
        case ID_MENU_LANG_EN:
            SetLanguage(&lang_en);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_LANG_HU:
            SetLanguage(&lang_hu);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_LANG_DE:
            SetLanguage(&lang_de);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_LANG_IT:
            SetLanguage(&lang_it);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_LANG_ES:
            SetLanguage(&lang_es);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_LANG_FR:
            SetLanguage(&lang_fr);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_LANG_RU:
            SetLanguage(&lang_ru);
            SaveLanguageSelectionToRegistry();
            break;
        case ID_MENU_AUTOSTART:
            g_autoStartChecked = !g_autoStartChecked;
            SetAutoStart(g_autoStartChecked);
            CheckMenuItem(g_hMenu, ID_MENU_AUTOSTART, MF_BYCOMMAND | (g_autoStartChecked ? MF_CHECKED : MF_UNCHECKED));
            break;
        case ID_MENU_EXIT:
            PostQuitMessage(0);
            break;
        case ID_MENU_SHOW_MEMORY_ICON:
            if (g_showMemoryIcon && !g_showCpuIcon) break;
            g_showMemoryIcon = !g_showMemoryIcon;
            CheckMenuItem(g_hMenu, ID_MENU_SHOW_MEMORY_ICON, MF_BYCOMMAND | (g_showMemoryIcon ? MF_CHECKED : MF_UNCHECKED));
            UpdateIconVisibility();
            SaveMenuStateToRegistry();
            break;
        case ID_MENU_SHOW_CPU_ICON:
            if (g_showCpuIcon && !g_showMemoryIcon) break;
            g_showCpuIcon = !g_showCpuIcon;
            CheckMenuItem(g_hMenu, ID_MENU_SHOW_CPU_ICON, MF_BYCOMMAND | (g_showCpuIcon ? MF_CHECKED : MF_UNCHECKED));
            UpdateIconVisibility();
            SaveMenuStateToRegistry();
            break;
        }
        break;

    case WM_DESTROY:
        CleanupMenuIcons();
        Shell_NotifyIcon(NIM_DELETE, &g_nidMemory);
        Shell_NotifyIcon(NIM_DELETE, &g_nidCpu);
        if (g_hIconMemory) DestroyIcon(g_hIconMemory);
        if (g_hIconCpu) DestroyIcon(g_hIconCpu);
        if (g_hMenu) DestroyMenu(g_hMenu);
        if (g_hLangMenu) DestroyMenu(g_hLangMenu);
        if (g_pdhQueryMemory) PdhCloseQuery(g_pdhQueryMemory);
        if (g_pdhQueryCpu) PdhCloseQuery(g_pdhQueryCpu);
        if (g_pdhCountersCpu) free(g_pdhCountersCpu);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Application entry point
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Initialize window class
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, NULL, NULL,
                        (HBRUSH)(COLOR_WINDOW+1), NULL, _T("TrayAppClass"), NULL };
    RegisterClassEx(&wcex);
    g_hWnd = CreateWindow(_T("TrayAppClass"), _T("TrayApp"), WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    ShowWindow(g_hWnd, SW_HIDE);
    UpdateWindow(g_hWnd);

    // Get CPU core count
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    g_coreCount = min(sysInfo.dwNumberOfProcessors, MAX_CORES);

    // Initialize system tray menu
    g_hMenu = CreatePopupMenu();
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_ABOUT, g_lang->menu_about);
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_USED, g_lang->menu_used);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_FREE, g_lang->menu_free);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_CACHED, g_lang->menu_cached);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_SWAP, g_lang->menu_swap);
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    for (int i = 0; i < g_coreCount; i++) {
        TCHAR buf[32];
        StringCchPrintf(buf, 32, g_lang->cpu_core_format, i + 1, 0.00);
        AppendMenu(g_hMenu, MF_STRING, ID_MENU_CPU_BASE + i, buf);
    }
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING | MF_CHECKED, ID_MENU_AUTOSTART, g_lang->menu_autostart);
    AppendMenu(g_hMenu, MF_STRING | MF_CHECKED, ID_MENU_SHOW_MEMORY_ICON, g_lang->menu_show_memory_icon);
    AppendMenu(g_hMenu, MF_STRING | MF_CHECKED, ID_MENU_SHOW_CPU_ICON, g_lang->menu_show_cpu_icon);
    g_hLangMenu = CreatePopupMenu();
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_EN, _T("English"));
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_DE, _T("Deutsch"));
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_IT, _T("Italiano"));
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_ES, _T("Español"));
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_FR, _T("Français"));
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_HU, _T("Magyar"));
    AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_RU, _T("Русский"));
    AppendMenu(g_hMenu, MF_POPUP, (UINT_PTR)g_hLangMenu, g_lang->menu_language);
    AppendMenu(g_hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_EXIT, g_lang->menu_exit);

    InitializeMenuWithIcons();

    // Initialize system tray icons
    g_nidMemory.cbSize = sizeof(NOTIFYICONDATA);
    g_nidMemory.hWnd = g_hWnd;
    g_nidMemory.uID = 1;
    g_nidMemory.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nidMemory.uCallbackMessage = WM_TRAYICON;
    StringCchCopy(g_nidMemory.szTip, sizeof(g_nidMemory.szTip)/sizeof(TCHAR), g_lang->tooltip_memory_usage);
    g_hIconMemory = LoadIcon(NULL, IDI_APPLICATION);
    g_nidMemory.hIcon = g_hIconMemory;
    Shell_NotifyIcon(NIM_ADD, &g_nidMemory);

    g_nidCpu.cbSize = sizeof(NOTIFYICONDATA);
    g_nidCpu.hWnd = g_hWnd;
    g_nidCpu.uID = 2;
    g_nidCpu.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_nidCpu.uCallbackMessage = WM_TRAYICON;
    StringCchCopy(g_nidCpu.szTip, sizeof(g_nidCpu.szTip)/sizeof(TCHAR), g_lang->tooltip_cpu_avg);
    g_hIconCpu = LoadIcon(NULL, IDI_APPLICATION);
    g_nidCpu.hIcon = g_hIconCpu;
    Shell_NotifyIcon(NIM_ADD, &g_nidCpu);

    // Initialize PDH counters for memory
    PdhOpenQuery(NULL, 0, &g_pdhQueryMemory);
    PdhAddEnglishCounter(g_pdhQueryMemory, _T("\\Memory\\Cache Bytes"), 0, &g_pdhCounterMemory);
    PdhAddEnglishCounter(g_pdhQueryMemory, _T("\\Memory\\Modified Page List Bytes"), 0, &g_pdhCounterModifiedPageListBytes);
    PdhAddEnglishCounter(g_pdhQueryMemory, _T("\\Memory\\Standby Cache Core Bytes"), 0, &g_pdhCounterStandbyCoreBytes);
    PdhAddEnglishCounter(g_pdhQueryMemory, _T("\\Memory\\Standby Cache Normal Priority Bytes"), 0, &g_pdhCounterStandbyNormalBytes);
    PdhAddEnglishCounter(g_pdhQueryMemory, _T("\\Memory\\Standby Cache Reserve Bytes"), 0, &g_pdhCounterStandbyReserveBytes);
    PdhAddEnglishCounter(g_pdhQueryMemory, _T("\\Memory\\Committed Bytes"), 0, &g_pdhCounterCommittedBytes);
    for (int i = 0; i < 3; i++) {
        PdhCollectQueryData(g_pdhQueryMemory);
        Sleep(100);
    }

    // Initialize PDH counters for CPU
    PdhOpenQuery(NULL, 0, &g_pdhQueryCpu);
    g_pdhCountersCpu = (PDH_HCOUNTER*)malloc(g_coreCount * sizeof(PDH_HCOUNTER));
    for (int i = 0; i < g_coreCount; i++) {
        TCHAR path[128];
        StringCchPrintf(path, 128, _T("\\Processor(%d)\\%% Processor Time"), i);
        PdhAddEnglishCounter(g_pdhQueryCpu, path, 0, &g_pdhCountersCpu[i]);
    }
    for (int i = 0; i < 3; i++) {
        PdhCollectQueryData(g_pdhQueryCpu);
        Sleep(100);
    }

    // Load settings
    g_autoStartChecked = IsAutoStartEnabled();
    if (!g_autoStartChecked) {
        g_autoStartChecked = TRUE;
        SetAutoStart(TRUE);
    }
    LoadLanguageSelectionFromRegistry();
    LoadMenuStateFromRegistry();
    RefreshMenuText();
    UpdateIconVisibility();

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}