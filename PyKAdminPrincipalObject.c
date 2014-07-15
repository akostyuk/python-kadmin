
#include "PyKAdminObject.h"
#include "PyKAdminErrors.h"
#include "PyKAdminIterator.h"
#include "PyKAdminPrincipalObject.h"
#include "PyKAdminPolicyObject.h"

#include "PyKAdminCommon.h"

#include <datetime.h>

/*
kadm_principal_ent_rec reference

typedef struct _kadm5_principal_ent_t {

    // done
    krb5_principal  principal;
    krb5_principal  mod_name;

    // how to expose timestamps? 
    krb5_timestamp  princ_expire_time;
    krb5_timestamp  last_pwd_change;
    krb5_timestamp  pw_expiration;
    krb5_timestamp  mod_date;

    krb5_deltat     max_life;

    krb5_kvno       kvno;
    krb5_kvno       mkvno;

    char            *policy;

    krb5_flags      attributes;
    long            aux_attributes;

    // version 2 fields 

    krb5_timestamp last_success;
    krb5_timestamp last_failed;

    krb5_deltat max_renewable_life;

    krb5_kvno fail_auth_count;

    // these should not be accessed or modified directly by python.
    krb5_int16 n_key_data;
    krb5_int16 n_tl_data;
    krb5_tl_data *tl_data;
    krb5_key_data *key_data;

} kadm5_principal_ent_rec, *kadm5_principal_ent_t;
*/




static void KAdminPrincipal_dealloc(PyKAdminPrincipalObject *self) {
    
    kadm5_free_principal_ent(self->kadmin->server_handle, &self->entry);

    Py_XDECREF(self->kadmin);
   
    self->ob_type->tp_free((PyObject*)self);
}

static PyObject *KAdminPrincipal_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {

    PyKAdminPrincipalObject *self;

    self = (PyKAdminPrincipalObject *)type->tp_alloc(type, 0);

    if (!self)
        return NULL;
    
    memset(&self->entry, 0, sizeof(kadm5_principal_ent_rec));
    return (PyObject *)self;

}

static int KAdminPrincipal_init(PyKAdminPrincipalObject *self, PyObject *args, PyObject *kwds) {

    return 0;
}



static int KAdminPrincipal_print(PyKAdminPrincipalObject *self, FILE *file, int flags){

    static const char *kPRINT_FORMAT = "%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s";

    krb5_error_code errno;
    char *client_name = NULL;

    if (self && self->kadmin) {

        errno = krb5_unparse_name(self->kadmin->context, self->entry.principal, &client_name);

        fprintf(file, kPRINT_FORMAT, 
            "Principal",                      client_name,
            "Expiration date",                NULL,
            "Last password change",           NULL,
            "Password expiration date",       NULL,
            "Maximum ticket life",            NULL,
            "Maximum renewable life",         NULL,
            "Last modified",                  NULL,
            "Last successful authentication", NULL,
            "Last failed authentication",     NULL,
            "Failed password attempts",       NULL,
            "Number of keys",                 NULL
            );
    }

    if (client_name)
        free(client_name);
    
    return 0;
}



static PyMemberDef KAdminPrincipal_members[] = {


    //{"last_password_change",        T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, last_pwd_change),       READONLY, ""},
    //{"expire_time",                 T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, princ_expire_time),     READONLY, ""},
    //{"password_expiration",         T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, pw_expiration),         READONLY, ""},
    //{"modified_time",               T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, mod_date),              READONLY, ""},
    //{"max_life",                    T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, max_life),              READONLY, ""},
    //{"max_renewable_life",          T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, max_renewable_life),    READONLY, ""},
    //{"last_success",                T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, last_success),          READONLY, ""},
    //{"last_failed",                 T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, last_failed),           READONLY, ""},
    
    {"failed_auth_count",           T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, fail_auth_count),       READONLY, ""},
    {"key_version_number",          T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, kvno),                  READONLY, ""},
    {"master_key_version_number",   T_INT, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, mkvno),                 READONLY, ""},

    {"policy",                      T_STRING, offsetof(PyKAdminPrincipalObject, entry) + offsetof(kadm5_principal_ent_rec, policy),                READONLY, ""},
    
    {NULL}
};



