#define _WIN32_WINNT        0x0400
#define WIN32
#define NT

#define DEBUG   0

#include <windows.h>
#include <stdio.h>
#include "detours.h"

#define PULONG_PTR          PVOID
#define PLONG_PTR           PVOID
#define ULONG_PTR           PVOID
#define ENUMRESNAMEPROCA    PVOID
#define ENUMRESNAMEPROCW    PVOID
#define ENUMRESLANGPROCA    PVOID
#define ENUMRESLANGPROCW    PVOID
#define ENUMRESTYPEPROCA    PVOID
#define ENUMRESTYPEPROCW    PVOID
#define STGOPTIONS          PVOID

static HMODULE s_hInst = NULL;
static CHAR s_szDllPath[MAX_PATH];

extern "C" {

    HANDLE (WINAPI *
            Real_CreateFileW)(LPCWSTR a0,
                              DWORD a1,
                              DWORD a2,
                              LPSECURITY_ATTRIBUTES a3,
                              DWORD a4,
                              DWORD a5,
                              HANDLE a6)
        = CreateFileW;

    BOOL (WINAPI *
          Real_WriteFile)(HANDLE hFile,
                          LPCVOID lpBuffer,
                          DWORD nNumberOfBytesToWrite,
                          LPDWORD lpNumberOfBytesWritten,
                          LPOVERLAPPED lpOverlapped)
        = WriteFile;
    BOOL (WINAPI *
          Real_FlushFileBuffers)(HANDLE hFile)
        = FlushFileBuffers;
    BOOL (WINAPI *
          Real_CloseHandle)(HANDLE hObject)
        = CloseHandle;

    BOOL (WINAPI *
          Real_WaitNamedPipeW)(LPCWSTR lpNamedPipeName, DWORD nTimeOut)
        = WaitNamedPipeW;
    BOOL (WINAPI *
          Real_SetNamedPipeHandleState)(HANDLE hNamedPipe,
                                        LPDWORD lpMode,
                                        LPDWORD lpMaxCollectionCount,
                                        LPDWORD lpCollectDataTimeout)
        = SetNamedPipeHandleState;

    DWORD (WINAPI *
           Real_GetCurrentProcessId)(VOID)
        = GetCurrentProcessId;
    VOID (WINAPI *
          Real_GetSystemTimeAsFileTime)(LPFILETIME lpSystemTimeAsFileTime)
        = GetSystemTimeAsFileTime;

    VOID (WINAPI *
          Real_InitializeCriticalSection)(LPCRITICAL_SECTION lpSection)
        = InitializeCriticalSection;
    VOID (WINAPI *
          Real_EnterCriticalSection)(LPCRITICAL_SECTION lpSection)
        = EnterCriticalSection;
    VOID (WINAPI *
          Real_LeaveCriticalSection)(LPCRITICAL_SECTION lpSection)
        = LeaveCriticalSection;
}

DWORD (WINAPI * Real_GetModuleFileNameW)(HMODULE a0,
                                         LPWSTR a1,
                                         DWORD a2)
    = GetModuleFileNameW;

DWORD (WINAPI * Real_GetModuleFileNameA)(HMODULE a0,
                                         LPSTR a1,
                                         DWORD a2)
    = GetModuleFileNameA;

BOOL (WINAPI * Real_CreateProcessW)(LPCWSTR a0,
                                    LPWSTR a1,
                                    LPSECURITY_ATTRIBUTES a2,
                                    LPSECURITY_ATTRIBUTES a3,
                                    BOOL a4,
                                    DWORD a5,
                                    LPVOID a6,
                                    LPCWSTR a7,
                                    struct _STARTUPINFOW* a8,
                                    LPPROCESS_INFORMATION a9)
    = CreateProcessW;

static SHORT (WINAPI * Real_GetKeyState)(int nVirtKey) = GetKeyState;

SHORT WINAPI Mine_GetKeyState(int nVirtKey)
{
    SHORT ret;
    ret = Real_GetKeyState(nVirtKey);
    #ifdef DEBUG
    printf("getkeystate(%d) - detoured - result 0x%04x\r\n", nVirtKey, ret);
    #endif
    if (ret & ~0x8001) {
        #ifdef DEBUG
        printf("bad flags are set in GetKeyState(%d): 0x%04x, fixing them for you to save you!!!111111111111111111111\r\n", nVirtKey, ret);
        #endif
        ret &= 0x8001;
    }
    return ret;
}

