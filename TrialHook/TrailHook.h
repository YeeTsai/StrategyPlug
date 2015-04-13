#pragma once
#include "stdafx.h"

extern "C" {
    void __declspec(dllexport) InstallHook(BOOL, DWORD);
}
BOOL WINAPI UnInstallHook();
LRESULT CALLBACK GetMsgProC(int code, WPARAM wParam, LPARAM lParam);
void WINAPI HookOneAPI(LPCTSTR pszCalleeModuleName, PROC pfnOriginApiAddress,
    PROC pfnDummyFuncAddress, HMODULE hModCallerModule);
BOOL WINAPI HookAllAPI(LPCTSTR pszCalleeModuleName, PROC pfnOriginApiAddress,
    PROC pfnDummyFuncAddress, HMODULE hModCallerModule);
BOOL WINAPI UnhookAllAPIHooks(LPCTSTR pszCalleeModuleName, PROC pfnOriginApiAddress,
    PROC pfnDummyFuncAddress, HMODULE hModCallerModule);
BOOL WINAPI H_TextOutA(HDC, int, int, LPCSTR, int);
BOOL WINAPI H_TextOutW(HDC, int, int, LPCWSTR, int);
BOOL WINAPI H_ExtTextOutA(HDC, int, int, UINT, CONST RECT *, LPCSTR, UINT, CONST INT *);
BOOL WINAPI H_ExtTextOutW(HDC, int, int, UINT, CONST RECT *, LPCWSTR, UINT, CONST INT *);