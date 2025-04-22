# CPU and Memory Monitor

This repository contains a system tray application written in C that monitors CPU and memory usage:
- **CPU Monitor**: Displays CPU usage per core and updates a bar chart icon in the system tray.
- **Memory Monitor**: Displays memory usage statistics and updates a pie chart icon in the system tray.

  CPU monitor systray icon with CPU usage:

  ![CPU tooltip](images/cpu_icon_tooltip.png "CPU monitor systray icon with CPU usage")

  CPU / mem monitor systray icon with menu:

  ![menu](images/menu.png "CPU / mem monitor systray icon with menu")

  Mem monitor systray icon with Mem usage:

  ![Memory tooltip](images/mem_tooltip.png "Mem monitor systray icon with Mem usage")

## Build Windows

To build and run the application on Windows:

```sh
# Build Resource Monitor in cmd
\mingw32\bin\windres resource-monitor.rc -o resource-monitor_res.o
\mingw32\bin\gcc -ffunction-sections -fdata-sections -s -o resource-monitor resource-monitor.c resource-monitor_res.o -lpdh -mwindows -lwinmm -Wl,--gc-sections -static-libgcc -municode
```

## Features
- Displays live CPU core usage in the system tray.
- Displays memory usage statistics in the system tray.
- Updates system tray icons dynamically to reflect real-time usage.
- Provides a language selection menu with support for multiple languages.
- Provides an exit option to close the application.

## Adding a New Language

To add a new language to the Resource Monitor, follow these steps:

1. **Create a new `LANG` structure**: Define a new `LANG` structure in the source code for the new language. This structure contains all menu and tooltip strings. For example, to add Portuguese, create a `lang_pt` structure and fill it with the appropriate translations.

   ```c
   static LANG lang_pt = {
       _T("Sobre o Resource Monitor"),          // menu_about
       _T("Usado: %u MB"),                      // menu_used
       _T("Livre: %u MB"),                      // menu_free
       _T("Em cache: %u MB"),                   // menu_cached
       _T("Swap: %u MB"),                       // menu_swap
       _T("CPU %d: %.2f%%"),                    // cpu_core_format
       _T("Iniciar na inicialização do sistema"), // menu_autostart
       _T("Idioma"),                            // menu_language
       _T("Sair"),                              // menu_exit
       _T("Uso de memória: %.1f%%"),            // tooltip_memory_usage
       _T("Média CPU: %.1f%%"),                 // tooltip_cpu_avg
       _T("Mostrar ícone de memória"),           // menu_show_memory_icon
       _T("Mostrar ícone de CPU")               // menu_show_cpu_icon
   };
   ```

2. **Add the new language to the menu**: In the `_tWinMain` function, append a new menu item for the language to the `g_hLangMenu` submenu. Assign it a unique identifier, such as `ID_MENU_LANG_PT`.

   ```c
   AppendMenu(g_hLangMenu, MF_STRING, ID_MENU_LANG_PT, _T("Português"));
   ```

3. **Handle the new language selection**: In the `WM_COMMAND` case of the `WndProc` function, add a new case for the language identifier to set the `g_lang` pointer to the new `LANG` structure and save the selection to the registry.

   ```c
   case ID_MENU_LANG_PT:
       SetLanguage(&lang_pt);
       SaveLanguageSelectionToRegistry();
       break;
   ```

4. **Update registry save and load functions**: Modify the `SaveLanguageSelectionToRegistry` and `LoadLanguageSelectionFromRegistry` functions to handle the new language identifier.

   - In `SaveLanguageSelectionToRegistry`, add a condition to save the new identifier:

     ```c
     DWORD langId = g_lang == &lang_en ? ID_MENU_LANG_EN :
                    g_lang == &lang_hu ? ID_MENU_LANG_HU :
                    g_lang == &lang_de ? ID_MENU_LANG_DE :
                    g_lang == &lang_it ? ID_MENU_LANG_IT :
                    g_lang == &lang_es ? ID_MENU_LANG_ES :
                    g_lang == &lang_fr ? ID_MENU_LANG_FR :
                    g_lang == &lang_ru ? ID_MENU_LANG_RU :
                    g_lang == &lang_pt ? ID_MENU_LANG_PT : ID_MENU_LANG_EN;
     ```

   - In `LoadLanguageSelectionFromRegistry`, add a case to load the new language:

     ```c
     case ID_MENU_LANG_PT: g_lang = &lang_pt; break;
     ```

5. **Update menu refresh function**: In the `RefreshMenuText` function, add a line to check the new language menu item and update its checked state.

   ```c
   CheckMenuItem(g_hLangMenu, ID_MENU_LANG_PT, MF_BYCOMMAND | (g_lang == &lang_pt ? MF_CHECKED : MF_UNCHECKED));
   ```

6. **Translate all strings**: Ensure all strings in the new `LANG` structure are accurately translated into the target language.

7. **Update UI Language Detection** (optional)
   ```c
   // In _tWinMain() auto-detection:
   switch (langId) {
       // ... existing cases ...
       case 0x0816: g_lang = &lang_pt; break; // Portugal (pt_PT)
   }
   ```
   Language codes from: https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/70feba9f-294e-491e-b6eb-56532684c37f


After completing these steps, recompile the program to enable support for the new language. The new language will appear in the language selection menu, and selecting it will update the UI accordingly, with the choice persisted in the registry.

## License
This project is open-source and available under the MIT License.