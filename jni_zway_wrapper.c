#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include <ZWayLib.h>
#include <ZLogging.h>

#define JNIT_CLASS "ZWayJNIWrapper/ZWay"
#define JNIT_CLASS_DATA JNIT_CLASS "$Data"

#define JNI_THROW_EXCEPTION_INTERNAL() \
    jclass Exception = (*env)->FindClass(env, "java/lang/Exception"); \
    (*env)->ThrowNew(env, Exception, zstrerror(err)); \

#define JNI_THROW_EXCEPTION() \
    JNI_THROW_EXCEPTION_INTERNAL(); \
    return;

#define JNI_THROW_EXCEPTION_RET(ret) \
    JNI_THROW_EXCEPTION_INTERNAL(); \
    return (ret);

struct JZWay {
    ZWay zway;
    JavaVM *jvm;
    jobject self;
    jmethodID successCallbackID;
    jmethodID failureCallbackID;
    jmethodID deviceCallbackID;
    jmethodID terminateCallbackID;
    jmethodID dataCallbackID;
};
typedef struct JZWay * JZWay;

struct JArg {
    JZWay jzway;
    jobject arg;
};
typedef struct JArg * JArg;

struct JZData {
    ZDataHolder dh;
    jobject self;
    JZWay jzway;
};
typedef struct JZData * JZData;

struct JDataArg {
    JZData jzdata;
    jobject arg;
};
typedef struct JDataArg * JDataArg;

// Forward declarations

static void successCallback(const ZWay zway, ZWBYTE funcId, void *jarg);
static void failureCallback(const ZWay zway, ZWBYTE funcId, void *jarg);
static void dataCallback(const ZWay zway, ZWDataChangeType type, ZDataHolder dh, void *jarg);
static void deviceCallback(const ZWay zway, ZWDeviceChangeType type, ZWNODE node_id, ZWBYTE instance_id, ZWBYTE command_class_id, void *jarg);
static void terminateCallback(const ZWay zway, void* arg);

// TODO: use an appropriate callback for java - terminator_callback
// TODO: stdout (logger)
// TODO: add loglevel
static jlong jni_zway_init(JNIEnv *env, jobject obj, jstring name, jstring port, jint speed, jstring config_folder, jstring translations_folder, jstring zddx_folder, jlong ternminator_callback) {
    (void)obj;

    // Config folders

    const char *str_name;
    const char *str_port;
    const char *str_config_folder;
    const char *str_translations_folder;
    const char *str_zddx_folder;

    str_name  = (*env)->GetStringUTFChars(env, name, JNI_FALSE);
    str_port  = (*env)->GetStringUTFChars(env, port, JNI_FALSE);
    str_config_folder  = (*env)->GetStringUTFChars(env, config_folder, JNI_FALSE);
    str_translations_folder  = (*env)->GetStringUTFChars(env, translations_folder, JNI_FALSE);
    str_zddx_folder  = (*env)->GetStringUTFChars(env, zddx_folder, JNI_FALSE);

    ZWLog logger = zlog_create_syslog(Debug);

    // Init Z-Way

    ZWay zway = NULL;

    ZWError err = zway_init(&zway, str_port, (speed_t)speed, str_config_folder, str_translations_folder, str_zddx_folder, str_name, logger);
    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    (*env)->ReleaseStringUTFChars(env, name, str_name);
    (*env)->ReleaseStringUTFChars(env, port, str_port);
    (*env)->ReleaseStringUTFChars(env, config_folder, str_config_folder);
    (*env)->ReleaseStringUTFChars(env, translations_folder, str_translations_folder);
    (*env)->ReleaseStringUTFChars(env, zddx_folder, str_zddx_folder);

    // Prepare JZWay object

    JZWay jzway = (JZWay)malloc(sizeof(struct JZWay));
    jzway->zway = zway;

    jclass cls = (*env)->FindClass(env, JNIT_CLASS);
    if (cls == 0) {
        zway_log(jzway->zway, Critical, ZSTR(JNIT_CLASS " class not found"));
        ZWError err = InvalidArg;
        JNI_THROW_EXCEPTION_RET(0);
    }
    jmethodID successCallbackID = (*env)->GetMethodID(env, cls, "successCallback", "(Ljava/lang/Object;)V");
    jmethodID failureCallbackID = (*env)->GetMethodID(env, cls, "failureCallback", "(Ljava/lang/Object;)V");
    jmethodID deviceCallbackID = (*env)->GetMethodID(env, cls, "deviceCallback", "(IIII)V");
    jmethodID terminateCallbackID = (*env)->GetMethodID(env, cls, "terminateCallback", "()V");
    if (successCallbackID == 0 || failureCallbackID == 0 || deviceCallbackID == 0 || terminateCallbackID == 0) {
        zway_log(jzway->zway, Critical, ZSTR(JNIT_CLASS " callback ID method not found"));
        ZWError err = InvalidArg;
        JNI_THROW_EXCEPTION_RET(0);
    }

    jclass clsData = (*env)->FindClass(env, JNIT_CLASS_DATA);
    if (clsData == 0) {
        zway_log(jzway->zway, Critical, ZSTR(JNIT_CLASS_DATA " class not found"));
        ZWError err = InvalidArg;
        JNI_THROW_EXCEPTION_RET(0);
    }
    jmethodID dataCallbackID = (*env)->GetMethodID(env, clsData, "dataCallback", "(I)V");
    if (dataCallbackID == 0) {
        zway_log(jzway->zway, Critical, ZSTR(JNIT_CLASS_DATA " callback ID method not found"));
        ZWError err = InvalidArg;
        JNI_THROW_EXCEPTION_RET(0);
    }

    jobject self = (*env)->NewGlobalRef(env, obj);
    
    (*env)->GetJavaVM(env, &(jzway->jvm));
    jzway->self = self;
    jzway->successCallbackID = successCallbackID;
    jzway->failureCallbackID = failureCallbackID;
    jzway->deviceCallbackID = deviceCallbackID;
    jzway->terminateCallbackID = terminateCallbackID;
    jzway->dataCallbackID = dataCallbackID;

    // Set up device callback
    
    err = zway_device_add_callback(jzway->zway, DeviceAdded | DeviceRemoved | InstanceAdded | InstanceRemoved | CommandAdded | CommandRemoved, (ZDeviceCallback)deviceCallback, (void *)jzway);
    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    // Start Z-Way thread
    
    err = zway_start(zway, terminateCallback, (void *)jzway);
    if (err != NoError) {
        zway_terminate(&zway);

        JNI_THROW_EXCEPTION_RET(0);
    }

    return (jlong)jzway;
}

