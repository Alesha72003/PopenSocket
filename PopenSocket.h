#ifndef POEPNSOCKET_H
#define POPENSOCKET_H

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
} PopenSocket_Object;


static PyMemberDef PopenSocket_members[] = {
    // {"handleRead", T_INT, offsetof(PopenSocket_Object, handleRead), READONLY, "Pipe handle for read. Internal use only."},
    // {"handleWrite", T_INT, offsetof(PopenSocket_Object, handleWrite), READONLY, "Pipe handle for write. Internal use only."},
    {"timeout", T_INT, offsetof(PopenSocket_Object, timeout), 0, "timeout for reading from pipe"},
    // {"closed", T_INT, offsetof(PopenSocket_Object, closed), READONLY, "Closed flag of child pipe"},
    {NULL}
};

//Init
static void PopenSocket_dealloc(PopenSocket_Object* self);
static PyObject* PopenSocket_new(PyTypeObject* type, PyObject* args, PyObject* kwds);
static int PopenSocket_init(PopenSocket_Object* self, PyObject* args, PyObject* kwds);

//Methods
static PyObject* PopenSocket_send(PopenSocket_Object* self, PyObject* arg);
static PyObject* PopenSocket_recv(PopenSocket_Object* self, PyObject* arg);
static PyObject* PopenSocket_close(PopenSocket_Object* self, PyObject* Py_UNUSED(ignored));
static PyObject* PopenSocket_settimeout(PopenSocket_Object* self, PyObject* arg);

static PyMethodDef PopenSocket_methods[] = {
    {"send", (PyCFunction) PopenSocket_send, METH_VARARGS, "Send data to child"},
    {"recv", (PyCFunction) PopenSocket_recv, METH_VARARGS, "Read data from child"},
    {"close", (PyCFunction) PopenSocket_close, METH_NOARGS, "Kill child"},
    {"settimeout", (PyCFunction) PopenSocket_settimeout, METH_VARARGS, "Set timeout. Wrapper for socket"},
    {NULL}
};

//getsetters
static PyObject* PopenSocket_getclosed(PopenSocket_Object* self, void*);
static int PopenSocket_setclosed(PopenSocket_Object* self, PyObject* value, void*);


static PyGetSetDef PopenSocket_getsetters[] = {
    {"closed", (getter) PopenSocket_getclosed, (setter) PopenSocket_setclosed, "get child state", NULL},
    {"_closed", (getter) PopenSocket_getclosed, (setter) PopenSocket_setclosed, "get child state", NULL},
    {NULL}
};

//general declaration 
static PyTypeObject PopenSocket_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "PopenSocket.PopenSocket",
    .tp_basicsize = sizeof(PopenSocket_Object),
    .tp_itemsize = 0,
    .tp_dealloc = (destructor) PopenSocket_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc = PyDoc_STR("Run child process and create socket-like object for communicate"),
    .tp_methods = PopenSocket_methods,
    .tp_members = PopenSocket_members,
    .tp_getset = PopenSocket_getsetters,
    .tp_init = (initproc) PopenSocket_init,
    .tp_new = (newfunc) PopenSocket_new,
};

static PyModuleDef PopenSocket_Module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "PopenSocket",
    .m_doc = "Run child process and create socket-like object for communicate",
    .m_size = -1,
};

PyMODINIT_FUNC PyInit_PopenSocket(void);

#endif // POPENSOCKET_H