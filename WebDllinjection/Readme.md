#### Dll Injection To a Remote Process 

- Dll is stored in a local web server ( remote web server can definetely be used )
- local web server command is 
```bash
    python -m http.server 8080
```
- functions is getDllandSave.h fetches the Dll from the server and saves it in memory and then from memory address the dll is saved into disk location `C:/Users/Public/inject.dll`

- Remote Process ID and Handle to the Process is got through GetRemoteProcessID (Self Implemented Function)

- This dll is fetched and then Loaded into the Remote Process via CreateRemoteThread and LoadLibraryW 

I guess there was no need for loading dll into memory rather it could directly be saved to disk
but this can be used for reference when shellcode is used because for shellcode there is no need to save in disk , just place it in memory

