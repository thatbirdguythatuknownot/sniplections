#define _WIN32_WINNT 0x0500
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#include <synchapi.h>

static inline const HWND get_roblox_frontwindow(void)
{
    TCHAR windowtext[10000];
    HWND window = GetForegroundWindow();
    if (window) {
        GetWindowText(window, windowtext, 10000);
        TCHAR *ret = StrStrA(windowtext, "Roblox");
        if (ret - windowtext == 0) return window;
    }
    return NULL;
}

int main(int argc, char **argv)
{
    HWND hwnd;
    DWORD dwstyle;

    hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_MINIMIZE);
    ShowWindow(hwnd, SW_HIDE);
    while (1) {
        if ((hwnd = get_roblox_frontwindow())) {
            dwstyle = GetWindowLong(hwnd, GWL_STYLE);
            if (!(dwstyle & WS_OVERLAPPEDWINDOW)) {
                Sleep(20);
                SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
                ChangeDisplaySettings(NULL, CDS_RESET);
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
        }
        Sleep(20);
    }
}
