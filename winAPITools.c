#include "winAPITools.h"
#include <strsafe.h>

//private functions and vars
static LPTSTR LastError = NULL;
static void THROW(PTSTR error);

static BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo);
//end

BOOL CreateParedAsyncPipes(HANDLE* reader, HANDLE* writer, LPTSTR name, SECURITY_ATTRIBUTES* saAttrWriter) {
    BOOL result = FALSE;
    DWORD lasterror = 0, dwMode = PIPE_READMODE_BYTE;

    OVERLAPPED fakeoverlap = {
        .hEvent = CreateEvent(
            NULL,    // default security attribute 
            TRUE,    // manual-reset event 
            FALSE,    // initial state = signaled 
            NULL   // unnamed event object
        )
    };

    // Reader part
    *reader = CreateNamedPipe(
        name,
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        WINAPITOOLS_BUFSIZE,
        WINAPITOOLS_BUFSIZE,
        0,
        NULL
    );
    if (*reader == INVALID_HANDLE_VALUE) {
        THROW(TEXT("CreateNamedPipe failed."));
        return FALSE;
    }

    result = ConnectToNewClient(*reader, &fakeoverlap);
    if (!result) 
        return FALSE;
    
    // Writer part
    *writer = CreateFile(
        name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        saAttrWriter,
        OPEN_EXISTING,
        0,
        NULL
    );
    if (*writer == INVALID_HANDLE_VALUE) {
        THROW(TEXT("CreateFile failed."));
        return FALSE;
    }
    lasterror = GetLastError();
    if (lasterror != 0) {
        THROW(TEXT("Cannot open pipe."));
        return FALSE;
    }

    result = SetNamedPipeHandleState( 
        *writer,  // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL      // don't set maximum time 
    );
    if (!result) {
        THROW(TEXT("SetNamedHandleState failed."));
        return FALSE;
    }

    if (!SetHandleInformation(*reader, HANDLE_FLAG_INHERIT, 0)) {
        THROW(TEXT("SetHandleInformation failed."));
        return FALSE;
    }

    return TRUE;
}

static BOOL ConnectToNewClient(HANDLE hPipe, LPOVERLAPPED lpo) {
    if (ConnectNamedPipe(hPipe, lpo)) {
        THROW(TEXT("ConnectNamedPipe failed."));
        return FALSE;
    }
    switch (GetLastError()) {
        case ERROR_IO_PENDING:
            return TRUE;
        case ERROR_PIPE_CONNECTED:
            return TRUE;
        default:
            THROW(TEXT("ConnectedNamedPipe failed."));
            return FALSE;
    }
}

BOOL CreateChildProcess(PTSTR cmdline, PTSTR args, HANDLE input, HANDLE output, HANDLE err, PROCESS_INFORMATION* process_info) {
    STARTUPINFO siStartInfo;
    BOOL result = FALSE;

    ZeroMemory(process_info, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

	siStartInfo.hStdOutput = output;
	siStartInfo.hStdInput = input;
    siStartInfo.hStdError = err;
	siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    result = CreateProcess(
        cmdline,       // command line 
		args,          // args
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		process_info   // receives PROCESS_INFORMATION 
    );

    if (!result) {
        THROW(TEXT("CreateProcess failed."));
        return FALSE;
    }

    CloseHandle(output);

    // CloseHandle(process_info->hProcess);
	CloseHandle(process_info->hThread);
    return TRUE;
}

int AsyncReadFromNamedPipe(HANDLE handleNamedPipe, CHAR* buf, DWORD bufRead, DWORD* actualRead, DWORD msTimeout, OVERLAPPED* overlap) {
    BOOL result = FALSE;
    result = ReadFile(handleNamedPipe, buf, bufRead, actualRead, msTimeout ? overlap : NULL);
    if (result) {
        return 0;
    }
    DWORD lasterror = GetLastError();

    switch (lasterror) {
        case ERROR_BROKEN_PIPE:
            return 2;
        case ERROR_IO_PENDING:
            break;
        case ERROR_MORE_DATA:
            *actualRead = bufRead;
            return 0;
        default:
            THROW(TEXT("ReadFile not pending, but not result."));
            return -1;
    }

    // if (!result && lasterror == ERROR_BROKEN_PIPE) {
    //     return 2;
    // }

    // if (!result && lasterror != ERROR_IO_PENDING) {
    //     THROW(TEXT("ReadFile not pending, but not result."));
    //     return -1;
    // }
    

    BOOL timeout = FALSE;
    switch(WaitForSingleObject(overlap->hEvent, msTimeout)) {
        default:
            THROW(TEXT("WaitForSingleObject failed."));
            return -1;
        case WAIT_OBJECT_0: //event happened. Data recieved
            break;
        case WAIT_TIMEOUT:
            timeout = TRUE;
            CancelIoEx(handleNamedPipe, overlap);
            break;
    }

    //timeout happened
    result = GetOverlappedResult(
        handleNamedPipe,
        overlap,
        actualRead,
        timeout
    );
    if (result) {
        return 0;
    }
    lasterror = GetLastError();
    if (!result && *actualRead != 0 && lasterror == ERROR_IO_INCOMPLETE) {
        if (!ReadFile(handleNamedPipe, buf, *actualRead, 0, overlap)) {
            THROW(TEXT("Cannot read have received data after timeout."));
            return -1;
        }
        return 1;
    }
    if (!result && lasterror == ERROR_BROKEN_PIPE) {
        return 2;
    }
    return 1;
}

// Error proceeding

LPTSTR WinAPITools_GetLastError(void) {
    return LastError;
}

// shit from windows example
static void THROW(PTSTR lpszFunction) {
    LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

    if (LastError) 
        LocalFree(LastError);

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL 
    );
	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
    
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf
    );
    LocalFree(lpMsgBuf);

    LastError = (LPTSTR) lpDisplayBuf;
}