#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <systemd/sd-bus.h>

#define SD_BUS_PY_CHECK_RETURN_VALUE(_exception_to_raise) \
    if (return_value < 0)                                 \
    {                                                     \
        PyErr_SetFromErrno(_exception_to_raise);          \
        return NULL;                                      \
    }

typedef struct
{
    PyObject_HEAD;
    sd_bus_message *message_ref;
} SdBusMessageObject;

static void
SdBusMessage_free(SdBusMessageObject *self)
{
    sd_bus_message_unref(self->message_ref);
}

static PyObject *
SdBusMessage_dump(SdBusMessageObject *self, PyObject *args)
{
    int return_value = sd_bus_message_dump(self->message_ref, 0, SD_BUS_MESSAGE_DUMP_WITH_HEADER);
    SD_BUS_PY_CHECK_RETURN_VALUE(PyExc_RuntimeError);
    return_value = sd_bus_message_rewind(self->message_ref, 1);
    SD_BUS_PY_CHECK_RETURN_VALUE(PyExc_RuntimeError);
    return Py_None;
}

static PyMethodDef SdBusMessage_methods[] = {
    {"dump", (PyCFunction)SdBusMessage_dump, METH_NOARGS, "Dump message to stdout"},
    {NULL, NULL, 0, NULL},
};

static PyTypeObject SdBusMessageType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "sd_bus_internals.SdBusMessage",
    .tp_basicsize = sizeof(SdBusMessageObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_free = (freefunc)SdBusMessage_free,
    .tp_methods = SdBusMessage_methods,
};

typedef struct
{
    PyObject_HEAD;
    sd_bus *sd_bus_ref;
} SdBusObject;

static PyObject *
SdBus_test(SdBusObject *self, PyObject *args);

static void
SdBus_free(SdBusObject *self)
{
    sd_bus_unref(self->sd_bus_ref);
}

static PyMethodDef SdBus_methods[] = {
    {"test", (PyCFunction)SdBus_test, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL},
};

static PyTypeObject SdBusType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "sd_bus_internals.SdBus",
    .tp_basicsize = sizeof(SdBusObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_free = (freefunc)SdBus_free,
    .tp_methods = SdBus_methods,
};

static PyObject *
SdBus_test(SdBusObject *self, PyObject *args)
{
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *m = NULL;
    const char *name = NULL;

    int return_value = sd_bus_call_method(
        self->sd_bus_ref,
        "org.freedesktop.DBus",
        "/org/freedesktop/DBus",
        "org.freedesktop.DBus.Peer",
        "GetMachineId",
        &error,
        &m,
        "");
    if (return_value < 0)
    {
        PyErr_SetFromErrno(PyExc_RuntimeError);
        return NULL;
    }
    return_value = sd_bus_message_read(m, "s", &name);
    if (return_value < 0)
    {
        PyErr_SetFromErrno(PyExc_RuntimeError);
        return NULL;
    }
    return PyUnicode_FromString(name);
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
}

static SdBusObject *
get_default_sd_bus(PyObject *self,
                   PyObject *Py_UNUSED(ignored))
{
    SdBusObject *new_sd_bus = PyObject_New(SdBusObject, &SdBusType);
    int return_value = sd_bus_default(&(new_sd_bus->sd_bus_ref));
    if (return_value < 0)
    {
        PyErr_SetFromErrno(PyExc_RuntimeError);
        return NULL;
    }
    else
    {
        return new_sd_bus;
    }
}

static PyMethodDef SdBusPyInternal_methods[] = {
    {"get_default_sdbus", (PyCFunction)get_default_sd_bus, METH_NOARGS, "Get default sd_bus."},
    {NULL, NULL, 0, NULL},
};

static PyModuleDef sd_bus_internals_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "sd_bus_internals",
    .m_doc = "Sd bus internals module.",
    .m_methods = SdBusPyInternal_methods,
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_sd_bus_internals(void)
{
    PyObject *m;
    if (PyType_Ready(&SdBusType) < 0)
    {
        return NULL;
    }
    if (PyType_Ready(&SdBusMessageType) < 0)
    {
        return NULL;
    }

    m = PyModule_Create(&sd_bus_internals_module);
    if (m == NULL)
    {
        return NULL;
    }

    Py_INCREF(&SdBusType);
    if (PyModule_AddObject(m, "SdBus", (PyObject *)&SdBusType) < 0)
    {
        Py_DECREF(&SdBusType);
        Py_DECREF(m);
        return NULL;
    }
    if (PyModule_AddObject(m, "SdBusMessage", (PyObject *)&SdBusMessageType) < 0)
    {
        Py_DECREF(&SdBusMessageType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}