#include <Windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <stdio.h>

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

BOOL InjectDllToRemoteProcess(HANDLE hProcess, TCHAR* DllName) {
    if (hProcess == NULL) return FALSE;

    LPTHREAD_START_ROUTINE pLoadLibraryW = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
    if (pLoadLibraryW == NULL) return FALSE;

    SIZE_T dwSizeToWrite = (_tcslen(DllName) + 1) * sizeof(TCHAR);
    LPVOID pAddress = VirtualAllocEx(hProcess, NULL, dwSizeToWrite, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pAddress == NULL) return FALSE;

    SIZE_T lpNumberOfBytesWritten = 0;
    if (!WriteProcessMemory(hProcess, pAddress, DllName, dwSizeToWrite, &lpNumberOfBytesWritten)) {
        VirtualFreeEx(hProcess, pAddress, 0, MEM_RELEASE);
        return FALSE;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibraryW, pAddress, 0, NULL);
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
    if (argc < 3) return -1;

    HANDLE hProcess = NULL;
    DWORD PID = 0;

#ifdef UNICODE
    wchar_t DllName[MAX_PATH];
    wchar_t szProcessName[MAX_PATH];
    mbstowcs_s(NULL, DllName, MAX_PATH, argv[1], _TRUNCATE); // convert Dllname from char* to wchar_t*
    mbstowcs_s(NULL, szProcessName, MAX_PATH, argv[2], _TRUNCATE); // szProcessName char* to wchar_t*
#else
    char* DllName = argv[1];
    char* szProcessName = argv[2];
#endif

    if (!GetRemoteProcessID(szProcessName, &PID, &hProcess)) return -1;
    if (!InjectDllToRemoteProcess(hProcess, DllName)) return -1;

    CloseHandle(hProcess);
    return 0;
}