static PyObject *KAdminPrincipal_set_expire(PyKAdminPrincipalObject *self, PyObject *args, PyObject *kwds) {
    
    kadm5_ret_t retval = KADM5_OK; 
    time_t date        = 0; 
    char *expire       = NULL;

    if (!PyArg_ParseTuple(args, "s", &expire))
        return NULL;
    
    date = get_date(expire);
    if (date == (time_t)-1 ) { 
        // todo raise exception 
        return NULL;
    }

    self->entry.princ_expire_time = date;

    retval = kadm5_modify_principal(self->kadmin->server_handle, &self->entry, KADM5_PRINC_EXPIRE_TIME);
    if (retval != KADM5_OK) { PyKAdmin_RaiseKAdminError(retval, "kadm5_modify_principal"); return NULL; }

    Py_RETURN_TRUE;
}


static PyObject *KAdminPrincipal_clear_policy(PyKAdminPrincipalObject *self) {
    
    kadm5_ret_t retval = KADM5_OK;

    retval = kadm5_modify_principal(self->kadmin->server_handle, &self->entry, KADM5_POLICY_CLR);
    if (retval != KADM5_OK) { PyKAdmin_RaiseKAdminError(retval, "kadm5_modify_principal"); return NULL; }

    Py_RETURN_TRUE;
}


static PyObject *KAdminPrincipal_set_policy(PyKAdminPrincipalObject *self, PyObject *args, PyObject *kwds) {
    
    kadm5_ret_t retval = KADM5_OK;

    //  todo: parse as a pyobject, if we pass in a PolicyObject we need to accept that too.

    if (!PyArg_ParseTuple(args, "|z", &self->entry.policy))
        return NULL;

    if (self->entry.policy == NULL) {
        KAdminPrincipal_clear_policy(self);
    } else {
        retval = kadm5_modify_principal(self->kadmin->server_handle, &self->entry, KADM5_POLICY);
        if (retval != KADM5_OK) { PyKAdmin_RaiseKAdminError(retval, "kadm5_modify_principal"); return NULL; }
    }

    Py_RETURN_TRUE;
}



static PyObject *_KAdminPrincipal_load_principal(PyKAdminPrincipalObject *self, char *client_name) {

    kadm5_ret_t retval = KADM5_OK;
    krb5_error_code errno;
    krb5_principal parsed_name;

    if (client_name) {

        errno = krb5_parse_name(self->kadmin->context, client_name, &parsed_name);

        if (errno) {
           printf("Failed to parse princ name %d\n", errno);
        }
    
        retval = kadm5_get_principal(self->kadmin->server_handle, parsed_name, &self->entry, KADM5_PRINCIPAL_NORMAL_MASK);

        krb5_free_principal(self->kadmin->context, parsed_name);
        
        if (retval != 0x0) { PyKAdmin_RaiseKAdminError(retval, "kadm5_get_principal"); return NULL; }

        Py_RETURN_TRUE;
    }

    // TODO: raise exception 
    return NULL;
    //Py_RETURN_FALSE;
}


static PyObject *KAdminPrincipal_reload(PyKAdminPrincipalObject *self) {

    kadm5_ret_t retval = KADM5_OK; 

    if (self) {
        retval = kadm5_get_principal(self->kadmin->server_handle, self->entry.principal, &self->entry, KADM5_PRINCIPAL_NORMAL_MASK);
        if (retval != KADM5_OK) { PyKAdmin_RaiseKAdminError(retval, "kadm5_get_principal"); return NULL; }
    }

    Py_RETURN_TRUE;
}

static PyObject *KAdminPrincipal_change_password(PyKAdminPrincipalObject *self, PyObject *args, PyObject *kwds) {

    kadm5_ret_t retval = KADM5_OK; 
    char *password     = NULL;

    if (!PyArg_ParseTuple(args, "s", &password))
        return NULL; 

    retval = kadm5_chpass_principal(self->kadmin->server_handle, self->entry.principal, password);
    if (retval != KADM5_OK) { PyKAdmin_RaiseKAdminError(retval, "kadm5_chpass_principal"); return NULL; }

    Py_RETURN_TRUE;
}

static PyObject *KAdminPrincipal_randomize_key(PyKAdminPrincipalObject *self) {

    kadm5_ret_t retval = KADM5_OK; 

    retval = kadm5_randkey_principal(self->kadmin->server_handle, self->entry.principal, NULL, NULL);
    if (retval != KADM5_OK) { PyKAdmin_RaiseKAdminError(retval, "kadm5_randkey_principal"); return NULL; }

    Py_RETURN_TRUE;
}

