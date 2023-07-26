#ifndef WINAPITOOLS_H
#define WINAPITOOLS_H

#define UNICODE
#include <windows.h>

#ifndef WINAPITOOLS_BUFSIZE
#define WINAPITOOLS_BUFSIZE 4*1024*1024
#endif //WINAPITOOLS_BUFSIZE 4MB default

BOOL CreateParedAsyncPipes(HANDLE* reader, HANDLE* writer, LPTSTR name, SECURITY_ATTRIBUTES* saAttrWriter);
BOOL CreateChildProcess(PTSTR cmdline, PTSTR args, HANDLE input, HANDLE output, HANDLE err, PROCESS_INFORMATION* process_info);
int AsyncReadFromNamedPipe(HANDLE handleNamedPipe, CHAR* buf, DWORD bufRead, DWORD* actualRead, DWORD msTimeout, OVERLAPPED* overlap);

LPTSTR WinAPITools_GetLastError(void);

#endif //WINAPITOOLS_H