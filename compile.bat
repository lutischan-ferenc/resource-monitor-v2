..\mingw32\bin\windres resource-monitor.rc -o resource-monitor_res.o
..\mingw32\bin\gcc -ffunction-sections -fdata-sections -s -o resource-monitor resource-monitor.c resource-monitor_res.o -lpdh -mwindows -lwinmm -Wl,--gc-sections -static-libgcc -municode
