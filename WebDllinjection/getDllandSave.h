#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")


BOOL GetDllFromUrl(const wchar_t* url, BYTE** pBytes, size_t* dllSize) {
    HINTERNET hInternet = NULL, hFileW = NULL;
    DWORD dwBytesRead = 0, dwTotalSize = 0;

    // Open an internet session
    hInternet = InternetOpenW(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        printf("[!] InternetOpenW Failed: %d\n", GetLastError());
        return FALSE;
    }

    // Open the URL
    hFileW = InternetOpenUrlW(hInternet, url, NULL, 0, INTERNET_FLAG_HYPERLINK | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID, 0);
    if (hFileW == NULL) {
        printf("[!] InternetOpenUrlW Failed: %d\n", GetLastError());
        InternetCloseHandle(hInternet);
        return FALSE;
    }

    // Allocate initial memory
    *pBytes = (BYTE*)LocalAlloc(LPTR, 4096);
    if (*pBytes == NULL) {
        printf("[!] Memory allocation failed.\n");
        InternetCloseHandle(hFileW);
        InternetCloseHandle(hInternet);
        return FALSE;
    }

    // Read DLL data
    while (InternetReadFile(hFileW, *pBytes + dwTotalSize, 4096, &dwBytesRead) && dwBytesRead > 0) {
        dwTotalSize += dwBytesRead;
        BYTE* temp = (BYTE*)LocalReAlloc(*pBytes, dwTotalSize + 4096, LMEM_MOVEABLE);
        if (temp == NULL) {
            printf("[!] Memory reallocation failed.\n");
            LocalFree(*pBytes);
            InternetCloseHandle(hFileW);
            InternetCloseHandle(hInternet);
            return FALSE;
        }
        *pBytes = temp;
    }

    *dllSize = dwTotalSize;

    // Clean up
    InternetCloseHandle(hFileW);
    InternetCloseHandle(hInternet);
    InternetSetOptionW(NULL, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);

    return TRUE;
}
BOOL SaveDllToFile(const char* filePath, BYTE* dllBytes, size_t dllSize) {
    FILE* file;
    if (fopen_s(&file, filePath, "wb") != 0) {
        return FALSE;
    }
    fwrite(dllBytes, 1, dllSize, file);
    fclose(file);
    return TRUE;
}
