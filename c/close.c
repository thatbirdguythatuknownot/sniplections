#define _WIN32_WINNT 0x0500
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#include <synchapi.h>
#include <conio.h>

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

#define GETFREER_DIR ""

int main(int argc, char **argv)
{
    HWND hwnd;
    DWORD dwstyle;
    BOOL (*func)(HWND) = DestroyWindow;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD exit_code;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmpi(argv[i], "--annoy") == 0 || strcmpi(argv[i], "-a") == 0) {
                func = CloseWindow;
                break;
            }
        }
    }
    else /* {
redo:
        printf("> ");
        char a = getch();
        if (!isspace(a)) {
            if (a != 'a' && a != 'A' && a != 'n' && a != 'N') {
                puts("invalid input! must be 'a' (annoy) or 'n' (normal)");
                goto redo;
            }
        }
        if (a == 'a' || a == 'A') {
            func = CloseWindow;
        }
    } */

    hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_MINIMIZE);
    ShowWindow(hwnd, SW_HIDE);
    while (1) {
        if (GetAsyncKeyState(VK_F12) & 0x01) {
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            if (!CreateProcess(
                NULL,
                TEXT(GETFREER_DIR "\\getfreer.exe"),
                NULL,
                NULL,
                FALSE,
                CREATE_NEW_CONSOLE,
                NULL,
                NULL,
                &si,
                &pi
            ))
            {
                continue;
            }

            WaitForSingleObject(pi.hProcess, INFINITE);
            if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
                continue;
            }
            if (exit_code == 0) {
                break;
            }
        }

        if ((hwnd = get_roblox_frontwindow())) {
            func(hwnd);
        }
        Sleep(20);
    }
}
