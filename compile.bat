..\mingw32\bin\windres resource-monitor.rc -o resource-monitor_res.o
..\mingw32\bin\gcc -O2 -ffunction-sections -fdata-sections -s -o resource-monitor resource-monitor.c resource-monitor_res.o -lpdh -lgdi32 -lcomctl32 -lshell32 -ladvapi32 -lwinmm -Wl,--gc-sections -static-libgcc -municode -mwindows
