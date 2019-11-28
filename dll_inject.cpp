#include <stdio.h>
#include <windows.h>
#include <detours.h>
#include <strsafe.h>

//#pragma warning(push)
//#if _MSC_VER > 1400
//#pragma warning(disable:6102 6103) // /analyze warnings
//#endif
//#pragma warning(pop)

//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
struct ExportContext
{
    BOOL    fHasOrdinal1;
    ULONG   nExports;
};

static BOOL CALLBACK ExportCallback(_In_opt_ PVOID pContext,
                                    _In_ ULONG nOrdinal,
                                    _In_opt_ LPCSTR pszSymbol,
                                    _In_opt_ PVOID pbTarget)
{
    (void)pContext;
    (void)pbTarget;
    (void)pszSymbol;

    ExportContext *pec = (ExportContext *)pContext;

    if (nOrdinal == 1) {
        pec->fHasOrdinal1 = TRUE;
    }
    pec->nExports++;

    return TRUE;
}

int CDECL main(int argc, char **argv)
{
    // find the name of the ini file from the name of our executable
    char file_name[4096];
    GetModuleFileNameA(NULL, file_name, sizeof(file_name) - 4);

    // search for the point where we need to add the '.ini' to the string
    size_t pos = strlen(file_name);

    if (pos == 0) {
        printf("could not determine file name of executable\r\n");
        return 1;
    } else {
        // search for the dot
        while (file_name[pos] != '.') {
            if (--pos == 0) {
                // filename contains no dot, wtf fu
                printf("name of executable contains no dot\r\n");
            }
        }
    }
    file_name[pos + 1] = 'i';
    file_name[pos + 2] = 'n';
    file_name[pos + 3] = 'i';
    file_name[pos + 4] = '\0';

    // file_name now contains path to the .ini file
    printf("ini file               %s\r\n", file_name);

    // load the name of the exe file from the ini file
    char exe_name[4096];
    exe_name[0] = '\0';

    GetPrivateProfileStringA(
        "dll_inject",
        "exe_file",
        "",
        exe_name,
        sizeof(exe_name),
        file_name
    );

    if (exe_name[0] == '\0') {
        printf("could not read %s section [dll_inject] key exe_file\r\n", file_name);
        return 1;
    } else {
        printf("exe file               %s\r\n", exe_name);
    }

    // load the name of the dll file from the ini file
    char dll_file[4096];
    dll_file[0] = '\0';

    GetPrivateProfileStringA(
        "dll_inject",
        "dll_file",
        "",
        dll_file,
        sizeof(dll_file),
        file_name
    );

    if (dll_file[0] == '\0') {
        printf("could not read %s section [dll_inject] key dll_file\r\n", file_name);
        return 1;
    } else {
        printf("dll file               %s\r\n", dll_file);
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    CHAR command_string[4096];

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    command_string[0] = L'\0';

    argv[0] = exe_name;

    for (int arg = 0; arg < argc; arg++) {
        if (strchr(argv[arg], ' ') != NULL || strchr(argv[arg], '\t') != NULL) {
            StringCchCatA(command_string, sizeof(command_string), "\"");
            StringCchCatA(command_string, sizeof(command_string), argv[arg]);
            StringCchCatA(command_string, sizeof(command_string), "\"");
        }
        else {
            StringCchCatA(command_string, sizeof(command_string), argv[arg]);
        }

        if (arg + 1 < argc) {
            StringCchCatA(command_string, sizeof(command_string), " ");
        }
    }
    printf("start command          %s\r\n", command_string);
    DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    // save the current directory name
    char current_directory[4096];
    GetCurrentDirectoryA(sizeof(current_directory), current_directory);
    printf("start cwd              %s\r\n", current_directory);

    // remove the filename component from the path to our executable
    pos = strlen(file_name);
    if (pos == 0) {
        printf("could not determine file name of executable\r\n");
        return 1;
    }
    
    // search for the backslash
    while (file_name[pos] != '\\') {
        if (--pos == 0) {
            // filename contains no backslash
            printf("path to executable contains no backslash\r\n");
            return 1;
        }
    }
    // cut the string at the backslash
    file_name[pos] = '\0';

    // cd to the directory of our exe file
    if (!SetCurrentDirectoryA(file_name)) {
        printf("could not cd to %s\r\n", file_name);
    }

    SetLastError(0);
    char exe_name_full_path[4096];
    exe_name_full_path[0] = '\0';
    char *file_part = NULL;
    SearchPathA(
        NULL,
        exe_name,
        ".exe",
        sizeof(exe_name_full_path),
        exe_name_full_path,
        &file_part
    );

    // get the full path of the dll file
    char dll_file_full_path[4096];

    if (!GetFullPathNameA(dll_file, sizeof(dll_file_full_path), dll_file_full_path, NULL)) {
        printf("could not resolve full path of dll file\r\n");
        return 1;
    } else {
        printf("dll file [full path]   %s\r\n", dll_file_full_path);
    }

    HMODULE hDll = LoadLibraryExA(dll_file_full_path, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hDll == NULL) {
        printf("could not load dll file: %d\r\n", GetLastError());
        return 1;
    }

    ExportContext ec;
    ec.fHasOrdinal1 = FALSE;
    ec.nExports = 0;
    DetourEnumerateExports(hDll, &ec, ExportCallback);
    FreeLibrary(hDll);

    if (!ec.fHasOrdinal1) {
        printf("dll file does not export ordinal #1\r\n");
        return 1;
    }

    if (exe_name_full_path[0] == '\0') {
        printf("could not resolve full path of exe name %s\r\n", exe_name);
        return 1;
    }
    printf("exe file [full path]   %s\r\n", exe_name_full_path);

    const char *dll_file_full_path_ptr = (const char *) dll_file_full_path;

    if (!DetourCreateProcessWithDllsA(
        exe_name_full_path,
        command_string,
        NULL,
        NULL,
        TRUE,
        dwFlags,
        NULL,
        current_directory,
        &si,
        &pi,
        1,                       // one dll file
        &dll_file_full_path_ptr,
        NULL
    )) {
        DWORD dwError = GetLastError();
        printf("DetourCreateProcessWithDllEx failed: %d\r\n", dwError);
        if (dwError == ERROR_INVALID_HANDLE) {
#if DETOURS_64BIT
            printf("withdll.exe: Can't detour a 32-bit target process from a 64-bit parent process.\r\n");
#else
            printf("withdll.exe: Can't detour a 64-bit target process from a 32-bit parent process.\r\n");
#endif
        }
        return 1;
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD child_process_result = 0;
    if (!GetExitCodeProcess(pi.hProcess, &child_process_result)) {
        printf("GetExitCodeProcess failed: %d\r\n", GetLastError());
        return 1;
    }

    return child_process_result;
}