PyObject *PyKAdminPrincipal_RichCompare(PyObject *o1, PyObject *o2, int opid) {

    PyKAdminPrincipalObject *a = (PyKAdminPrincipalObject *)o1;
    PyKAdminPrincipalObject *b = (PyKAdminPrincipalObject *)o2;

    PyObject *result = NULL; 
        
    int equal = pykadmin_principal_ent_rec_compare(a->kadmin->context, &a->entry, &b->entry);

    switch (opid) {

        case Py_EQ:
            result = ((a == b) || equal) ? Py_True : Py_False;
            break;
        case Py_NE:
            result = ((a != b) && !equal) ? Py_True : Py_False;
            break;
        case Py_LT:
        case Py_LE:
        case Py_GT:
        case Py_GE:
        default: 
            result = Py_NotImplemented;
            goto done;
    }


done:
    Py_XINCREF(result);
    return result;
}

static PyMethodDef KAdminPrincipal_methods[] = {
    {"cpw",             (PyCFunction)KAdminPrincipal_change_password,   METH_VARARGS, ""},
    {"change_password", (PyCFunction)KAdminPrincipal_change_password,   METH_VARARGS, ""},
    {"randkey",         (PyCFunction)KAdminPrincipal_randomize_key,     METH_NOARGS, ""},
    {"randomize_key",   (PyCFunction)KAdminPrincipal_randomize_key,     METH_NOARGS, ""},
    
    {"expire",          (PyCFunction)KAdminPrincipal_set_expire,        METH_VARARGS, ""},
    {"set_policy",      (PyCFunction)KAdminPrincipal_set_policy,        METH_VARARGS, ""},
    {"clear_policy",    (PyCFunction)KAdminPrincipal_clear_policy,      METH_NOARGS, ""},

    {"reload",          (PyCFunction)KAdminPrincipal_reload,            METH_NOARGS, ""},

    {NULL, NULL, 0, NULL}
};



static PyObject *PyKAdminPrincipal_get_principal(PyKAdminPrincipalObject *self, void *closure) {
  
    krb5_error_code ret = 0;
    PyObject *principal = NULL;
    char *client_name   = NULL;
    
    // todo: handle error
    ret = krb5_unparse_name(self->kadmin->context, self->entry.principal, &client_name);

    if (client_name) {
        principal = PyString_FromString(client_name);
        free(client_name);
    }

    return principal;
}


static PyObject *PyKAdminPrincipal_get_mod_name(PyKAdminPrincipalObject *self, void *closure) {
  
    krb5_error_code ret = 0;
    PyObject *principal = NULL;
    char *client_name   = NULL;
    
    // todo: handle error
    ret = krb5_unparse_name(self->kadmin->context, self->entry.mod_name, &client_name);

    if (client_name) {
        principal = PyString_FromString(client_name);
        free(client_name);
    }

    return principal;
}

static PyObject *PyKAdminPrincipal_get_last_pwd_change(PyKAdminPrincipalObject *self, void *closure) {

    PyObject *value = NULL;

    value = pykadmin_pydatetime_from_timestamp(self->entry.last_pwd_change);

    if (!value) {
        // todo: raise exception
    }

    return value;
}

static PyObject *PyKAdminPrincipal_get_princ_expire_time(PyKAdminPrincipalObject *self, void *closure) {

    PyObject *value = NULL;

    value = pykadmin_pydatetime_from_timestamp(self->entry.princ_expire_time);
    
    if (!value) {
        // todo: raise exception
    }

    return value;
}

static PyObject *PyKAdminPrincipal_get_pw_expiration(PyKAdminPrincipalObject *self, void *closure) {

    PyObject *value = NULL;

    value = pykadmin_pydatetime_from_timestamp(self->entry.pw_expiration);
    
    if (!value) {
        // todo: raise exception
    }

    return value;
}

static PyObject *PyKAdminPrincipal_get_mod_date(PyKAdminPrincipalObject *self, void *closure) {

    PyObject *value = NULL;

    value = pykadmin_pydatetime_from_timestamp(self->entry.mod_date);
    
    if (!value) {
        // todo: raise exception
    }

    return value;
}

static PyObject *PyKAdminPrincipal_get_last_success(PyKAdminPrincipalObject *self, void *closure) {

    PyObject *value = NULL;

    value = pykadmin_pydatetime_from_timestamp(self->entry.last_success);
    
    if (!value) {
        // todo: raise exception
    }

    return value;
}

static PyObject *PyKAdminPrincipal_get_last_failed(PyKAdminPrincipalObject *self, void *closure) {

    PyObject *value = NULL;

    value = pykadmin_pydatetime_from_timestamp(self->entry.last_failed);
    
    if (!value) {
        // todo: raise exception
    }

    return value;
}

