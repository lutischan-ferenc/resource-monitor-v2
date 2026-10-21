// Minimum Windows version: Windows Vista
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <shellapi.h>
#include <pdh.h>
#include <tchar.h>
#include <strsafe.h>
#include <winreg.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <commctrl.h>

#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "comctl32.lib")

// System tray and menu constants
#define WM_TRAYICON (WM_USER + 1)
#define ID_MENU_ABOUT 100
#define ID_MENU_USED 101
#define ID_MENU_FREE 102
#define ID_MENU_CACHED 103
#define ID_MENU_SWAP 104
#define IDD_ABOUT 105
#define IDC_WEBSITE 106
#define IDD_REFRESH_INTERVAL 107
#define IDC_COFFEE 108
#define IDC_SLIDER_REFRESH_INTERVAL 1002
#define IDC_STATIC_REFRESH_VALUE 1003
#define IDC_STATIC_REFRESH_LABEL 1004
#define ID_MENU_CPU_BASE 3000
#define ID_MENU_AUTOSTART 300
#define ID_MENU_LANGUAGE 301
#define ID_MENU_LANG_EN 302
#define ID_MENU_LANG_HU 303
#define ID_MENU_LANG_DE 304
#define ID_MENU_LANG_IT 305
#define ID_MENU_LANG_ES 306
#define ID_MENU_LANG_FR 307
#define ID_MENU_LANG_RU 308
#define ID_MENU_REFRESH_INTERVAL 309
#define ID_MENU_EXIT 400
#define ID_MENU_SHOW_MEMORY_ICON 500
#define ID_MENU_SHOW_CPU_ICON 501
#define ID_MENU_CPU_DISPLAY 600
#define ID_MENU_CPU_SHOW_ALL 601
#define ID_MENU_CPU_SHOW_AVG 602
#define ID_MENU_CPU_SHOW_MAX 603

#define CPU_DISPLAY_MODE_ALL 0
#define CPU_DISPLAY_MODE_AVG 1
#define CPU_DISPLAY_MODE_MAX 2
#define MAX_CORES 256
#define ICON_SIZE 32
#define CACHE_COUNTERS 5
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
    double avg, max;                // stats of percents[], computed once in UpdateCpuInfo
    int max_core;
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
    TCHAR menu_refresh_interval[64];
    TCHAR dialog_refresh_title[64];
    TCHAR dialog_refresh_label[64];
    TCHAR menu_cpu_display[64];
    TCHAR menu_cpu_show_all[64];
    TCHAR menu_cpu_show_avg[64];
    TCHAR menu_cpu_show_max[64];
    TCHAR cpu_avg_format[64];
    TCHAR cpu_max_format[64];
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
    _T("Show CPU Icon"),
    _T("Refresh Interval..."),
    _T("Refresh Interval"),
    _T("Refresh interval:"),
    _T("CPU Display"),
    _T("Show All Cores"),
    _T("Show Average"),
    _T("Show Maximum"),
    _T("CPU Avg: %.1f%%"),
    _T("CPU Max: %.1f%% (Core %d)"),
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
    _T("CPU ikon megjelenítése"),
    _T("Frissítési idő..."),
    _T("Frissítési idő"),
    _T("Frissítési intervallum:"),
    _T("CPU Megjelenítés"),
    _T("Összes mag mutatása"),
    _T("Átlag mutatása"),
    _T("Maximum mutatása"),
    _T("CPU Átlag: %.1f%%"),
    _T("CPU Max: %.1f%% (%d. mag)"),
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
    _T("CPU-Symbol anzeigen"),
    _T("Aktualisierungsintervall..."),
    _T("Aktualisierungsintervall"),
    _T("Aktualisierungsintervall:"),
    _T("CPU-Anzeige"),
    _T("Alle Kerne anzeigen"),
    _T("Durchschnitt anzeigen"),
    _T("Maximum anzeigen"),
    _T("CPU Durchschnitt: %.1f%%"),
    _T("CPU Max: %.1f%% (Kern %d)"),
};

static LANG lang_it = {
    _T("Info su Resource Monitor"),
    _T("Memoria usata: %u MB"),
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
    _T("Visualizza icona CPU"),
    _T("Intervallo aggiornamento..."),
    _T("Intervallo aggiornamento"),
    _T("Intervallo aggiornamento:"),
    _T("Visualizzazione CPU"),
    _T("Visualizza tutti i core"),
    _T("Visualizza media"),
    _T("Visualizza massimo"),
    _T("Media CPU: %.1f%%"),
    _T("Max CPU: %.1f%% (Core %d)"),
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
    _T("Mostrar icono de CPU"),
    _T("Intervalo de actualización..."),
    _T("Intervalo de actualización"),
    _T("Intervalo de actualización:"),
    _T("Visualización de CPU"),
    _T("Mostrar todos los núcleos"),
    _T("Mostrar promedio"),
    _T("Mostrar máximo"),
    _T("Promedio CPU: %.1f%%"),
    _T("Máx CPU: %.1f%% (Núcleo %d)"),
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
    _T("Afficher l'icône de CPU"),
    _T("Intervalle de rafraîchissement..."),
    _T("Intervalle de rafraîchissement"),
    _T("Intervalle de rafraîchissement:"),
    _T("Affichage CPU"),
    _T("Afficher tous les cœurs"),
    _T("Afficher la moyenne"),
    _T("Afficher le maximum"),
    _T("Moyenne CPU: %.1f%%"),
    _T("Max CPU: %.1f%% (Cœur %d)"),
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
    _T("Показать значок CPU"),
    _T("Интервал обновления..."),
    _T("Интервал обновления"),
    _T("Интервал обновления:"),
    _T("Отображение CPU"),
    _T("Показать все ядра"),
    _T("Показать среднее"),
    _T("Показать максимум"),
    _T("Средняя CPU: %.1f%%"),
    _T("Макс CPU: %.1f%% (Ядро %d)"),
};

