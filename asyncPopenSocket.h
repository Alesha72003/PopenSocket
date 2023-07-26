#ifndef ASYNCPOEPNSOCKET_H
#define ASYNCPOPENSOCKET_H

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#define WIN32_DEFAULT_LIBS
#define UNICODE
#pragma comment(lib, "user32.lib") 
#include <windows.h>

//Object define
typedef struct {
    PyObject_HEAD
    HANDLE handleWrite;
    HANDLE handleRead;
    unsigned long timeout;
    OVERLAPPED overlap;
    PyObject* SocketException;
    BOOL closed;
    PROCESS_INFORMATION proc;
} AsyncPopenSocket_Object;


static PyMemberDef AsyncPopenSocket_members[] = {
    // {"handleRead", T_INT, offsetof(AsyncPopenSocket_Object, handleRead), READONLY, "Pipe handle for read. Internal use only."},
    // {"handleWrite", T_INT, offsetof(AsyncPopenSocket_Object, handleWrite), READONLY, "Pipe handle for write. Internal use only."},
    {"timeout", T_INT, offsetof(AsyncPopenSocket_Object, timeout), 0, "timeout for reading from pipe"},
    // {"closed", T_INT, offsetof(AsyncPopenSocket_Object, closed), READONLY, "Closed flag of child pipe"},
    {NULL}
};

//Init
static void AsyncPopenSocket_dealloc(AsyncPopenSocket_Object* self);
static PyObject* AsyncPopenSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int AsyncPopenSocket_init(AsyncPopenSocket_Object* self, PyObject* args, PyObject* kwds);

//Methods
static PyObject* AsyncPopenSocket_send(AsyncPopenSocket_Object* self, PyObject* arg);
static PyObject* AsyncPopenSocket_recv(AsyncPopenSocket_Object* self, PyObject* arg);
static PyObject* AsyncPopenSocket_close(AsyncPopenSocket_Object* self, PyObject* Py_UNUSED(ignored));
static PyObject* AsyncPopenSocket_settimeout(AsyncPopenSocket_Object* self, PyObject* arg);

static PyMethodDef AsyncPopenSocket_methods[] = {
    {"send", (PyCFunction) AsyncPopenSocket_send, METH_VARARGS, "Send data to child"},
    {"recv", (PyCFunction) AsyncPopenSocket_recv, METH_VARARGS, "Read data from child"},
    {"close", (PyCFunction) AsyncPopenSocket_close, METH_NOARGS, "Kill child"},
    {"settimeout", (PyCFunction) AsyncPopenSocket_settimeout, METH_VARARGS, "Set timeout. Wrapper for socket"},
    {NULL}
};

//getsetters
static PyObject* AsyncPopenSocket_getclosed(AsyncPopenSocket_Object* self, void*);
static int AsyncPopenSocket_setclosed(AsyncPopenSocket_Object* self, PyObject* value, void*);


static PyGetSetDef AsyncPopenSocket_getsetters[] = {
    {"closed", (getter) AsyncPopenSocket_getclosed, (setter) AsyncPopenSocket_setclosed, "get child state", NULL},
    {"_closed", (getter) AsyncPopenSocket_getclosed, (setter) AsyncPopenSocket_setclosed, "get child state", NULL},
    {NULL}
};

//general declaration 
static PyTypeObject AsyncPopenSocket_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "AsyncPopenSocket.AsyncPopenSocket",
    .tp_basicsize = sizeof(AsyncPopenSocket_Object),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor) AsyncPopenSocket_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("Run child process and create socket-like object for communicate"),
    .tp_methods = AsyncPopenSocket_methods,
    .tp_members = AsyncPopenSocket_members,
    .tp_getset = AsyncPopenSocket_getsetters,
    .tp_init = (initproc) AsyncPopenSocket_init,
    .tp_new = (newfunc) AsyncPopenSocket_new,
};

static PyModuleDef AsyncPopenSocket_Module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "AsyncPopenSocket",
    .m_doc = "Run child process and create socket-like object for communicate",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_AsyncPopenSocket(void);

#endif // ASYNCPOPENSOCKET_H