// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "TrailHook.h"
#include <ImageHlp.h>   
#include <tlhelp32.h>   
#include <stdio.h>
#pragma comment(lib,"ImageHlp")

#pragma data_seg("Shared")     
HMODULE hmodDll = NULL;
HHOOK hHook = NULL;
#pragma data_seg()     
#pragma comment(linker,"/Section:Shared,rws") //设置全局共享数据段的属性     

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
        UnInstallHook();
		break;
	}

    hmodDll = hModule;

	return TRUE;
}

///////////////////////////////////// HookOneAPI 函数 /////////////////////////////////////////   
//进行IAT转换的关键函数，其参数含义：   
//pszCalleeModuleName：需要hook的模块名   
//pfnOriginApiAddress：要替换的自己API函数的地址   
//pfnDummyFuncAddress：需要hook的模块名的地址   
//hModCallerModule：我们要查找的模块名称，如果没有被赋值，   
//     将会被赋值为枚举的程序所有调用的模块   
void WINAPI HookOneAPI(LPCTSTR pszCalleeModuleName, PROC pfnOriginApiAddress,
    PROC pfnDummyFuncAddress, HMODULE hModCallerModule) {
    ULONG size;
    //获取指向PE文件中的Import中IMAGE_DIRECTORY_DESCRIPTOR数组的指针   
    PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)
        ImageDirectoryEntryToData(hModCallerModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size);
    if (pImportDesc == NULL)
        return;
    //查找记录,看看有没有我们想要的DLL   
    for (; pImportDesc->Name; pImportDesc++) {
        LPSTR pszDllName = (LPSTR)((PBYTE)hModCallerModule + pImportDesc->Name);
        if (lstrcmpiA(pszDllName, pszCalleeModuleName) == 0)
            break;
    }
    if (pImportDesc->Name == NULL) {
        return;
    }
    //寻找我们想要的函数   
    PIMAGE_THUNK_DATA pThunk =
        (PIMAGE_THUNK_DATA)((PBYTE)hModCallerModule + pImportDesc->FirstThunk);//IAT   
    for (; pThunk->u1.Function; pThunk++) {
        //ppfn记录了与IAT表项相应的函数的地址   
        PROC * ppfn = (PROC *)&pThunk->u1.Function;
        if (*ppfn == pfnOriginApiAddress) {
            //如果地址相同，也就是找到了我们想要的函数，进行改写，将其指向我们所定义的函数   
            WriteProcessMemory(GetCurrentProcess(), ppfn, &(pfnDummyFuncAddress),
                sizeof(pfnDummyFuncAddress), NULL);
            return;
        }
    }
}
//查找所挂钩的进程所应用的dll模块的   
BOOL WINAPI HookAllAPI(LPCTSTR pszCalleeModuleName, PROC pfnOriginApiAddress,
    PROC pfnDummyFuncAddress, HMODULE hModCallerModule) {
    if (pszCalleeModuleName == NULL) {
        return FALSE;
    }
    if (pfnOriginApiAddress == NULL) {
        return FALSE;
    }
    //如果没传进来要挂钩的模块名称，枚举被挂钩进程的所有引用的模块，   
    //并对这些模块进行传进来的相应函数名称的查找   

    if (hModCallerModule == NULL) {
        MEMORY_BASIC_INFORMATION mInfo;
        HMODULE hModHookDLL;
        HANDLE hSnapshot;
        MODULEENTRY32 me = { sizeof(MODULEENTRY32) };
        //MODULEENTRY32:描述了一个被指定进程所应用的模块的struct   
        VirtualQuery(HookOneAPI, &mInfo, sizeof(mInfo));
        hModHookDLL = (HMODULE)mInfo.AllocationBase;

        hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
        BOOL bOk = Module32First(hSnapshot, &me);
        while (bOk) {
            if (me.hModule != hModHookDLL) {
                hModCallerModule = me.hModule;//赋值   
                //me.hModule:指向当前被挂钩进程的每一个模块    
                HookOneAPI(pszCalleeModuleName, pfnOriginApiAddress,
                    pfnDummyFuncAddress, hModCallerModule);
            }
            bOk = Module32Next(hSnapshot, &me);
        }
        return TRUE;
    }
    //如果传进来了，进行查找   
    else {
        HookOneAPI(pszCalleeModuleName, pfnOriginApiAddress,
            pfnDummyFuncAddress, hModCallerModule);
        return TRUE;
    }
    return FALSE;
}
//////////////////////////////////// UnhookAllAPIHooks 函数 /////////////////////////////////////   
//通过使pfnDummyFuncAddress与pfnOriginApiAddress相等的方法，取消对IAT的修改   
BOOL WINAPI UnhookAllAPIHooks(LPCTSTR pszCalleeModuleName, PROC pfnOriginApiAddress,
    PROC pfnDummyFuncAddress, HMODULE hModCallerModule) {
    PROC temp;
    temp = pfnOriginApiAddress;
    pfnOriginApiAddress = pfnDummyFuncAddress;
    pfnDummyFuncAddress = temp;
    return HookAllAPI(pszCalleeModuleName, pfnOriginApiAddress,
        pfnDummyFuncAddress, hModCallerModule);
}
////////////////////////////////// GetMsgProc 函数 ////////////////////////////////////////   
//钩子子程。与其它钩子子程不大相同，没做什么有意义的事情，继续调用下一个钩子子程，形成循环   
LRESULT CALLBACK GetMsgProc(int code, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(hHook, code, wParam, lParam);
}
//////////////////////////////////// InstallHook 函数 /////////////////////////////////////   
//安装或卸载钩子，BOOL IsHook参数是标志位   
//对要钩哪个API函数进行初始化   
//我们这里装的钩子类型是WH_GETMESSAGE   
extern "C" {
void __declspec(dllexport) InstallHook(BOOL IsHook, DWORD dwThreadId) {
    if (IsHook) {
        hHook = SetWindowsHookEx(WH_GETMESSAGE, (HOOKPROC)GetMsgProc, hmodDll, dwThreadId);

        //GetProcAddress(GetModuleHandle("GDI32.dll"),"ExtTextOutA")：取得要钩的函数在所在dll中的地址   

        BOOL ok = HookAllAPI("GDI32.dll", GetProcAddress(GetModuleHandle("GDI32.dll"),
            "TextOutW"), (PROC)&H_TextOutW, NULL);
        ok = HookAllAPI("GDI32.dll", GetProcAddress(GetModuleHandle("GDI32.dll"),
            "TextOutA"), (PROC)&H_TextOutA, NULL);
        fprintf(stdout, "InstallHook OK!\n");
    } else {
        UnInstallHook();
        UnhookAllAPIHooks("GDI32.dll", GetProcAddress(GetModuleHandle("GDI32.dll"),
            "TextOutW"), (PROC)&H_TextOutW, NULL);
        UnhookAllAPIHooks("GDI32.dll", GetProcAddress(GetModuleHandle("GDI32.dll"),
            "TextOutA"), (PROC)&H_TextOutA, NULL);
    }
}
}
///////////////////////////////////// UnInstallHook 函数 ////////////////////////////////////   
//卸载钩子   
BOOL WINAPI UnInstallHook() {
    UnhookWindowsHookEx(hHook);
    return TRUE;
}
///////////////////////////////////// H_TextOutA 函数 /////////////////////////////////////////   
//我们的替换函数，可以在里面实现我们所要做的功能   
//这里我做的是显示一个对话框，指明是替换了哪个函数   
BOOL WINAPI H_TextOutA(HDC hdc, int nXStart, int nYStart, LPCSTR lpString, int cbString) {
    //  FILE *stream=fopen("logfile.txt","a+t");   
    //MessageBox(NULL, "TextOutA", "APIHook_Dll ---rivershan", MB_OK);
    fprintf(stdout, "TextOutA x %d y %d output %s\n", nXStart, nYStart, lpString);
    TextOutA(hdc, nXStart, nYStart, lpString, cbString);//返回原来的函数，以显示字符   
    // fprintf(stream,lpString);   
    // fclose(stream);   
    return TRUE;
}
///////////////////////////////////// H_TextOutW 函数 /////////////////////////////////////////   
//同上   
BOOL WINAPI H_TextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString) {
    //MessageBox(NULL, "TextOutW", "APIHook_Dll ---rivershan", MB_OK);
    fprintf(stdout, "TextOutW x %d y %d output %s\n", nXStart, nYStart, lpString);
    TextOutW(hdc, nXStart, nYStart, lpString, cbString);//返回原来的函数，以显示字符   
    return TRUE;
}