// Function prototypes
void UpdateMemoryInfo(MEMINFO *mi);
HICON GeneratePieIcon(MEMINFO *mi);
void UpdateTrayMemory(MEMINFO *mi);
void UpdateMenuMemory(MEMINFO *mi);
void UpdateCpuInfo(CPUINFO *ci);
HICON GenerateBarIcon(CPUINFO *ci);
void UpdateTrayCpu(CPUINFO *ci);
void UpdateMenuCpu(CPUINFO *ci);
void SetMenuText(HMENU m, UINT id, LPCTSTR text);
void SetLanguage(LANG *newLang);
void InitializeMenuWithIcons(void);
void CleanupMenuIcons(void);
HBITMAP CreateColorBitmap(COLORREF color, int width, int height);
void SaveLanguageSelectionToRegistry(void);
BOOL LoadLanguageSelectionFromRegistry(void);
void RefreshMenuText(void);
BOOL IsAutoStartEnabled(void);
void SetAutoStart(BOOL enable);
void UpdateIconVisibility(void);
void SaveMenuStateToRegistry(void);
void LoadMenuStateFromRegistry(void);
void SaveRefreshIntervalToRegistry(void);
void LoadRefreshIntervalFromRegistry(void);
INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK RefreshIntervalDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ShowAboutDialog(HWND hwndParent);
void RebuildCpuMenuItems(CPUINFO *ci);
void SaveCpuDisplaySettingsToRegistry(void);
void LoadCpuDisplaySettingsFromRegistry(void);

// Global variables
static LANG *g_lang = &lang_en;
static HINSTANCE g_hInst;
static HWND g_hWnd;
static HMENU g_hMenu;
static HMENU g_hLangMenu;
static HMENU g_hCpuDisplayMenu;
static NOTIFYICONDATA g_nidMemory = {0};
static NOTIFYICONDATA g_nidCpu = {0};
static HBITMAP g_hBmpUsed;
static HBITMAP g_hBmpFree;
static HICON g_hIconMemory;
static HICON g_hIconCpu;
static BOOL g_autoStartChecked;
static DWORD g_refreshInterval = 1000;
static DWORDLONG g_lastUsedMB;
static int g_lastBars[ICON_SIZE];   // packed height+color of the bars in the last CPU icon
static BOOL g_menuOpen;             // menu texts are only refreshed while the menu is visible
static PDH_HQUERY g_pdhQuery;       // one query holds every counter: one collect per tick
static BOOL g_pdhOk;
static PDH_HCOUNTER g_pdhCounterCache[CACHE_COUNTERS];
static PDH_HCOUNTER g_pdhCounterCommittedBytes;
static PDH_HCOUNTER g_pdhCountersCpu[MAX_CORES];
static int g_coreCount;
static BOOL g_showMemoryIcon = TRUE;
static BOOL g_showCpuIcon = TRUE;
static int g_cpuDisplayMode = CPU_DISPLAY_MODE_ALL;
static int screenWidth = 0, screenHeight = 0;

static const LPCTSTR kCachePaths[CACHE_COUNTERS] = {
    _T("\\Memory\\Cache Bytes"),
    _T("\\Memory\\Modified Page List Bytes"),
    _T("\\Memory\\Standby Cache Core Bytes"),
    _T("\\Memory\\Standby Cache Normal Priority Bytes"),
    _T("\\Memory\\Standby Cache Reserve Bytes"),
};

static const BYTE kZeroMask[(ICON_SIZE / 8) * ICON_SIZE]; // all-zero icon mask

// Rebuilds the CPU section of the context menu based on display mode
void RebuildCpuMenuItems(CPUINFO *ci) {
    // Remove the existing CPU items (one loop over the real core count)
    for (int i = 0; i < g_coreCount; i++) {
        RemoveMenu(g_hMenu, ID_MENU_CPU_BASE + i, MF_BYCOMMAND);
    }

    // Reference the CPU Display submenu handle for correct insertion position
    UINT_PTR insertRef = (UINT_PTR)g_hCpuDisplayMenu;

    TCHAR buf[64];
    if (g_cpuDisplayMode == CPU_DISPLAY_MODE_ALL) {
        for (int i = g_coreCount - 1; i >= 0; i--) {
            StringCchPrintf(buf, 64, g_lang->cpu_core_format, i + 1, ci ? ci->percents[i] : 0.0);
            InsertMenu(g_hMenu, insertRef, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_BASE + i, buf);
        }
    } else if (g_cpuDisplayMode == CPU_DISPLAY_MODE_AVG) {
        // stats are precomputed by UpdateCpuInfo (one pass per tick)
        StringCchPrintf(buf, 64, g_lang->cpu_avg_format, ci ? ci->avg : 0.0);
        InsertMenu(g_hMenu, insertRef, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_BASE, buf);
    } else {
        StringCchPrintf(buf, 64, g_lang->cpu_max_format,
                        ci ? ci->max : 0.0, ci ? ci->max_core + 1 : 1);
        InsertMenu(g_hMenu, insertRef, MF_BYCOMMAND | MF_STRING, ID_MENU_CPU_BASE, buf);
    }
}

