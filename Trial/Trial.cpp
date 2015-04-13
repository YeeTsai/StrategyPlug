// Trial.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <locale.h>

typedef void __declspec(dllimport) (*InstallHookFunc)(BOOL IsHook, DWORD dwThreadId);
BOOL CALLBACK EnumChildWinFunc(HWND hwnd, LPARAM lparam);

int _tmain(int argc, _TCHAR* argv[])
{
    setlocale(LC_CTYPE, "chs");
    const char* winTitle = "海通证券彩虹投资(通达信)V1.24 - [行情图-自选股]";
    HWND hwnd = FindWindow(NULL, winTitle);

    if (hwnd != NULL) {
        EnumChildWindows(hwnd, EnumChildWinFunc, NULL);
    }

    Sleep(50000);

	return 0;
}

BOOL CALLBACK EnumChildWinFunc(HWND hwnd, LPARAM lparam) {
    char classname[256];
    char title[256];
    memset(classname, 0x00, sizeof(classname));
    memset(title, 0x00, sizeof(title));
    GetClassName(hwnd, classname, 255);
    GetWindowText(hwnd, title, 255);

    if (strcmp(classname, "AfxFrameOrView42") == 0 &&
        strcmp(title, "") == 0) {
        DWORD dwPID;
        DWORD dwThreadID = GetWindowThreadProcessId(hwnd, &dwPID);

        fprintf(stdout, "class %s get pid %ld threadid %ld\n", classname, dwPID, dwThreadID);

        HINSTANCE dll = LoadLibrary("TrialHook-d.dll");
        if (dll == NULL) {
            fprintf(stderr, "load dll error!\n");
            return -1;
        }

        InstallHookFunc installHook = (InstallHookFunc)GetProcAddress(dll, "InstallHook");
        if (installHook == NULL) {
            fprintf(stderr, "get InstallHook function error!\n");
            return -1;
        }

        installHook(TRUE, dwThreadID);
    }

    return TRUE;
}