static BOOL (WINAPI *Real_GetKeyboardState)(PBYTE lpKeyState) = GetKeyboardState;

BOOL WINAPI Mine_GetKeyboardState(PBYTE lpKeyState)
{
    BOOL ret;
    int i;
    ret = Real_GetKeyboardState(lpKeyState);
    #ifdef DEBUG
    printf("GetKeyboardState() - detoured\r\n");
    #endif
    if (lpKeyState == NULL) {
        #ifdef DEBUG
        printf("GetKeyboardState() - buffer is nullptr\r\n");
        #endif
        return ret;
    }
    if (ret == FALSE) {
        #ifdef DEBUG
        printf("GetKeyboardState() - failed\r\n");
        #endif
        return ret;
    }

    for (i = 255; i >= 0; i--) {
        if (lpKeyState[i] & ~0x81) {
            #ifdef DEBUG
            printf("GetKeyboardState(): bad flag in lpKeyState[%d]: 0x%02x, fixing it for you to save you!!1111111\r\n", i, lpKeyState[i] & 0xff);
            #endif
            lpKeyState[i] &= 0x81;
        }
    }
    return ret;
}

BOOL WINAPI Mine_CreateProcessW(LPCWSTR lpApplicationName,
                                LPWSTR lpCommandLine,
                                LPSECURITY_ATTRIBUTES lpProcessAttributes,
                                LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                BOOL bInheritHandles,
                                DWORD dwCreationFlags,
                                LPVOID lpEnvironment,
                                LPCWSTR lpCurrentDirectory,
                                LPSTARTUPINFOW lpStartupInfo,
                                LPPROCESS_INFORMATION lpProcessInformation)
{
    BOOL rv = 0;
    rv = DetourCreateProcessWithDllExW(lpApplicationName,
                                       lpCommandLine,
                                       lpProcessAttributes,
                                       lpThreadAttributes,
                                       bInheritHandles,
                                       dwCreationFlags,
                                       lpEnvironment,
                                       lpCurrentDirectory,
                                       lpStartupInfo,
                                       lpProcessInformation,
                                       s_szDllPath,
                                       Real_CreateProcessW);
    return rv;
}

#define ATTACH(x)       DetourAttach(&(PVOID&)Real_##x,Mine_##x)

LONG AttachDetours(VOID)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    printf("attaching detours [%lld]\n", (long long int) GetCurrentThread());

    ATTACH(CreateProcessW);

    ATTACH(GetKeyboardState);
    ATTACH(GetKeyState);

    return DetourTransactionCommit();
}

#define DETACH(x)       DetourDetach(&(PVOID&)Real_##x,Mine_##x)

LONG DetachDetours(VOID)
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    printf("detaching detours [%lld]\n", (long long int) GetCurrentThread());

    DETACH(CreateProcessW);

    DETACH(GetKeyboardState);
    DETACH(GetKeyState);

    return DetourTransactionCommit();
}

BOOL ThreadAttach(HMODULE hDll)
{
    (void)hDll;
    return TRUE;
}

BOOL ThreadDetach(HMODULE hDll)
{
    (void)hDll;
    return TRUE;
}

BOOL ProcessAttach(HMODULE hDll)
{
    WCHAR wzExePath[MAX_PATH];

    s_hInst = hDll;
    Real_GetModuleFileNameA(s_hInst, s_szDllPath, ARRAYSIZE(s_szDllPath));
    Real_GetModuleFileNameW(NULL, wzExePath, ARRAYSIZE(wzExePath));

    LONG error = AttachDetours();
    (void) error;
    ThreadAttach(hDll);

    return TRUE;
}

BOOL ProcessDetach(HMODULE hDll)
{
    ThreadDetach(hDll);

    LONG error = DetachDetours();
    (void) error;
    return TRUE;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, PVOID lpReserved)
{
    (void)hModule;
    (void)lpReserved;

    if (DetourIsHelperProcess()) {
        return TRUE;
    }

    switch (dwReason) {
      case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();
        return ProcessAttach(hModule);
      case DLL_PROCESS_DETACH:
        return ProcessDetach(hModule);
      case DLL_THREAD_ATTACH:
        return ThreadAttach(hModule);
      case DLL_THREAD_DETACH:
        return ThreadDetach(hModule);
    }
    return TRUE;
}