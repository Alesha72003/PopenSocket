#include "PopenSocket.h"
#include "winAPITools.h"

static void ErrorExit_WinAPITools() {
    LPTSTR error = WinAPITools_GetLastError();
    PyErr_SetObject(PyExc_WindowsError, PyUnicode_FromWideChar(error, -1));
}

static void ErrorExit_Python(const char* error) {
    PyErr_SetString(PyExc_RuntimeError, error);
}

static void PopenSocket_dealloc(PopenSocket_Object* self) {
    CloseHandle(self->handleRead);
    CloseHandle(self->handleWrite);
    Py_DECREF(self->SocketException);
    Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject* PopenSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds) {
    PopenSocket_Object* self;
    self = (PopenSocket_Object*) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->handleRead = 0;
        self->timeout = 0;
        self->handleWrite = 0;
        self->closed = FALSE;
        self->overlap.hEvent = CreateEvent(
            NULL,    // default security attribute 
            TRUE,    // manual-reset event 
            FALSE,   // initial state = signaled 
            NULL     // unnamed event object
        );
        PyObject* socket = PyImport_ImportModule("socket");
        if (socket == NULL) {
            return NULL;
        }
        self->SocketException = PyObject_GetAttrString(socket, "timeout");
        if (self->SocketException == NULL) {
            return NULL;
        }
        Py_DECREF(socket);
    }
    return (PyObject*) self;
}

static int PopenSocket_init(PopenSocket_Object* self, PyObject* args, PyObject* kwds) {
    PyObject* pycmdline, *pyargs;
    LPTSTR cmdline, argstext, exefile, argsformatted;
    wchar_t pipename[28 + 2*8+1]; // \\.\pipe\popensocket-r-
    // wchar_t pipename2[28 + 2*8+1]; // \\.\pipe\popensocket-w-

    
    if (!PyArg_ParseTuple(args, "UU", &pycmdline, &pyargs))
        return -1;
    if (pycmdline == NULL) {
        return -1;
    }
    cmdline = PyUnicode_AsWideCharString(pycmdline, NULL);
    //TODO: Empty args
    argstext = PyUnicode_AsWideCharString(pyargs, NULL);

    exefile = cmdline;
    for (;*exefile;exefile++) {}
    for (;*exefile != '\\' && exefile + 1 != cmdline;exefile--) {}
    ++exefile;

    argsformatted = (LPTSTR)LocalAlloc(
        LMEM_ZEROINIT,
        (lstrlen(exefile)+1+lstrlen(argstext)+1)*sizeof(TCHAR)
    );
    swprintf(argsformatted, LocalSize(argsformatted) / sizeof(TCHAR), TEXT("%s %s"), exefile, argstext);

    swprintf(pipename, 28 + 2*8 + 1, TEXT("\\\\.\\pipe\\popensocket-r-%X"), self);
    // swprintf(pipename2, 28 + 2*8 + 1, TEXT("\\\\.\\pipe\\popensocket-w-%X"), self);
    // MessageBox(NULL, (LPCTSTR)argsformatted, TEXT("Error"), MB_OK); 
    HANDLE g_hChildStd_OUT_Rd, g_hChildStd_OUT_Wr, g_hChildStd_IN_Rd, g_hChildStd_IN_Wr;
    SECURITY_ATTRIBUTES saAttr = {
        .nLength = sizeof(SECURITY_ATTRIBUTES),
        .lpSecurityDescriptor = NULL,
        .bInheritHandle = TRUE // need to make inheritable writer handle
    };
    OVERLAPPED overlap = {
        .hEvent =  CreateEvent(
            NULL,    // default security attribute 
            TRUE,    // manual-reset event 
            FALSE,   // initial state = signaled 
            NULL     // unnamed event object
        )
    };
    BOOL result;
    result = CreateParedAsyncPipes(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, pipename, &saAttr);
    if (!result) {
        ErrorExit_WinAPITools();
        return -1;
    }
    if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
        ErrorExit_Python("Error in create anonymous pipe");
		return -1;
    }
    // if (!CreateParedAsyncPipes(&g_hChildStd_IN_Wr, &g_hChildStd_IN_Rd, pipename2, &saAttr)) {
    //     ErrorExit_WinAPITools();
    // }
    if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        ErrorExit_Python("Error in sethandleinfrormation");
		return -1;
    }

    result = CreateChildProcess(cmdline, argsformatted, g_hChildStd_IN_Rd, g_hChildStd_OUT_Wr, GetStdHandle(STD_ERROR_HANDLE), &self->proc);
    if (!result) {
        ErrorExit_WinAPITools();
        return -1;
    }

    self->handleRead = g_hChildStd_OUT_Rd;
    self->handleWrite = g_hChildStd_IN_Wr;
    return 0;
}

