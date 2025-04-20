# CPU and Memory Monitor

This repository contains two system tray applications written in Go:
- **CPU Monitor**: Displays CPU usage per core and updates a bar chart icon in the system tray.


  CPU monitor systray icon with CPU usage:

  ![CPU tooltip](images/cpu_icon_tooltip.png "CPU monitor systray icon with CPU usage")


  CPU / mem monitor systray icon with menu:

 ![menu](images/menu.png "CPU / mem monitor systray icon with menu")

- **Memory Monitor**: Displays memory usage statistics and updates a pie chart icon in the system tray.


  Mem monitor systray icon with Mem usage:

  ![Memory tooltip](images/mem_tooltip.png "Mem monitor systray icon with Mem usage")

## Build Windows

To build and run the applications on macOS:

```sh
# Build Resource Monitor
\mingw32\bin\windres resource-monitor.rc -o resource-monitor_res.o
\mingw32\bin\gcc -ffunction-sections -fdata-sections -s -o resource-monitor resource-monitor.c resource-monitor_res.o -lpdh -mwindows -lwinmm -Wl,--gc-sections -static-libgcc -municode
```

## Features
- Displays live CPU core usage in the system tray.
- Displays memory usage statistics in the system tray.
- Updates system tray icons dynamically to reflect real-time usage.
- Provides an exit option to close the application.

## License
This project is open-source and available under the MIT License.