static void jni_discover(JNIEnv *env, jobject obj, jlong ptr) {
    JZWay jzway = (JZWay)ptr;

    ZWError err = zway_discover(jzway->zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_add_node_to_network(JNIEnv *env, jobject obj, jlong jzway, jboolean startStop) {
    (void)obj;

    ZWay zway = ((JZWay)jzway)->zway;

    ZWError err = zway_controller_add_node_to_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_remove_node_from_network(JNIEnv *env, jobject obj, jlong jzway, jboolean startStop) {
    (void)obj;
    
    ZWay zway = ((JZWay)jzway)->zway;

    ZWError err = zway_controller_remove_node_from_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_set_default(JNIEnv *env, jobject obj, jlong jzway) {
    (void)obj;

    ZWay zway = ((JZWay)jzway)->zway;

    ZWError err = zway_controller_set_default(zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jboolean jni_is_running(JNIEnv *env, jobject obj, jlong jzway) {
    (void)obj;
    (void)env;

    ZWay zway = ((JZWay)jzway)->zway;

    return (jboolean)zway_is_running(zway);
}

static jlong jni_zdata_find(JNIEnv *env, jobject obj, jlong dh, jstring path, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)jzway;
    jzdata->self = NULL;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zdata_find(((JZData)dh)->dh, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)jzdata;
}

static jlong jni_zdata_controller_find(JNIEnv *env, jobject obj, jstring path, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)jzway;
    jzdata->self = NULL;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_controller_data(jzdata->jzway->zway, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)jzdata;
}

static jlong jni_zdata_device_find(JNIEnv *env, jobject obj, jstring path, jint device_id, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)jzway;
    jzdata->self = NULL;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_device_data(jzdata->jzway->zway, device_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)jzdata;
}

static jlong jni_zdata_instance_find(JNIEnv *env, jobject obj, jstring path, jint device_id, jint instance_id, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)jzway;
    jzdata->self = NULL;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_device_instance_data(jzdata->jzway->zway, device_id, instance_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)jzdata;
}

static jlong jni_zdata_command_class_find(JNIEnv *env, jobject obj, jstring path, jint device_id, jint instance_id, jint command_class_id, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)jzway;
    jzdata->self = NULL;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_device_instance_cc_data(jzdata->jzway->zway, device_id, instance_id, command_class_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)jzdata;
}

static void jni_zdata_add_callback(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;
    
    if (jzdata->self == NULL) {
        jzdata->self = (*env)->NewGlobalRef(env, obj);
    }
    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_add_callback(jzdata->dh, (ZDataChangeCallback)&dataCallback, 0, jzdata);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_remove_callback(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_remove_callback(jzdata->dh, (ZDataChangeCallback)&dataCallback);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jstring jni_zdata_get_name(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    const char *str_name;

    JZData jzdata = (JZData)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    str_name = zdata_get_name(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    return (*env)->NewStringUTF(env, str_name);
}

static jstring jni_zdata_get_path(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    const char *str_name;

    JZData jzdata = (JZData)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    str_name = zdata_get_path(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    return (*env)->NewStringUTF(env, str_name);
}

static jint jni_zdata_get_type(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    ZWDataType type;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_get_type(jzdata->dh, &type);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    return (jint)type;
}

static jlongArray jni_zdata_get_children(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    int length = 0;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));

    ZDataIterator child = zdata_first_child((const ZDataHolder)jzdata->dh);

    while (child != NULL) {
        length++;
        child = zdata_next_child(child);
    }

    jlong fill[length];
    JZData jzchild;
        
    child = zdata_first_child((const ZDataHolder)jzdata->dh);
    for (int i = 0; i < length; i++) {
        TODO(this is wrong! fill properly jzchild structure)
        jzchild = (JZData)dh;
        jzchild->dh = child->data;
        fill[i] = (long) jzchild;
        child = zdata_next_child(child);
    }
    
    free(child);

    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    jlongArray jchildren = (*env)->NewLongArray(env, (jsize)length);
    (*env)->SetLongArrayRegion(env, jchildren, 0, length, fill);

    return jchildren;
}

// zdata value: get by type

static jboolean jni_zdata_get_boolean(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    ZWBOOL ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_boolean(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (jboolean)ret;
}

static jint jni_zdata_get_integer(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    int ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_integer(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (jint)ret;
}

static jfloat jni_zdata_get_float(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    float ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_float(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (jfloat)ret;
}

static jstring jni_zdata_get_string(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    ZWCSTR ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_string(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (*env)->NewStringUTF(env, (char*)ret);
}

static jintArray jni_zdata_get_binary(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    ZWBYTE* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_binary(jzdata->dh, (const ZWBYTE **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jintArray intArray = (*env)->NewIntArray(env, (jsize)length);
    jint fill[length];
    for (int i = 0; i < length; i++) {
        fill[i] = ret[i];
    }
    (*env)->SetIntArrayRegion(env, intArray, 0, length, fill);
    return intArray;
}

static jintArray jni_zdata_get_intArray(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    int* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_integer_array(jzdata->dh, (const int **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jintArray intArray = (*env)->NewIntArray(env, (jsize)length);
    jint fill[length];
    for (int i = 0; i < length; i++) {
        fill[i] = ret[i];
    }
    (*env)->SetIntArrayRegion(env, intArray, 0, length, fill);
    return intArray;
}

static jfloatArray jni_zdata_get_floatArray(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    float* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_float_array(jzdata->dh, (const float **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jfloatArray floatArray = (*env)->NewFloatArray(env, (jsize)length);
    jfloat fill[length];
    for (int i = 0; i < length; i++) {
        fill[i] = ret[i];
    }
    (*env)->SetFloatArrayRegion(env, floatArray, 0, length, fill);
    return floatArray;
}

static jobjectArray jni_zdata_get_stringArray(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    ZWCSTR* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    err = zdata_get_string_array(jzdata->dh, (const ZWCSTR **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jobjectArray stringArray = (*env)->NewObjectArray(env, (jsize)length, (*env)->FindClass(env, "java/lang/String"), (*env)->NewStringUTF(env, ""));
    for (int i = 0; i < length; i++) {
        (*env)->SetObjectArrayElement(env, stringArray, length, /*(char*)(str_list_ret[i])*/ (*env)->NewStringUTF(env, (char*)ret[i]));
        TODO(jni_zdata_get_stringArray)
    }

    return stringArray;
}

// set by types
/*
static void jni_zdata_set_boolean(JNIEnv *env, jobject obj, jlong dh, jboolean data) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_boolean(jzdata->dh, data);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_integer(JNIEnv *env, jobject obj, jlong dh, jint data) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_integer(jzdata->dh, data);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_float(JNIEnv *env, jobject obj, jlong dh, jfloat data) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_float(jzdata->dh, data);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_string(JNIEnv *env, jobject obj, jlong dh, jstring data, jboolean copy) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    char *str_data = (*env)->GetStringUTFChars(env, data, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_string(jzdata->dh, (ZWCSTR) str_data, copy);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_binary(JNIEnv *env, jobject obj, jlong dh, jintArray data, jint length, jboolean copy) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    int *cdata[length];
    for (int i = 0; i < length; i++) {
        cdata[i] = data[i];
    }

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_binary(jzdata->dh, (const ZWBYTE *)cdata, length, copy);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_int_array(JNIEnv *env, jobject obj, jlong dh, jintArray data, jint length) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    int *cdata[length];
    for (int i = 0; i < length; i++) {
        cdata[i] = data[i];
    }

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_integer_array(jzdata->dh, cdata, length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_float_array(JNIEnv *env, jobject obj, jlong dh, jfloatArray data, jint length) {
    (void)obj;

    JZData jzdata = (JZData)dh;

    float *cdata[length];
    for (int i = 0; i < length; i++) {
        cdata[i] = data[i];
    }

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_float_array(jzdata->dh, cdata, length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_string_array(JNIEnv *env, jobject obj, jlong dh, jobjectArray data, jint length, jboolean copy) {
    (void)obj;

    JZData jzdata = (JZData)dh;
    jstring str;
    char ** cstr;

    char **cdata[length];
    for (int i = 0; i < length; i++) {
        str = *data[i];
        cstr = (*env)->GetStringUTFChars(env, str, JNI_FALSE);
        cdata[i] = (ZWCSTR) cstr;
    }

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_string_array(jzdata->dh, cdata, length, copy);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}
*/

// Callback stubs

static void successCallback(const ZWay zway, ZWBYTE funcId, void *jarg) {
    (void)zway;
    (void)funcId;

    JZWay jzway = ((JArg)jarg)->jzway;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->successCallbackID, ((JArg)jarg)->arg);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);

    free(jarg);
}

static void failureCallback(const ZWay zway, ZWBYTE funcId, void *jarg) {
    (void)zway;
    (void)funcId;

    JZWay jzway = ((JArg)jarg)->jzway;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->failureCallbackID, ((JArg)jarg)->arg);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);

    free(jarg);
}

static void dataCallback(const ZWay zway, ZWDataChangeType type, ZDataHolder dh, void *arg) {
    (void)zway;

    JZData jzdata = (JZData)arg;
    JNIEnv* env;
    (*(jzdata->jzway->jvm))->AttachCurrentThread(jzdata->jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzdata->self, jzdata->jzway->dataCallbackID, type);
    (*(jzdata->jzway->jvm))->DetachCurrentThread(jzdata->jzway->jvm);
}

static void deviceCallback(const ZWay zway, ZWDeviceChangeType type, ZWNODE node_id, ZWBYTE instance_id, ZWBYTE command_class_id, void *arg) {
    (void) zway;

    JZWay jzway = (JZWay)arg;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->deviceCallbackID, (int)type, (int)node_id, (int)instance_id, (int)command_class_id);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);
}

static void terminateCallback(const ZWay zway, void* arg) {
    (void) zway;
    
    JZWay jzway = (JZWay)arg;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->terminateCallbackID, NULL);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);
    
    free(jzway);
    jzway = NULL;
}

// Command Classes methods

/* BEGIN AUTOGENERATED CC TEMPLATE
 * BEGIN AUTOGENERATED CMD TEMPLATE
static void jni_cc_%function_short_name%(JNIEnv *env, jobject obj, jlong jzway, jint deviceId, jint instanceId, %params_jni_declarations%jlong arg) {
    (void)env;
    (void)obj;

    ZWay zway = ((JZWay)jzway)->zway;

    JArg jarg = (JArg)malloc(sizeof(struct JArg));
    jarg->jzway = (JZWay)jzway;
    jarg->arg = (void *)arg;

    ZWError err = %function_name%(zway, deviceId, instanceId, %params%(ZJobCustomCallback) successCallback, (ZJobCustomCallback) failureCallback, (void*)jarg);
    if (err != NoError) {
        free(jarg);
        JNI_THROW_EXCEPTION();
    }
}

 * END AUTOGENERATED CMD TEMPLATE
 * END AUTOGENERATED CC TEMPLATE */

static JNINativeMethod funcs[] = {
    { "jni_zwayInit", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;J)J", (void *)&jni_zway_init },
    { "jni_discover", "(J)V", (void *)&jni_discover },
    { "jni_addNodeToNetwork", "(JZ)V", (void *)&jni_add_node_to_network },
    { "jni_removeNodeFromNetwork", "(JZ)V", (void *)&jni_remove_node_from_network },
    { "jni_setDefault", "(J)V", (void *)&jni_set_default },
    { "jni_isRunning", "(J)Z", (void *)&jni_is_running },
    { "jni_zdataFind", "(JLjava/lang/String;J)J", (void *)&jni_zdata_find },
    { "jni_zdataControllerFind", "(Ljava/lang/String;J)J", (void *)&jni_zdata_controller_find },
    { "jni_zdataDeviceFind", "(Ljava/lang/String;IJ)J", (void *)&jni_zdata_device_find },
    { "jni_zdataInstanceFind", "(Ljava/lang/String;IIJ)J", (void *)&jni_zdata_instance_find },
    { "jni_zdataCommandClassFind", "(Ljava/lang/String;IIIJ)J", (void *)&jni_zdata_command_class_find },
    /* BEGIN AUTOGENERATED CC TEMPLATE
     * BEGIN AUTOGENERATED CMD TEMPLATE
    { "jni_cc_%function_short_camel_case_name%", "(JII%params_signature%JJJ)V", (void *)&jni_cc_%function_short_name% },
     * END AUTOGENERATED CMD TEMPLATE
     * END AUTOGENERATED CC TEMPLATE */
};

static JNINativeMethod funcsData[] = {
    { "jni_zdataAddCallback", "(J)V", (void *)&jni_zdata_add_callback },
    { "jni_zdataRemoveCallback", "(J)V", (void *)&jni_zdata_remove_callback },
    { "jni_zdataGetName", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_name },
    { "jni_zdataGetPath", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_path },
    { "jni_zdataGetChildren", "(J)[J", (void *)&jni_zdata_get_children },
    { "jni_zdataGetType", "(J)I", (void *)&jni_zdata_get_type },
    { "jni_zdataGetBoolean", "(J)Z", (void *)&jni_zdata_get_boolean },
    { "jni_zdataGetInteger", "(J)I", (void *)&jni_zdata_get_integer },
    { "jni_zdataGetFloat", "(J)F", (void *)&jni_zdata_get_float },
    { "jni_zdataGetString", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_string },
    { "jni_zdataGetBinary", "(J)[I", (void *)&jni_zdata_get_binary },
    { "jni_zdataGetIntArray", "(J)[I", (void *)&jni_zdata_get_intArray },
    { "jni_zdataGetFloatArray", "(J)[F", (void *)&jni_zdata_get_floatArray },
    { "jni_zdataGetStringArray", "(J)[Ljava/lang/String;", (void *)&jni_zdata_get_stringArray },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv *env;
    jclass  cls;
    jint    reg_res;

    (void)reserved;

    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_8) != JNI_OK)
        return -1;

    // Z-Way methods

    cls = (*env)->FindClass(env, JNIT_CLASS);
    if (cls == NULL)
        return -1;

    reg_res = (*env)->RegisterNatives(env, cls, funcs, sizeof(funcs)/sizeof(funcs[0]));
    if (reg_res != 0)
        return -1;

    // Data methods
    
    cls = (*env)->FindClass(env, JNIT_CLASS_DATA);
    if (cls == NULL)
        return -1;

    reg_res = (*env)->RegisterNatives(env, cls, funcsData, sizeof(funcsData)/sizeof(funcsData[0]));
    if (reg_res != 0)
        return -1;

    return JNI_VERSION_1_8;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    jclass  cls;

    (void)reserved;

    if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_8) != JNI_OK)
        return;

    // Z-Way methods
    
    cls = (*env)->FindClass(env, JNIT_CLASS);
    if (cls == NULL)
        return;

    (*env)->UnregisterNatives(env, cls);
    
    // Data methods
    
    cls = (*env)->FindClass(env, JNIT_CLASS_DATA);
    if (cls == NULL)
        return;

    (*env)->UnregisterNatives(env, cls);
}