static PyObject* PopenSocket_send(PopenSocket_Object* self, PyObject* arg) {
    const char* data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(arg, "y#", &data, &size)) {
        return NULL;
    }

    if (self->closed) {
        PyErr_SetNone(PyExc_ConnectionAbortedError);
        return NULL;
    }

    DWORD writen = 0;
    if (!WriteFile(self->handleWrite, data, size, &writen, NULL)) {
        ErrorExit_Python("Error in write to child. May be broken pipe");
        return NULL;
    }
    
    return Py_BuildValue("l", writen);
}

static PyObject* PopenSocket_recv(PopenSocket_Object* self, PyObject* arg) {
    long int sizeread;
    if (!PyArg_ParseTuple(arg, "l", &sizeread)) {
        return NULL;
    }
    if (sizeread < 0) {
        ErrorExit_Python("Read size must be > 0");
        return NULL;
    }
    char* buffer = LocalAlloc(LMEM_ZEROINIT, sizeread);
    DWORD readed = 0;
    int result = -1;
    result = AsyncReadFromNamedPipe(self->handleRead, buffer, sizeread, &readed, self->timeout, &self->overlap);
    if (result == -1) {
        ErrorExit_WinAPITools();
        LocalFree(buffer);
        return NULL;
    }
    if (result == 1 && readed == 0) {
        PyErr_SetNone(self->SocketException);
        LocalFree(buffer);
        return NULL;
    }
    if (result == 2) {
        if (!self->closed) {
            self->closed = TRUE;
        } else {
            PyErr_SetNone(PyExc_ConnectionAbortedError);
            LocalFree(buffer);
            return NULL;
        }
    }
    // printf("BUFFER: %s; result: %d\n", buffer, result);
    PyObject* ret = PyBytes_FromStringAndSize(buffer, readed);
    LocalFree(buffer);
    return ret;
}

static PyObject* PopenSocket_close(PopenSocket_Object* self, PyObject* Py_UNUSED(ignored)) {
    if (self->proc.dwProcessId) {
        TerminateProcess(self->proc.hProcess, 0);
        self->proc.dwProcessId = 0;
    } else {
        ErrorExit_Python("Process alredy killed");
        return NULL;
    }
    self->closed = TRUE;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* PopenSocket_settimeout(PopenSocket_Object* self, PyObject* arg) {
    float timeout = 0;
    if (!PyArg_ParseTuple(arg, "f", &timeout)) {
        return NULL;
    }
    if (timeout < 0) {
        ErrorExit_Python("Timeout must be > 0");
        return NULL;
    }
    self->timeout = (unsigned long int)(timeout * 1000);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* PopenSocket_getclosed(PopenSocket_Object* self, void* closure) {
    if (self->closed) {
        Py_INCREF(Py_True);
        return Py_True;
    }
    Py_INCREF(Py_False);
    return Py_False;
}

static int PopenSocket_setclosed(PopenSocket_Object* self, PyObject* value, void* closure) {
    ErrorExit_Python("_closed cannot be setted");
    return -1;
}

PyMODINIT_FUNC PyInit_PopenSocket(void) {
    PyObject* m;
    if (PyType_Ready(&PopenSocket_Type) < 0) {
        return NULL;
    }
    m = PyModule_Create(&PopenSocket_Module);
    if (m == NULL)
        return NULL;
    
    Py_INCREF(&PopenSocket_Type);
    if (PyModule_AddObject(m, "PopenSocket", (PyObject*) &PopenSocket_Type) < 0) {
        Py_DECREF(&PopenSocket_Type);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}