// Updates memory usage information using system and PDH counters.
// The query data itself is collected once per tick in the WM_TIMER handler.
void UpdateMemoryInfo(MEMINFO *mi) {
    MEMORYSTATUSEX ms = { sizeof(ms) };
    if (!GlobalMemoryStatusEx(&ms) || ms.ullTotalPhys == 0) return;

    mi->totalPhys = ms.ullTotalPhys;
    mi->availPhys = ms.ullAvailPhys;
    mi->usedPhys = mi->totalPhys - mi->availPhys;
    mi->usedPercent = (double)mi->usedPhys / mi->totalPhys * 100.0;
    mi->totalSwap = ms.ullTotalPageFile;

    if (g_pdhOk) {
        PDH_FMT_COUNTERVALUE cv;
        DWORDLONG totalCached = 0;
        for (int i = 0; i < CACHE_COUNTERS; i++) {
            if (PdhGetFormattedCounterValue(g_pdhCounterCache[i], PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS)
                totalCached += (DWORDLONG)cv.largeValue;
        }
        mi->cachedBytes = totalCached;

        mi->committedBytes = (PdhGetFormattedCounterValue(g_pdhCounterCommittedBytes, PDH_FMT_LARGE, NULL, &cv) == ERROR_SUCCESS)
            ? (DWORDLONG)cv.largeValue : 0;
    }

    mi->swapUsed = (mi->committedBytes > mi->usedPhys) ? (mi->committedBytes - mi->usedPhys) : 0;
}

// Creates a 32bpp ARGB icon from raw pixel data (no GDI device contexts)
static HICON CreateArgbIcon(const DWORD *pix) {
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = ICON_SIZE;
    bmi.bmiHeader.biHeight = -ICON_SIZE;      // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP hColor = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (!hColor || !bits) {
        if (hColor) DeleteObject(hColor);
        return NULL;
    }
    memcpy(bits, pix, sizeof(DWORD) * ICON_SIZE * ICON_SIZE);

    HBITMAP hMask = CreateBitmap(ICON_SIZE, ICON_SIZE, 1, 1, kZeroMask);
    HICON hIcon = NULL;
    if (hMask) {
        ICONINFO ii = { TRUE, 0, 0, hMask, hColor };
        hIcon = CreateIconIndirect(&ii);
        DeleteObject(hMask);
    }
    DeleteObject(hColor);
    return hIcon;
}

// The pie icon's subpixel geometry never changes, so the expensive atan2
// angles are precomputed once into a lookup table (built lazily on first
// use): 0 = subpixel outside the circle, otherwise angle in 1/65536 turns + 1.
// A companion table holds each subpixel's edge-fade (feather) weight, so the
// render loop needs no sqrt either.  Both tables are heap-allocated: as
// statics they would add ~320 KB to the PE image on this toolchain.
enum { PIE_SS = 8, PIE_SCALE = 65536, PIE_SUBPX = PIE_SS * PIE_SS };
static DWORD *g_pieAngles;
static BYTE  *g_pieFeather;
static BOOL g_pieAnglesReady;

static BOOL InitPieAngles(void) {
    const size_t n = (size_t)ICON_SIZE * ICON_SIZE * PIE_SUBPX;
    const double c = ICON_SIZE / 2.0, R = c - 0.5, R2 = R * R;
    const double twoPi = 6.28318530717958647692;
    DWORD *p = g_pieAngles = calloc(n, sizeof(DWORD));   // zeroed: 0 = outside circle
    BYTE *q = g_pieFeather = calloc(n, sizeof(BYTE));
    if (!p || !q) {
        free(g_pieAngles);   g_pieAngles = NULL;
        free(g_pieFeather);  g_pieFeather = NULL;
        return FALSE;
    }
    for (int y = 0; y < ICON_SIZE; y++) {
        for (int x = 0; x < ICON_SIZE; x++) {
            for (int sy = 0; sy < PIE_SS; sy++) {
                for (int sx = 0; sx < PIE_SS; sx++, p++, q++) {
                    double dx = x + (sx + 0.5) / PIE_SS - c;
                    double dy = y + (sy + 0.5) / PIE_SS - c;
                    double d2 = dx * dx + dy * dy;
                    if (d2 > R2) continue;
                    double a = atan2(dx, -dy);       // 0 at 12 o'clock, clockwise
                    if (a < 0.0) a += twoPi;
                    *p = (DWORD)(a * (PIE_SCALE / twoPi)) + 1;

                    // edge fade over the outermost 1.5 px, sampled per subpixel
                    double af = (R + 0.5 - sqrt(d2)) / 1.5;
                    if (af > 1.0) af = 1.0;
                    *q = (BYTE)(af * 255.0 + 0.5);
                }
            }
        }
    }
    g_pieAnglesReady = TRUE;
    return TRUE;
}