static PyObject *PyKAdminPrincipal_get_max_renewable_life(PyKAdminPrincipalObject *self, void *closure) {

    PyDateTime_IMPORT;

    PyObject *value = NULL;

    value = PyDelta_FromDSU(0, self->entry.max_renewable_life, 0);
    
    if (!value) {
        // todo: raise exception
    }

    return value;
}




static PyGetSetDef KAdminPrincipal_getters_setters[] = {

    {"principal", (getter)PyKAdminPrincipal_get_principal,  NULL, "Kerberos Principal", NULL},
    {"name",      (getter)PyKAdminPrincipal_get_principal,  NULL, "Kerberos Principal", NULL},
    {"mod_name",  (getter)PyKAdminPrincipal_get_mod_name,   NULL, "Kerberos Principal", NULL},

    {"last_password_change", (getter)PyKAdminPrincipal_get_last_pwd_change,    NULL, "Kerberos Principal", NULL},
    {"expire_time",          (getter)PyKAdminPrincipal_get_princ_expire_time,  NULL, "Kerberos Principal", NULL},
    {"password_expiration",  (getter)PyKAdminPrincipal_get_pw_expiration,      NULL, "Kerberos Principal", NULL},
    {"mod_date",             (getter)PyKAdminPrincipal_get_mod_date,           NULL, "Kerberos Principal", NULL},
    {"last_success",         (getter)PyKAdminPrincipal_get_last_success,       NULL, "Kerberos Principal", NULL},
    {"last_failure",         (getter)PyKAdminPrincipal_get_last_failed,        NULL, "Kerberos Principal", NULL},

    {"max_renewable_life",   (getter)PyKAdminPrincipal_get_max_renewable_life, NULL, "Kerberos Principal", NULL},

    {NULL, NULL, NULL, NULL, NULL}
};



PyTypeObject PyKAdminPrincipalObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "kadmin.Principal",             /*tp_name*/
    sizeof(PyKAdminPrincipalObject),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)KAdminPrincipal_dealloc, /*tp_dealloc*/
    (printfunc)KAdminPrincipal_print,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0, //PyKAdminPrincipal_str,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /*tp_flags*/
    "KAdminPrincipal objects",           /* tp_doc */
    0,                     /* tp_traverse */
    0,                     /* tp_clear */
    PyKAdminPrincipal_RichCompare,                     /* tp_richcompare */
    0,                     /* tp_weaklistoffset */
    0,                     /* tp_iter */
    0,                     /* tp_iternext */
    KAdminPrincipal_methods,             /* tp_methods */
    KAdminPrincipal_members,             /* tp_members */
    KAdminPrincipal_getters_setters,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)KAdminPrincipal_init,      /* tp_init */
    0,                         /* tp_alloc */
    KAdminPrincipal_new,                 /* tp_new */
};


PyKAdminPrincipalObject *PyKAdminPrincipalObject_principal_with_name(PyKAdminObject *kadmin, char *client_name) {

    PyKAdminPrincipalObject *principal = (PyKAdminPrincipalObject *)KAdminPrincipal_new(&PyKAdminPrincipalObject_Type, NULL, NULL);

    if (principal) {

        Py_XINCREF(kadmin);
        principal->kadmin = kadmin;

        /* todo : fetch kadmin entry */
        PyObject *result = _KAdminPrincipal_load_principal(principal, client_name);

        if (!result) {
            //Py_XDECREF(kadmin); // this is redundant as dealloc decrementes the reference for us
            Py_XINCREF(Py_None);
            KAdminPrincipal_dealloc(principal);
            principal = (PyKAdminPrincipalObject *)Py_None;
        }

    }

    return principal;
}

PyKAdminPrincipalObject *PyKAdminPrincipalObject_principal_with_db_entry(PyKAdminObject *kadmin, krb5_db_entry *kdb) {

    kadm5_ret_t retval = KADM5_OK;

    PyKAdminPrincipalObject *principal = (PyKAdminPrincipalObject *)KAdminPrincipal_new(&PyKAdminPrincipalObject_Type, NULL, NULL);

    if (kdb) {

        Py_XINCREF(kadmin);
        principal->kadmin = kadmin;

        retval = pykadmin_kadm_from_kdb(kadmin, kdb, &principal->entry, KADM5_PRINCIPAL_NORMAL_MASK);

        if (retval) {

            KAdminPrincipal_dealloc(principal);
            
            // todo: set exception
            principal = NULL;

        } 
    }

    return principal;
}

void KAdminPrincipal_destroy(PyKAdminPrincipalObject *self) {
    KAdminPrincipal_dealloc(self);
}



