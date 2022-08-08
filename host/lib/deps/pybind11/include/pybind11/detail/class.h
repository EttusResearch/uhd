/*
    Copyright (c) 2016 Wenzel Jakob <wenzel.jakob@epfl.ch>

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/


#pragma once

#include "../attr.h"
#include "../options.h"

PYBIND11_NAMESPACE_BEGIN(PYBIND11_NAMESPACE)
PYBIND11_NAMESPACE_BEGIN(detail)

#if !defined(PYPY_VERSION)
#    define PYBIND11_BUILTIN_QUALNAME
#    define PYBIND11_SET_OLDPY_QUALNAME(obj, nameobj)
#else


#    define PYBIND11_SET_OLDPY_QUALNAME(obj, nameobj)                                             \
        setattr((PyObject *) obj, "__qualname__", nameobj)
#endif

inline std::string get_fully_qualified_tp_name(PyTypeObject *type) {
#if !defined(PYPY_VERSION)
    return type->tp_name;
#else
    auto module_name = handle((PyObject *) type).attr("__module__").cast<std::string>();
    if (module_name == PYBIND11_BUILTINS_MODULE)
        return type->tp_name;
    else
        return std::move(module_name) + "." + type->tp_name;
#endif
}

inline PyTypeObject *type_incref(PyTypeObject *type) {
    Py_INCREF(type);
    return type;
}

#if !defined(PYPY_VERSION)


extern "C" inline PyObject *pybind11_static_get(PyObject *self, PyObject *  , PyObject *cls) {
    return PyProperty_Type.tp_descr_get(self, cls, cls);
}


extern "C" inline int pybind11_static_set(PyObject *self, PyObject *obj, PyObject *value) {
    PyObject *cls = PyType_Check(obj) ? obj : (PyObject *) Py_TYPE(obj);
    return PyProperty_Type.tp_descr_set(self, cls, value);
}


inline PyTypeObject *make_static_property_type() {
    constexpr auto *name = "pybind11_static_property";
    auto name_obj = reinterpret_steal<object>(PYBIND11_FROM_STRING(name));


    auto *heap_type = (PyHeapTypeObject *) PyType_Type.tp_alloc(&PyType_Type, 0);
    if (!heap_type) {
        pybind11_fail("make_static_property_type(): error allocating type!");
    }

    heap_type->ht_name = name_obj.inc_ref().ptr();
#    ifdef PYBIND11_BUILTIN_QUALNAME
    heap_type->ht_qualname = name_obj.inc_ref().ptr();
#    endif

    auto *type = &heap_type->ht_type;
    type->tp_name = name;
    type->tp_base = type_incref(&PyProperty_Type);
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;
    type->tp_descr_get = pybind11_static_get;
    type->tp_descr_set = pybind11_static_set;

    if (PyType_Ready(type) < 0) {
        pybind11_fail("make_static_property_type(): failure in PyType_Ready()!");
    }

    setattr((PyObject *) type, "__module__", str("pybind11_builtins"));
    PYBIND11_SET_OLDPY_QUALNAME(type, name_obj);

    return type;
}

#else


inline PyTypeObject *make_static_property_type() {
    auto d = dict();
    PyObject *result = PyRun_String(R"(\
class pybind11_static_property(property):
    def __get__(self, obj, cls):
        return property.__get__(self, cls, cls)

    def __set__(self, obj, value):
        cls = obj if isinstance(obj, type) else type(obj)
        property.__set__(self, cls, value)
)",
                                    Py_file_input,
                                    d.ptr(),
                                    d.ptr());
    if (result == nullptr)
        throw error_already_set();
    Py_DECREF(result);
    return (PyTypeObject *) d["pybind11_static_property"].cast<object>().release().ptr();
}

#endif


extern "C" inline int pybind11_meta_setattro(PyObject *obj, PyObject *name, PyObject *value) {


    PyObject *descr = _PyType_Lookup((PyTypeObject *) obj, name);





    auto *const static_prop = (PyObject *) get_internals().static_property_type;
    const auto call_descr_set = (descr != nullptr) && (value != nullptr)
                                && (PyObject_IsInstance(descr, static_prop) != 0)
                                && (PyObject_IsInstance(value, static_prop) == 0);
    if (call_descr_set) {

#if !defined(PYPY_VERSION)
        return Py_TYPE(descr)->tp_descr_set(descr, obj, value);
#else
        if (PyObject *result = PyObject_CallMethod(descr, "__set__", "OO", obj, value)) {
            Py_DECREF(result);
            return 0;
        } else {
            return -1;
        }
#endif
    } else {

        return PyType_Type.tp_setattro(obj, name, value);
    }
}


extern "C" inline PyObject *pybind11_meta_getattro(PyObject *obj, PyObject *name) {
    PyObject *descr = _PyType_Lookup((PyTypeObject *) obj, name);
    if (descr && PyInstanceMethod_Check(descr)) {
        Py_INCREF(descr);
        return descr;
    }
    return PyType_Type.tp_getattro(obj, name);
}


extern "C" inline PyObject *pybind11_meta_call(PyObject *type, PyObject *args, PyObject *kwargs) {


    PyObject *self = PyType_Type.tp_call(type, args, kwargs);
    if (self == nullptr) {
        return nullptr;
    }


    auto *instance = reinterpret_cast<detail::instance *>(self);


    for (const auto &vh : values_and_holders(instance)) {
        if (!vh.holder_constructed()) {
            PyErr_Format(PyExc_TypeError,
                         "%.200s.__init__() must be called when overriding __init__",
                         get_fully_qualified_tp_name(vh.type->type).c_str());
            Py_DECREF(self);
            return nullptr;
        }
    }

    return self;
}


extern "C" inline void pybind11_meta_dealloc(PyObject *obj) {
    auto *type = (PyTypeObject *) obj;
    auto &internals = get_internals();




    auto found_type = internals.registered_types_py.find(type);
    if (found_type != internals.registered_types_py.end() && found_type->second.size() == 1
        && found_type->second[0]->type == type) {

        auto *tinfo = found_type->second[0];
        auto tindex = std::type_index(*tinfo->cpptype);
        internals.direct_conversions.erase(tindex);

        if (tinfo->module_local) {
            get_local_internals().registered_types_cpp.erase(tindex);
        } else {
            internals.registered_types_cpp.erase(tindex);
        }
        internals.registered_types_py.erase(tinfo->type);


        auto &cache = internals.inactive_override_cache;
        for (auto it = cache.begin(), last = cache.end(); it != last;) {
            if (it->first == (PyObject *) tinfo->type) {
                it = cache.erase(it);
            } else {
                ++it;
            }
        }

        delete tinfo;
    }

    PyType_Type.tp_dealloc(obj);
}


inline PyTypeObject *make_default_metaclass() {
    constexpr auto *name = "pybind11_type";
    auto name_obj = reinterpret_steal<object>(PYBIND11_FROM_STRING(name));


    auto *heap_type = (PyHeapTypeObject *) PyType_Type.tp_alloc(&PyType_Type, 0);
    if (!heap_type) {
        pybind11_fail("make_default_metaclass(): error allocating metaclass!");
    }

    heap_type->ht_name = name_obj.inc_ref().ptr();
#ifdef PYBIND11_BUILTIN_QUALNAME
    heap_type->ht_qualname = name_obj.inc_ref().ptr();
#endif

    auto *type = &heap_type->ht_type;
    type->tp_name = name;
    type->tp_base = type_incref(&PyType_Type);
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;

    type->tp_call = pybind11_meta_call;

    type->tp_setattro = pybind11_meta_setattro;
    type->tp_getattro = pybind11_meta_getattro;

    type->tp_dealloc = pybind11_meta_dealloc;

    if (PyType_Ready(type) < 0) {
        pybind11_fail("make_default_metaclass(): failure in PyType_Ready()!");
    }

    setattr((PyObject *) type, "__module__", str("pybind11_builtins"));
    PYBIND11_SET_OLDPY_QUALNAME(type, name_obj);

    return type;
}





inline void traverse_offset_bases(void *valueptr,
                                  const detail::type_info *tinfo,
                                  instance *self,
                                  bool (*f)(void *  , instance *  )) {
    for (handle h : reinterpret_borrow<tuple>(tinfo->type->tp_bases)) {
        if (auto *parent_tinfo = get_type_info((PyTypeObject *) h.ptr())) {
            for (auto &c : parent_tinfo->implicit_casts) {
                if (c.first == tinfo->cpptype) {
                    auto *parentptr = c.second(valueptr);
                    if (parentptr != valueptr) {
                        f(parentptr, self);
                    }
                    traverse_offset_bases(parentptr, parent_tinfo, self, f);
                    break;
                }
            }
        }
    }
}

inline bool register_instance_impl(void *ptr, instance *self) {
    get_internals().registered_instances.emplace(ptr, self);
    return true;
}
inline bool deregister_instance_impl(void *ptr, instance *self) {
    auto &registered_instances = get_internals().registered_instances;
    auto range = registered_instances.equal_range(ptr);
    for (auto it = range.first; it != range.second; ++it) {
        if (self == it->second) {
            registered_instances.erase(it);
            return true;
        }
    }
    return false;
}

inline void register_instance(instance *self, void *valptr, const type_info *tinfo) {
    register_instance_impl(valptr, self);
    if (!tinfo->simple_ancestors) {
        traverse_offset_bases(valptr, tinfo, self, register_instance_impl);
    }
}

inline bool deregister_instance(instance *self, void *valptr, const type_info *tinfo) {
    bool ret = deregister_instance_impl(valptr, self);
    if (!tinfo->simple_ancestors) {
        traverse_offset_bases(valptr, tinfo, self, deregister_instance_impl);
    }
    return ret;
}




inline PyObject *make_new_instance(PyTypeObject *type) {
#if defined(PYPY_VERSION)


    ssize_t instance_size = static_cast<ssize_t>(sizeof(instance));
    if (type->tp_basicsize < instance_size) {
        type->tp_basicsize = instance_size;
    }
#endif
    PyObject *self = type->tp_alloc(type, 0);
    auto *inst = reinterpret_cast<instance *>(self);

    inst->allocate_layout();

    return self;
}



extern "C" inline PyObject *pybind11_object_new(PyTypeObject *type, PyObject *, PyObject *) {
    return make_new_instance(type);
}




extern "C" inline int pybind11_object_init(PyObject *self, PyObject *, PyObject *) {
    PyTypeObject *type = Py_TYPE(self);
    std::string msg = get_fully_qualified_tp_name(type) + ": No constructor defined!";
    PyErr_SetString(PyExc_TypeError, msg.c_str());
    return -1;
}

inline void add_patient(PyObject *nurse, PyObject *patient) {
    auto &internals = get_internals();
    auto *instance = reinterpret_cast<detail::instance *>(nurse);
    instance->has_patients = true;
    Py_INCREF(patient);
    internals.patients[nurse].push_back(patient);
}

inline void clear_patients(PyObject *self) {
    auto *instance = reinterpret_cast<detail::instance *>(self);
    auto &internals = get_internals();
    auto pos = internals.patients.find(self);
    assert(pos != internals.patients.end());



    auto patients = std::move(pos->second);
    internals.patients.erase(pos);
    instance->has_patients = false;
    for (PyObject *&patient : patients) {
        Py_CLEAR(patient);
    }
}



inline void clear_instance(PyObject *self) {
    auto *instance = reinterpret_cast<detail::instance *>(self);


    for (auto &v_h : values_and_holders(instance)) {
        if (v_h) {



            if (v_h.instance_registered()
                && !deregister_instance(instance, v_h.value_ptr(), v_h.type)) {
                pybind11_fail(
                    "pybind11_object_dealloc(): Tried to deallocate unregistered instance!");
            }

            if (instance->owned || v_h.holder_constructed()) {
                v_h.type->dealloc(v_h);
            }
        }
    }

    instance->deallocate_layout();

    if (instance->weakrefs) {
        PyObject_ClearWeakRefs(self);
    }

    PyObject **dict_ptr = _PyObject_GetDictPtr(self);
    if (dict_ptr) {
        Py_CLEAR(*dict_ptr);
    }

    if (instance->has_patients) {
        clear_patients(self);
    }
}



extern "C" inline void pybind11_object_dealloc(PyObject *self) {
    clear_instance(self);

    auto *type = Py_TYPE(self);
    type->tp_free(self);

#if PY_VERSION_HEX < 0x03080000




    auto pybind11_object_type = (PyTypeObject *) get_internals().instance_base;
    if (type->tp_dealloc == pybind11_object_type->tp_dealloc)
        Py_DECREF(type);
#else


    Py_DECREF(type);
#endif
}

std::string error_string();


inline PyObject *make_object_base_type(PyTypeObject *metaclass) {
    constexpr auto *name = "pybind11_object";
    auto name_obj = reinterpret_steal<object>(PYBIND11_FROM_STRING(name));


    auto *heap_type = (PyHeapTypeObject *) metaclass->tp_alloc(metaclass, 0);
    if (!heap_type) {
        pybind11_fail("make_object_base_type(): error allocating type!");
    }

    heap_type->ht_name = name_obj.inc_ref().ptr();
#ifdef PYBIND11_BUILTIN_QUALNAME
    heap_type->ht_qualname = name_obj.inc_ref().ptr();
#endif

    auto *type = &heap_type->ht_type;
    type->tp_name = name;
    type->tp_base = type_incref(&PyBaseObject_Type);
    type->tp_basicsize = static_cast<ssize_t>(sizeof(instance));
    type->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HEAPTYPE;

    type->tp_new = pybind11_object_new;
    type->tp_init = pybind11_object_init;
    type->tp_dealloc = pybind11_object_dealloc;


    type->tp_weaklistoffset = offsetof(instance, weakrefs);

    if (PyType_Ready(type) < 0) {
        pybind11_fail("PyType_Ready failed in make_object_base_type(): " + error_string());
    }

    setattr((PyObject *) type, "__module__", str("pybind11_builtins"));
    PYBIND11_SET_OLDPY_QUALNAME(type, name_obj);

    assert(!PyType_HasFeature(type, Py_TPFLAGS_HAVE_GC));
    return (PyObject *) heap_type;
}


extern "C" inline PyObject *pybind11_get_dict(PyObject *self, void *) {
    PyObject *&dict = *_PyObject_GetDictPtr(self);
    if (!dict) {
        dict = PyDict_New();
    }
    Py_XINCREF(dict);
    return dict;
}


extern "C" inline int pybind11_set_dict(PyObject *self, PyObject *new_dict, void *) {
    if (!PyDict_Check(new_dict)) {
        PyErr_Format(PyExc_TypeError,
                     "__dict__ must be set to a dictionary, not a '%.200s'",
                     get_fully_qualified_tp_name(Py_TYPE(new_dict)).c_str());
        return -1;
    }
    PyObject *&dict = *_PyObject_GetDictPtr(self);
    Py_INCREF(new_dict);
    Py_CLEAR(dict);
    dict = new_dict;
    return 0;
}


extern "C" inline int pybind11_traverse(PyObject *self, visitproc visit, void *arg) {
    PyObject *&dict = *_PyObject_GetDictPtr(self);
    Py_VISIT(dict);

#if PY_VERSION_HEX >= 0x03090000
    Py_VISIT(Py_TYPE(self));
#endif
    return 0;
}


extern "C" inline int pybind11_clear(PyObject *self) {
    PyObject *&dict = *_PyObject_GetDictPtr(self);
    Py_CLEAR(dict);
    return 0;
}


inline void enable_dynamic_attributes(PyHeapTypeObject *heap_type) {
    auto *type = &heap_type->ht_type;
    type->tp_flags |= Py_TPFLAGS_HAVE_GC;
#if PY_VERSION_HEX < 0x030B0000
    type->tp_dictoffset = type->tp_basicsize;
    type->tp_basicsize += (ssize_t) sizeof(PyObject *);
#else
    type->tp_flags |= Py_TPFLAGS_MANAGED_DICT;
#endif
    type->tp_traverse = pybind11_traverse;
    type->tp_clear = pybind11_clear;

    static PyGetSetDef getset[] = {
        {const_cast<char *>("__dict__"), pybind11_get_dict, pybind11_set_dict, nullptr, nullptr},
        {nullptr, nullptr, nullptr, nullptr, nullptr}};
    type->tp_getset = getset;
}


extern "C" inline int pybind11_getbuffer(PyObject *obj, Py_buffer *view, int flags) {

    type_info *tinfo = nullptr;
    for (auto type : reinterpret_borrow<tuple>(Py_TYPE(obj)->tp_mro)) {
        tinfo = get_type_info((PyTypeObject *) type.ptr());
        if (tinfo && tinfo->get_buffer) {
            break;
        }
    }
    if (view == nullptr || !tinfo || !tinfo->get_buffer) {
        if (view) {
            view->obj = nullptr;
        }
        PyErr_SetString(PyExc_BufferError, "pybind11_getbuffer(): Internal error");
        return -1;
    }
    std::memset(view, 0, sizeof(Py_buffer));
    buffer_info *info = tinfo->get_buffer(obj, tinfo->get_buffer_data);
    if ((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE && info->readonly) {
        delete info;

        PyErr_SetString(PyExc_BufferError, "Writable buffer requested for readonly storage");
        return -1;
    }
    view->obj = obj;
    view->ndim = 1;
    view->internal = info;
    view->buf = info->ptr;
    view->itemsize = info->itemsize;
    view->len = view->itemsize;
    for (auto s : info->shape) {
        view->len *= s;
    }
    view->readonly = static_cast<int>(info->readonly);
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) {
        view->format = const_cast<char *>(info->format.c_str());
    }
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES) {
        view->ndim = (int) info->ndim;
        view->strides = info->strides.data();
        view->shape = info->shape.data();
    }
    Py_INCREF(view->obj);
    return 0;
}


extern "C" inline void pybind11_releasebuffer(PyObject *, Py_buffer *view) {
    delete (buffer_info *) view->internal;
}


inline void enable_buffer_protocol(PyHeapTypeObject *heap_type) {
    heap_type->ht_type.tp_as_buffer = &heap_type->as_buffer;

    heap_type->as_buffer.bf_getbuffer = pybind11_getbuffer;
    heap_type->as_buffer.bf_releasebuffer = pybind11_releasebuffer;
}


inline PyObject *make_new_python_type(const type_record &rec) {
    auto name = reinterpret_steal<object>(PYBIND11_FROM_STRING(rec.name));

    auto qualname = name;
    if (rec.scope && !PyModule_Check(rec.scope.ptr()) && hasattr(rec.scope, "__qualname__")) {
        qualname = reinterpret_steal<object>(
            PyUnicode_FromFormat("%U.%U", rec.scope.attr("__qualname__").ptr(), name.ptr()));
    }

    object module_;
    if (rec.scope) {
        if (hasattr(rec.scope, "__module__")) {
            module_ = rec.scope.attr("__module__");
        } else if (hasattr(rec.scope, "__name__")) {
            module_ = rec.scope.attr("__name__");
        }
    }

    const auto *full_name = c_str(
#if !defined(PYPY_VERSION)
        module_ ? str(module_).cast<std::string>() + "." + rec.name :
#endif
                rec.name);

    char *tp_doc = nullptr;
    if (rec.doc && options::show_user_defined_docstrings()) {

        size_t size = std::strlen(rec.doc) + 1;
        tp_doc = (char *) PyObject_MALLOC(size);
        std::memcpy((void *) tp_doc, rec.doc, size);
    }

    auto &internals = get_internals();
    auto bases = tuple(rec.bases);
    auto *base = (bases.empty()) ? internals.instance_base : bases[0].ptr();


    auto *metaclass
        = rec.metaclass.ptr() ? (PyTypeObject *) rec.metaclass.ptr() : internals.default_metaclass;

    auto *heap_type = (PyHeapTypeObject *) metaclass->tp_alloc(metaclass, 0);
    if (!heap_type) {
        pybind11_fail(std::string(rec.name) + ": Unable to create type object!");
    }

    heap_type->ht_name = name.release().ptr();
#ifdef PYBIND11_BUILTIN_QUALNAME
    heap_type->ht_qualname = qualname.inc_ref().ptr();
#endif

    auto *type = &heap_type->ht_type;
    type->tp_name = full_name;
    type->tp_doc = tp_doc;
    type->tp_base = type_incref((PyTypeObject *) base);
    type->tp_basicsize = static_cast<ssize_t>(sizeof(instance));
    if (!bases.empty()) {
        type->tp_bases = bases.release().ptr();
    }


    type->tp_init = pybind11_object_init;


    type->tp_as_number = &heap_type->as_number;
    type->tp_as_sequence = &heap_type->as_sequence;
    type->tp_as_mapping = &heap_type->as_mapping;
    type->tp_as_async = &heap_type->as_async;


    type->tp_flags |= Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE;
    if (!rec.is_final) {
        type->tp_flags |= Py_TPFLAGS_BASETYPE;
    }

    if (rec.dynamic_attr) {
        enable_dynamic_attributes(heap_type);
    }

    if (rec.buffer_protocol) {
        enable_buffer_protocol(heap_type);
    }

    if (rec.custom_type_setup_callback) {
        rec.custom_type_setup_callback(heap_type);
    }

    if (PyType_Ready(type) < 0) {
        pybind11_fail(std::string(rec.name) + ": PyType_Ready failed: " + error_string());
    }

    assert(!rec.dynamic_attr || PyType_HasFeature(type, Py_TPFLAGS_HAVE_GC));


    if (rec.scope) {
        setattr(rec.scope, rec.name, (PyObject *) type);
    } else {
        Py_INCREF(type);
    }

    if (module_) {
        setattr((PyObject *) type, "__module__", module_);
    }

    PYBIND11_SET_OLDPY_QUALNAME(type, qualname);

    return (PyObject *) type;
}

PYBIND11_NAMESPACE_END(detail)
PYBIND11_NAMESPACE_END(PYBIND11_NAMESPACE)