// Generates a pie chart icon representing memory usage: pure pixel math with
// 8x8 supersampling and a subpixel-accurate feathered edge, instead of the
// old GDI pipeline (3 DCs, 2 Polygon calls, HALFTONE StretchBlt, mask Ellipse).
HICON GeneratePieIcon(MEMINFO *mi)
{
    DWORD pix[ICON_SIZE * ICON_SIZE];
    if (!g_pieAnglesReady && !InitPieAngles()) return NULL;
    const DWORD thr = (DWORD)(mi->usedPercent * (PIE_SCALE / 100.0));   // used iff angle < thr
    const DWORD *a = g_pieAngles;
    const BYTE *f = g_pieFeather;

    for (int i = 0; i < ICON_SIZE * ICON_SIZE; i++, a += PIE_SUBPX, f += PIE_SUBPX) {
        int nUsed = 0, nFree = 0, fade = 0;
        for (int s = 0; s < PIE_SUBPX; s++) {
            DWORD v = a[s];
            if (v == 0) continue;               // subpixel outside the circle
            fade += f[s];                       // feather weight of this subpixel
            if (v - 1 < thr) nUsed++; else nFree++;
        }
        int n = nUsed + nFree;
        if (n == 0) { pix[i] = 0; continue; }    // transparent background

        DWORD gray = (DWORD)((nUsed * 200 + nFree * 100) / n);
        DWORD alpha = (DWORD)((fade + PIE_SUBPX / 2) / PIE_SUBPX);   // average 0..255 weight
        pix[i] = (alpha << 24) | (gray << 16) | (gray << 8) | gray;
    }
    return CreateArgbIcon(pix);
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

// Creates a solid color bitmap for menu icons (DIB fill, no DC needed)
HBITMAP CreateColorBitmap(COLORREF color, int width, int height) {
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *bits = NULL;
    HBITMAP hBitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &bits, NULL, 0);
    if (hBitmap && bits) {
        DWORD argb = 0xFF000000u | (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);
        DWORD *p = (DWORD *)bits;
        for (int i = 0; i < width * height; i++) p[i] = argb;
    }
    return hBitmap;
}

// MIIM_STRING keeps the item's bitmap, unlike ModifyMenu
void SetMenuText(HMENU m, UINT id, LPCTSTR text) {
    MENUITEMINFO mii = {0};
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_STRING;
    mii.dwTypeData = (LPTSTR)text;
    SetMenuItemInfo(m, id, FALSE, &mii);
}

// Updates the system tray memory icon; Shell_NotifyIcon is only called when
// the tooltip or the icon actually changed
void UpdateTrayMemory(MEMINFO *mi) {
    TCHAR tip[128];
    StringCchPrintf(tip, 128, g_lang->tooltip_memory_usage, mi->usedPercent);
    BOOL tipChanged = _tcscmp(tip, g_nidMemory.szTip) != 0;
    if (tipChanged) StringCchCopy(g_nidMemory.szTip, ARRAYSIZE(g_nidMemory.szTip), tip);

    DWORDLONG delta = mi->usedPhys > g_lastUsedMB ? mi->usedPhys - g_lastUsedMB
                                                  : g_lastUsedMB - mi->usedPhys;
    if (delta > (DWORDLONG)100 * 1024 * 1024) {     // same 100 MB threshold as before
        g_lastUsedMB = mi->usedPhys;
        HICON hNewIcon = GeneratePieIcon(mi);
        if (hNewIcon) {
            if (g_hIconMemory) DestroyIcon(g_hIconMemory);
            g_hIconMemory = hNewIcon;
            g_nidMemory.hIcon = hNewIcon;
            Shell_NotifyIcon(NIM_MODIFY, &g_nidMemory);   // sends the new icon and tip
            return;
        }
    }
    if (tipChanged) Shell_NotifyIcon(NIM_MODIFY, &g_nidMemory);
}

// Updates the four memory items of the context menu; MIIM_STRING keeps the
// Used/Free bitmaps, so no per-tick bitmap re-application is needed
void UpdateMenuMemory(MEMINFO *mi) {
    TCHAR buf[64];
    StringCchPrintf(buf, 64, g_lang->menu_used,   (UINT)(mi->usedPhys / (1024 * 1024)));
    SetMenuText(g_hMenu, ID_MENU_USED, buf);
    StringCchPrintf(buf, 64, g_lang->menu_free,   (UINT)(mi->availPhys / (1024 * 1024)));
    SetMenuText(g_hMenu, ID_MENU_FREE, buf);
    StringCchPrintf(buf, 64, g_lang->menu_cached, (UINT)(mi->cachedBytes / (1024 * 1024)));
    SetMenuText(g_hMenu, ID_MENU_CACHED, buf);
    StringCchPrintf(buf, 64, g_lang->menu_swap,   (UINT)(mi->swapUsed / (1024 * 1024)));
    SetMenuText(g_hMenu, ID_MENU_SWAP, buf);
}

// Updates CPU usage information and computes avg/max in the same pass.
// The query data itself is collected once per tick in the WM_TIMER handler.
void UpdateCpuInfo(CPUINFO *ci) {
    ci->core_count = g_coreCount;
    if (g_pdhOk) {
        for (int i = 0; i < g_coreCount; i++) {
            PDH_FMT_COUNTERVALUE cv;
            if (PdhGetFormattedCounterValue(g_pdhCountersCpu[i], PDH_FMT_DOUBLE, NULL, &cv) == ERROR_SUCCESS) {
                double v = cv.doubleValue;
                ci->percents[i] = v < 0.0 ? 0.0 : (v > 100.0 ? 100.0 : v);
            }
        }
    }

    double sum = 0.0, m = 0.0;
    int mc = 0;
    for (int i = 0; i < ci->core_count; i++) {
        sum += ci->percents[i];
        if (i == 0 || ci->percents[i] > m) { m = ci->percents[i]; mc = i; }
    }
    ci->avg = ci->core_count ? sum / ci->core_count : 0.0;
    ci->max = m;
    ci->max_core = mc;
}

// Generates a bar chart icon for CPU usage (direct pixel writes, no DCs).
// Returns NULL when the visible bars are identical to the previously rendered
// icon, so the DIB + icon + tray update is skipped entirely.
HICON GenerateBarIcon(CPUINFO *ci) {
    int numCores = ci->core_count > 0 ? ci->core_count : 1;
    int numGroups = numCores < ICON_SIZE ? numCores : ICON_SIZE;
    int bars[ICON_SIZE];                       // packed bar height + color flag

    for (int g = 0; g < numGroups; g++) {
        int c0 = g * numCores / numGroups, c1 = (g + 1) * numCores / numGroups;  // c1 > c0
        double sum = 0.0;
        for (int i = c0; i < c1; i++) sum += ci->percents[i];
        double avg = sum / (c1 - c0);

        int h = (int)(avg * ICON_SIZE / 100.0);
        if (h < 1) h = 1;
        if (h > ICON_SIZE) h = ICON_SIZE;
        bars[g] = h | (avg > 50.0 ? 256 : 0);
    }

    if (memcmp(bars, g_lastBars, numGroups * sizeof(int)) == 0)
        return NULL;                           // nothing visible changed

    DWORD pix[ICON_SIZE * ICON_SIZE];
    memset(pix, 0, sizeof(pix));               // transparent background
    for (int g = 0; g < numGroups; g++) {
        int h = bars[g] & 0xFF;
        DWORD color = (bars[g] & 256) ? 0xFFC86464u : 0xFF64C864u;   // RGB(200,100,100) / RGB(100,200,100)
        int x0 = g * ICON_SIZE / numGroups, x1 = (g + 1) * ICON_SIZE / numGroups;
        for (int y = ICON_SIZE - h; y < ICON_SIZE; y++)
            for (int x = x0; x < x1; x++)
                pix[y * ICON_SIZE + x] = color;
    }

    HICON hIcon = CreateArgbIcon(pix);
    if (hIcon) memcpy(g_lastBars, bars, numGroups * sizeof(int));  // on failure retry next tick
    return hIcon;
}

// Updates the system tray CPU icon; Shell_NotifyIcon is only called when
// the tooltip or the icon actually changed
void UpdateTrayCpu(CPUINFO *ci) {
    TCHAR tip[128];
    StringCchPrintf(tip, 128, g_lang->tooltip_cpu_avg, ci->avg);
    BOOL tipChanged = _tcscmp(tip, g_nidCpu.szTip) != 0;
    if (tipChanged) StringCchCopy(g_nidCpu.szTip, ARRAYSIZE(g_nidCpu.szTip), tip);

    HICON hNewIcon = GenerateBarIcon(ci);      // NULL when the bars did not change
    if (hNewIcon) {
        if (g_hIconCpu) DestroyIcon(g_hIconCpu);
        g_hIconCpu = hNewIcon;
        g_nidCpu.hIcon = hNewIcon;
        Shell_NotifyIcon(NIM_MODIFY, &g_nidCpu);   // sends the new icon and tip
        return;
    }
    if (tipChanged) Shell_NotifyIcon(NIM_MODIFY, &g_nidCpu);
}

// Updates the CPU items of the context menu for the current display mode
// (stats come precomputed from UpdateCpuInfo)
void UpdateMenuCpu(CPUINFO *ci) {
    TCHAR buf[64];
    if (g_cpuDisplayMode == CPU_DISPLAY_MODE_ALL) {
        for (int i = 0; i < ci->core_count; i++) {
            StringCchPrintf(buf, 64, g_lang->cpu_core_format, i + 1, ci->percents[i]);
            SetMenuText(g_hMenu, ID_MENU_CPU_BASE + i, buf);
        }
    } else if (g_cpuDisplayMode == CPU_DISPLAY_MODE_AVG) {
        StringCchPrintf(buf, 64, g_lang->cpu_avg_format, ci->avg);
        SetMenuText(g_hMenu, ID_MENU_CPU_BASE, buf);
    } else {
        StringCchPrintf(buf, 64, g_lang->cpu_max_format, ci->max, ci->max_core + 1);
        SetMenuText(g_hMenu, ID_MENU_CPU_BASE, buf);
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
BOOL LoadLanguageSelectionFromRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD langId, dataSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey, _T("Language"), NULL, NULL, (LPBYTE)&langId, &dataSize) == ERROR_SUCCESS) {
            switch (langId) {
                case ID_MENU_LANG_EN: g_lang = &lang_en; break;
                case ID_MENU_LANG_HU: g_lang = &lang_hu; break;
                case ID_MENU_LANG_DE: g_lang = &lang_de; break;
                case ID_MENU_LANG_IT: g_lang = &lang_it; break;
                case ID_MENU_LANG_ES: g_lang = &lang_es; break;
                case ID_MENU_LANG_FR: g_lang = &lang_fr; break;
                case ID_MENU_LANG_RU: g_lang = &lang_ru; break;
                default: g_lang = &lang_en; break;
            }
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }
    return FALSE;
}

// Refreshes menu text with current language
void RefreshMenuText(void) {
    SetMenuText(g_hMenu, ID_MENU_ABOUT, g_lang->menu_about);
    SetMenuText(g_hMenu, ID_MENU_AUTOSTART, g_lang->menu_autostart);
    SetMenuText(g_hMenu, ID_MENU_SHOW_MEMORY_ICON, g_lang->menu_show_memory_icon);
    SetMenuText(g_hMenu, ID_MENU_SHOW_CPU_ICON, g_lang->menu_show_cpu_icon);
    SetMenuText(g_hMenu, ID_MENU_REFRESH_INTERVAL, g_lang->menu_refresh_interval);
    SetMenuText(g_hMenu, ID_MENU_EXIT, g_lang->menu_exit);
    CheckMenuItem(g_hMenu, ID_MENU_AUTOSTART, MF_BYCOMMAND | (g_autoStartChecked ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hMenu, ID_MENU_SHOW_MEMORY_ICON, MF_BYCOMMAND | (g_showMemoryIcon ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hMenu, ID_MENU_SHOW_CPU_ICON, MF_BYCOMMAND | (g_showCpuIcon ? MF_CHECKED : MF_UNCHECKED));
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

    MENUITEMINFO miiCpu = {0};
    miiCpu.cbSize = sizeof(MENUITEMINFO);
    miiCpu.fMask = MIIM_STRING | MIIM_SUBMENU;
    miiCpu.dwTypeData = g_lang->menu_cpu_display;
    miiCpu.hSubMenu = g_hCpuDisplayMenu;
    SetMenuItemInfo(g_hMenu, (UINT_PTR)g_hCpuDisplayMenu, FALSE, &miiCpu);

    SetMenuText(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_ALL, g_lang->menu_cpu_show_all);
    SetMenuText(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_AVG, g_lang->menu_cpu_show_avg);
    SetMenuText(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_MAX, g_lang->menu_cpu_show_max);

    CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_ALL, MF_BYCOMMAND | (g_cpuDisplayMode == CPU_DISPLAY_MODE_ALL ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_AVG, MF_BYCOMMAND | (g_cpuDisplayMode == CPU_DISPLAY_MODE_AVG ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_MAX, MF_BYCOMMAND | (g_cpuDisplayMode == CPU_DISPLAY_MODE_MAX ? MF_CHECKED : MF_UNCHECKED));

    RebuildCpuMenuItems(NULL);
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
        TCHAR szQuotedPath[MAX_PATH + 2];
        if (GetModuleFileName(NULL, szPath, MAX_PATH)) {
            StringCchPrintf(szQuotedPath, MAX_PATH + 2, _T("\"%s\""), szPath);
            RegSetValueEx(hKey, AUTO_START_REG_VALUE, 0, REG_SZ, (const BYTE *)szQuotedPath, (lstrlen(szQuotedPath) + 1) * sizeof(TCHAR));
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

// Saves the CPU display mode to the registry
void SaveCpuDisplaySettingsToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        DWORD cpuMode = (DWORD)g_cpuDisplayMode;
        RegSetValueEx(hKey, _T("CpuDisplayMode"), 0, REG_DWORD, (const BYTE *)&cpuMode, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// Loads the CPU display mode from the registry
void LoadCpuDisplaySettingsFromRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dataSize = sizeof(DWORD);
        DWORD cpuMode = CPU_DISPLAY_MODE_ALL;
        RegQueryValueEx(hKey, _T("CpuDisplayMode"), NULL, NULL, (LPBYTE)&cpuMode, &dataSize);
        g_cpuDisplayMode = (int)cpuMode;
        RegCloseKey(hKey);
    }
}

// Saves the refresh interval to the registry
void SaveRefreshIntervalToRegistry(void) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, _T("RefreshInterval"), 0, REG_DWORD, (const BYTE *)&g_refreshInterval, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// Loads the refresh interval from the registry
void LoadRefreshIntervalFromRegistry(void) {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\ResourceMonitor"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dataSize = sizeof(DWORD);
        RegQueryValueEx(hKey, _T("RefreshInterval"), NULL, NULL, (LPBYTE)&g_refreshInterval, &dataSize);
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
            HFONT hFont = (HFONT)SendDlgItemMessage(hwndDlg, IDC_WEBSITE, WM_GETFONT, 0, 0);
            LOGFONT lf;
            GetObject(hFont, sizeof(LOGFONT), &lf);
            lf.lfUnderline = TRUE;
            hLinkFont = CreateFontIndirect(&lf);
            SendDlgItemMessage(hwndDlg, IDC_WEBSITE, WM_SETFONT, (WPARAM)hLinkFont, (LPARAM)TRUE);
            SendDlgItemMessage(hwndDlg, IDC_COFFEE, WM_SETFONT, (WPARAM)hLinkFont, (LPARAM)TRUE);

            hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

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
            HDC hdc = (HDC)wParam;
            HWND hwndStatic = (HWND)lParam;
            int ctrlId = GetDlgCtrlID(hwndStatic);

            if (ctrlId == IDC_WEBSITE || ctrlId == IDC_COFFEE) {
                SetTextColor(hdc, RGB(0, 0, 255));
                SetBkMode(hdc, TRANSPARENT);
                return (LRESULT)hBrush;
            }
            break;
        }
        case WM_COMMAND:
            if (HIWORD(wParam) == STN_CLICKED) {
                 if (LOWORD(wParam) == IDC_WEBSITE) {
                    ShellExecute(NULL, _T("open"), _T("https://github.com/lutischan-ferenc/resource-monitor-v2"), NULL, NULL, SW_SHOWNORMAL);
                    return TRUE;
                } 
                if (LOWORD(wParam) == IDC_COFFEE) {
                    ShellExecute(NULL, _T("open"), _T("https://www.buymeacoffee.com/lutischanf"), NULL, NULL, SW_SHOWNORMAL);
                    return TRUE;
                }
            }
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                if (hLinkFont) { DeleteObject(hLinkFont); hLinkFont = NULL; }
                if (hBrush) { DeleteObject(hBrush); hBrush = NULL; }
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
        case WM_DESTROY:
        case WM_CLOSE:
            if (hLinkFont) { DeleteObject(hLinkFont); hLinkFont = NULL; }
            if (hBrush) { DeleteObject(hBrush); hBrush = NULL; }
            if (uMsg == WM_CLOSE) EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

// Refresh interval dialog procedure
INT_PTR CALLBACK RefreshIntervalDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: {
            SetWindowText(hwndDlg, g_lang->dialog_refresh_title);
            SetDlgItemText(hwndDlg, IDC_STATIC_REFRESH_LABEL, g_lang->dialog_refresh_label);

            HWND hSlider = GetDlgItem(hwndDlg, IDC_SLIDER_REFRESH_INTERVAL);
            // Range: 500ms to 5000ms, in 250ms steps. (5000-500)/250 = 18 steps.
            SendMessage(hSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 18));
            SendMessage(hSlider, TBM_SETPAGESIZE, 0, (LPARAM)1);

            int pos = (g_refreshInterval - 500) / 250;
            SendMessage(hSlider, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)pos);

            TCHAR buf[32];
            StringCchPrintf(buf, 32, _T("%d ms"), g_refreshInterval);
            SetDlgItemText(hwndDlg, IDC_STATIC_REFRESH_VALUE, buf);
            return TRUE;
        }
        case WM_HSCROLL: {
            HWND hSlider = (HWND)lParam;
            if (GetDlgCtrlID(hSlider) == IDC_SLIDER_REFRESH_INTERVAL) {
                int pos = SendMessage(hSlider, TBM_GETPOS, 0, 0);
                int value = 500 + pos * 250;
                TCHAR buf[32];
                StringCchPrintf(buf, 32, _T("%d ms"), value);
                SetDlgItemText(hwndDlg, IDC_STATIC_REFRESH_VALUE, buf);
            }
            break;
        }
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                HWND hSlider = GetDlgItem(hwndDlg, IDC_SLIDER_REFRESH_INTERVAL);
                int pos = SendMessage(hSlider, TBM_GETPOS, 0, 0);
                g_refreshInterval = 500 + pos * 250;
                SetTimer(g_hWnd, 1, g_refreshInterval, NULL);
                SaveRefreshIntervalToRegistry();
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

// Window procedure for handling system tray and menu events
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static MEMINFO mi;
    static CPUINFO ci;

    switch (msg) {
    case WM_CREATE:
        g_hWnd = hWnd;
        LoadRefreshIntervalFromRegistry();
        SetTimer(hWnd, 1, g_refreshInterval, NULL);
        break;

    case WM_TIMER:
        g_pdhOk = g_pdhQuery && PdhCollectQueryData(g_pdhQuery) == ERROR_SUCCESS;   // one sample for all counters
        UpdateMemoryInfo(&mi);
        UpdateCpuInfo(&ci);
        UpdateTrayMemory(&mi);
        UpdateTrayCpu(&ci);
        if (g_menuOpen) {              // the visible menu text stays live
            UpdateMenuMemory(&mi);
            UpdateMenuCpu(&ci);
        }
        break;

    case WM_TRAYICON:
        if ((wParam == 1 || wParam == 2) && lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            UpdateMenuMemory(&mi);
            UpdateMenuCpu(&ci);
            TrackPopupMenu(g_hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
        }
        break;

    case WM_ENTERMENULOOP:            // menu texts are refreshed only while visible
        g_menuOpen = TRUE;
        break;

    case WM_EXITMENULOOP:
        g_menuOpen = FALSE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MENU_ABOUT:
            ShowAboutDialog(hWnd);
            break;
        case ID_MENU_REFRESH_INTERVAL:
            DialogBox(g_hInst, MAKEINTRESOURCE(IDD_REFRESH_INTERVAL), hWnd, RefreshIntervalDlgProc);
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
        case ID_MENU_CPU_SHOW_ALL:
            g_cpuDisplayMode = CPU_DISPLAY_MODE_ALL;
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_ALL, MF_BYCOMMAND | MF_CHECKED);
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_AVG, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_MAX, MF_BYCOMMAND | MF_UNCHECKED);
            RebuildCpuMenuItems(NULL);
            SaveCpuDisplaySettingsToRegistry();
            break;
        case ID_MENU_CPU_SHOW_AVG:
            g_cpuDisplayMode = CPU_DISPLAY_MODE_AVG;
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_ALL, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_AVG, MF_BYCOMMAND | MF_CHECKED);
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_MAX, MF_BYCOMMAND | MF_UNCHECKED);
            RebuildCpuMenuItems(NULL);
            SaveCpuDisplaySettingsToRegistry();
            break;
        case ID_MENU_CPU_SHOW_MAX:
            g_cpuDisplayMode = CPU_DISPLAY_MODE_MAX;
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_ALL, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_AVG, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_MAX, MF_BYCOMMAND | MF_CHECKED);
            RebuildCpuMenuItems(NULL);
            SaveCpuDisplaySettingsToRegistry();
            break;
        }
        break;

    case WM_DESTROY:
        CleanupMenuIcons();
        Shell_NotifyIcon(NIM_DELETE, &g_nidMemory);
        Shell_NotifyIcon(NIM_DELETE, &g_nidCpu);
        if (g_hIconMemory) DestroyIcon(g_hIconMemory);
        if (g_hIconCpu) DestroyIcon(g_hIconCpu);
        if (g_hMenu) DestroyMenu(g_hMenu);      // also destroys the submenus
        if (g_pdhQuery) PdhCloseQuery(g_pdhQuery);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// Application entry point
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
    HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, _T("ResourceMonitorEvent"));
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, _T("The program is already running. Exiting."), _T("Warning"), MB_OK | MB_ICONWARNING);
        return 0;
    }

    g_hInst = hInstance;
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };   // trackbar in the refresh dialog
    InitCommonControlsEx(&icc);
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

    // CPU Display mode submenu
    g_hCpuDisplayMenu = CreatePopupMenu();
    AppendMenu(g_hCpuDisplayMenu, MF_STRING | MF_CHECKED, ID_MENU_CPU_SHOW_ALL, g_lang->menu_cpu_show_all);
    AppendMenu(g_hCpuDisplayMenu, MF_STRING, ID_MENU_CPU_SHOW_AVG, g_lang->menu_cpu_show_avg);
    AppendMenu(g_hCpuDisplayMenu, MF_STRING, ID_MENU_CPU_SHOW_MAX, g_lang->menu_cpu_show_max);
    AppendMenu(g_hMenu, MF_POPUP, (UINT_PTR)g_hCpuDisplayMenu, g_lang->menu_cpu_display);

    AppendMenu(g_hMenu, MF_STRING | MF_CHECKED, ID_MENU_AUTOSTART, g_lang->menu_autostart);
    AppendMenu(g_hMenu, MF_STRING | MF_CHECKED, ID_MENU_SHOW_MEMORY_ICON, g_lang->menu_show_memory_icon);
    AppendMenu(g_hMenu, MF_STRING | MF_CHECKED, ID_MENU_SHOW_CPU_ICON, g_lang->menu_show_cpu_icon);
    AppendMenu(g_hMenu, MF_STRING, ID_MENU_REFRESH_INTERVAL, g_lang->menu_refresh_interval);
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

    // Initialize PDH counters: a single query holds every memory and CPU
    // counter. The one priming collect makes the rate counters valid from the
    // first timer tick on, so no Sleep() loops are needed at startup.
    if (PdhOpenQuery(NULL, 0, &g_pdhQuery) == ERROR_SUCCESS) {
        for (int i = 0; i < CACHE_COUNTERS; i++)
            PdhAddEnglishCounter(g_pdhQuery, kCachePaths[i], 0, &g_pdhCounterCache[i]);
        PdhAddEnglishCounter(g_pdhQuery, _T("\\Memory\\Committed Bytes"), 0, &g_pdhCounterCommittedBytes);
        for (int i = 0; i < g_coreCount; i++) {
            TCHAR path[128];
            StringCchPrintf(path, 128, _T("\\Processor(%d)\\%% Processor Time"), i);
            PdhAddEnglishCounter(g_pdhQuery, path, 0, &g_pdhCountersCpu[i]);
        }
        PdhCollectQueryData(g_pdhQuery);
    } else {
        g_pdhQuery = NULL;
    }

    if (!LoadLanguageSelectionFromRegistry()) {
        LANGID langId = GetUserDefaultUILanguage();
        switch (langId) {
            case 0x0409: g_lang = &lang_en; break;
            case 0x040e: g_lang = &lang_hu; break;
            case 0x0407: g_lang = &lang_de; break;
            case 0x0410: g_lang = &lang_it; break;
            case 0x0c0a: g_lang = &lang_es; break;
            case 0x040c: g_lang = &lang_fr; break;
            case 0x0419: g_lang = &lang_ru; break;
            default: g_lang = &lang_en; break;
        }
    }

    g_autoStartChecked = IsAutoStartEnabled();
    if (!g_autoStartChecked) {
        g_autoStartChecked = TRUE;
        SetAutoStart(TRUE);
    }
    LoadMenuStateFromRegistry();
    LoadCpuDisplaySettingsFromRegistry();
    if (g_cpuDisplayMode != CPU_DISPLAY_MODE_ALL) {
        RebuildCpuMenuItems(NULL);
    }
    RefreshMenuText();
    CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_ALL, MF_BYCOMMAND | (g_cpuDisplayMode == CPU_DISPLAY_MODE_ALL ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_AVG, MF_BYCOMMAND | (g_cpuDisplayMode == CPU_DISPLAY_MODE_AVG ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(g_hCpuDisplayMenu, ID_MENU_CPU_SHOW_MAX, MF_BYCOMMAND | (g_cpuDisplayMode == CPU_DISPLAY_MODE_MAX ? MF_CHECKED : MF_UNCHECKED));
    UpdateIconVisibility();

    // First refresh immediately instead of waiting one full interval: the
    // tray icons, menu and the PDH baseline sample above are all ready, so
    // the default icon is replaced right away (e.g. no 5 s blank at 5 s).
    SendMessage(g_hWnd, WM_TIMER, 1, 0);

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
