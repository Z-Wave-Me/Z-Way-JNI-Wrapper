#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include <ZWayLib.h>
#include <ZLogging.h>

#define JNIT_CLASS "ZWay"
#define JNIT_CLASS_DATA "ZWay.Data"

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
    jobject cls;
    jmethodID successCallbackID;
    jmethodID failureCallbackID;
    jmethodID dataCallbackID;
    jmethodID deviceCallbackID;
    jmethodID terminateCallbackID;
};
typedef struct JZWay * JZWay;

struct JArg {
    JZWay jzway;
    jobject arg;
};
typedef struct JArg * JArg;

struct JZData {
    ZDataHolder dh;
    JavaVM *jvm;
    jobject cls;
    jmethodID cbk;
    ZWay zway;
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
static void dataCallback(const ZWay zway, ZWDataChangeType type, void *jarg);
static void deviceCallback(const ZWay zway, ZWDeviceChangeType type, ZWNODE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *jarg);
static void terminateCallback(const ZWay zway, void* arg);

// TODO: use an appropriate callback for java - terminator_callback
// TODO: stdout (logger)
// TODO: add loglevel
static jlong jni_zway_init(JNIEnv *env, jobject obj, jstring name, jstring port, jint speed, jstring config_folder, jstring translations_folder, jstring zddx_folder, jlong ternminator_callback) {
    (void)obj;
    (void)env;

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

    jclass cls = (*env)->FindClass(env, JNIT_CLASS);
    jmethodID successCallbackID = (*env)->GetMethodID(env, cls, "successCallback", "(Ljava/lang/Object;)V");
    jmethodID failureCallbackID = (*env)->GetMethodID(env, cls, "failureCallback", "(Ljava/lang/Object;)V");
    jmethodID dataCallbackID = (*env)->GetMethodID(env, cls, "dataCallback", "(ILjava/lang/Object;)V");
    jmethodID deviceCallbackID = (*env)->GetMethodID(env, cls, "deviceCallback", "(IIIILjava/lang/Object;)V");
    jmethodID terminateCallbackID = (*env)->GetMethodID(env, cls, "terminateCallback", "()V");
    if (successCallbackID == 0 || failureCallbackID == 0 || dataCallbackID == 0 || deviceCallbackID == 0 || terminateCallbackID == 0) {
        zway_log(zway, Critical, ZSTR("CallbackID method not found"));
        ZWError err = InvalidArg;
        JNI_THROW_EXCEPTION_RET(0);
    }

    JZWay jzway = (JZWay)malloc(sizeof(struct JZWay));
    jzway->zway = zway;
    (*env)->GetJavaVM(env, &(jzway->jvm));
    jzway->cls = cls;
    jzway->successCallbackID = successCallbackID;
    jzway->failureCallbackID = failureCallbackID;
    jzway->dataCallbackID = dataCallbackID;
    jzway->deviceCallbackID = deviceCallbackID;
    jzway->terminateCallbackID = terminateCallbackID;

    err = zway_start(zway, terminateCallback, (void *)jzway);
    if (err != NoError) {
        zway_terminate(&zway);

        JNI_THROW_EXCEPTION_RET(0);
    }

    err = zway_device_add_callback(zway, DeviceAdded | DeviceRemoved | InstanceAdded | InstanceRemoved | CommandAdded | CommandRemoved | ZDDXSaved, (ZDeviceCallback)&deviceCallback, (void *)jzway);
    if (err != NoError) {
        zway_terminate(&zway);

        JNI_THROW_EXCEPTION_RET(0);
    }

    return (jlong) jzway;
}

static void jni_discover(JNIEnv *env, jobject obj, jlong jzway) {
    (void)obj;
    (void)env;

    ZWay zway = ((JZWay)jzway)->zway;

    ZWError err = zway_discover(zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_add_node_to_network(JNIEnv *env, jobject obj, jlong jzway, jboolean startStop) {
    (void)obj;
    (void)env;

    ZWay zway = ((JZWay)jzway)->zway;

    ZWError err = zway_controller_add_node_to_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_remove_node_from_network(JNIEnv *env, jobject obj, jlong jzway, jboolean startStop) {
    (void)obj;
    (void)env;
    
    ZWay zway = ((JZWay)jzway)->zway;

    ZWError err = zway_controller_remove_node_from_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_set_default(JNIEnv *env, jobject obj, jlong jzway) {
    (void)obj;
    (void)env;

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

    jboolean ret = zway_is_running(zway);

    return ret;
}

static jlong jni_zdata_find(JNIEnv *env, jobject obj, jlong dh, jstring path, jlong jzway) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->zway = ((JZWay)jzway)->zway;

    jclass cls_data = (*env)->FindClass(env, JNIT_CLASS_DATA);
    jmethodID cbkID = (*env)->GetMethodID(env, cls_data, "dataCallback", "(ILjava/lang/Object;)V");

    (*env)->GetJavaVM(env, &(jzdata->jvm));
    jzdata->cbk = cbkID;
    jzdata->cls = cls_data;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    jzdata->dh = zdata_find((ZDataHolder)dh, str_path);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong) jzdata;
}

static jlong jni_zdata_controller_find(JNIEnv *env, jobject obj, jstring path, jlong jzway) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->zway = ((JZWay)jzway)->zway;

    jclass cls_data = (*env)->FindClass(env, JNIT_CLASS_DATA);
    jmethodID cbkID = (*env)->GetMethodID(env, cls_data, "dataCallback", "(ILjava/lang/Object;)V");

    (*env)->GetJavaVM(env, &(jzdata->jvm));
    jzdata->cbk = cbkID;
    jzdata->cls = cls_data;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    jzdata->dh = zway_find_controller_data(jzdata->zway, str_path);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong) jzdata;
}

static jlong jni_zdata_device_find(JNIEnv *env, jobject obj, jstring path, jint device_id, jlong jzway) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->zway = ((JZWay)jzway)->zway;

    jclass cls_data = (*env)->FindClass(env, JNIT_CLASS_DATA);
    jmethodID cbkID = (*env)->GetMethodID(env, cls_data, "dataCallback", "(ILjava/lang/Object;)V");

    (*env)->GetJavaVM(env, &(jzdata->jvm));
    jzdata->cbk = cbkID;
    jzdata->cls = cls_data;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    jzdata->dh = zway_find_device_data(jzdata->zway, device_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong) jzdata;
}

static jlong jni_zdata_instance_find(JNIEnv *env, jobject obj, jstring path, jint device_id, jint instance_id, jlong jzway) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->zway = ((JZWay)jzway)->zway;

    jclass cls_data = (*env)->FindClass(env, JNIT_CLASS_DATA);
    jmethodID cbkID = (*env)->GetMethodID(env, cls_data, "dataCallback", "(ILjava/lang/Object;)V");

    (*env)->GetJavaVM(env, &(jzdata->jvm));
    jzdata->cbk = cbkID;
    jzdata->cls = cls_data;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    jzdata->dh = zway_find_device_instance_data(jzdata->zway, device_id, instance_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong) jzdata;
}

static jlong jni_zdata_command_find(JNIEnv *env, jobject obj, jlong dh, jstring path, jint device_id, jint instance_id, jint command_id, jlong jzway) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->zway = ((JZWay)jzway)->zway;

    jclass cls_data = (*env)->FindClass(env, JNIT_CLASS_DATA);
    jmethodID cbkID = (*env)->GetMethodID(env, cls_data, "dataCallback", "(ILjava/lang/Object;)V");

    (*env)->GetJavaVM(env, &(jzdata->jvm));
    jzdata->cbk = cbkID;
    jzdata->cls = cls_data;

    const char *str_path;
    str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    jzdata->dh = zway_find_device_instance_cc_data(jzdata->zway, device_id, instance_id, command_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong) jzdata;
}

static void jni_zdata_add_callback_ex(JNIEnv *env, jobject obj, jlong dh, jobject arg) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    JDataArg jdata_arg = (JDataArg)malloc(sizeof(struct JDataArg));
    jdata_arg->jzdata = jzdata;
    jdata_arg->arg = arg;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    ZWError err = zdata_add_callback_ex(jzdata->dh, (ZDataChangeCallback)&dataCallback, 0, jdata_arg->arg); // TODO do something with those 0 (ZWBOOL watch_children)
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }

}

static void jni_zdata_remove_callback_ex(JNIEnv *env, jobject obj, jlong dh, jobject arg) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    JDataArg jdata_arg = (JDataArg)malloc(sizeof(struct JDataArg));
    jdata_arg->jzdata = jzdata;
    jdata_arg->arg = arg;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    ZWError err = zdata_remove_callback_ex(jzdata->dh, (ZDataChangeCallback)&dataCallback, jdata_arg->arg);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }

    free(jdata_arg);
}

static jstring jni_zdata_get_name(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    const char *str_name;

    JZData jzdata = (JZData) dh;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    str_name = zdata_get_name(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    return (*env)->NewStringUTF(env, str_name);
}

static jint jni_zdata_get_type(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    ZWDataType type;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    ZWError err = zdata_get_type(jzdata->dh, &type);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    return (jint)type;
}

// ZDATA GET VALUE BY TYPE: BEGIN

static jboolean jni_zdata_get_boolean(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    ZWBOOL ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_boolean(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (jboolean)ret;
}

static jint jni_zdata_get_integer(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    int ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_integer(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (jint)ret;
}

static jfloat jni_zdata_get_float(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    float ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_float(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (jfloat)ret;
}

static jstring jni_zdata_get_string(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    ZWCSTR ret;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_string(jzdata->dh, &ret);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }
    return (*env)->NewStringUTF(env, (char*)ret);
}

static jintArray jni_zdata_get_binary(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    ZWBYTE* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_binary(jzdata->dh, (const ZWBYTE **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jintArray intArray = (*env)->NewIntArray(env, (jsize)length);
    jint fill[length];
    for (int i=0;i<length;i++) {
        fill[i] = ret[i];
    }
    (*env)->SetIntArrayRegion(env, intArray, 0, length, fill);
    return intArray;
}

static jintArray jni_zdata_get_intArray(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    int* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_integer_array(jzdata->dh, (const int **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jintArray intArray = (*env)->NewIntArray(env, (jsize)length);
    jint fill[length];
    for (int i=0;i<length;i++) {
        fill[i] = ret[i];
    }
    (*env)->SetIntArrayRegion(env, intArray, 0, length, fill);
    return intArray;
}

static jfloatArray jni_zdata_get_floatArray(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    float* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_float_array(jzdata->dh, (const float **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jfloatArray floatArray = (*env)->NewFloatArray(env, (jsize)length);
    jfloat fill[length];
    for (int i=0;i<length;i++) {
        fill[i] = ret[i];
    }
    (*env)->SetFloatArrayRegion(env, floatArray, 0, length, fill);
    return floatArray;
}

static jobjectArray jni_zdata_get_stringArray(JNIEnv *env, jobject obj, jlong dh) {
    (void)obj;
    (void)env;

    JZData jzdata = (JZData) dh;

    ZWCSTR* ret;
    size_t length;

    ZWError err = NoError;

    zdata_acquire_lock(ZDataRoot(jzdata->zway));
    err = zdata_get_string_array(jzdata->dh, (const ZWCSTR **)&ret, &length);
    zdata_release_lock(ZDataRoot(jzdata->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jobjectArray stringArray = (*env)->NewObjectArray(env, (jsize)length, (*env)->FindClass(env, "java/lang/String"), (*env)->NewStringUTF(env, ""));
    for (int i=0;i<length;i++) {
        (*env)->SetObjectArrayElement(env, stringArray, length, /*(char*)(str_list_ret[i])*/ (*env)->NewStringUTF(env, (char*)ret[i]));
    }

    return stringArray;
}

// Callback stubs

static void successCallback(const ZWay zway, ZWBYTE funcId, void *jarg) {
    (void)zway;
    (void)funcId;

    JZWay jzway = ((JArg)jarg)->jzway;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->cls, jzway->successCallbackID, ((JArg)jarg)->arg);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);

    free(jarg);
}

static void failureCallback(const ZWay zway, ZWBYTE funcId, void *jarg) {
    (void)zway;
    (void)funcId;

    JZWay jzway = ((JArg)jarg)->jzway;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->cls, jzway->failureCallbackID, ((JArg)jarg)->arg);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);

    free(jarg);
}

static void dataCallback(const ZWay zway, ZWDataChangeType type, void *jdata_arg) {
    (void)zway;

    JZData jzdata = ((JDataArg)jdata_arg)->jzdata;

    JNIEnv* env;
    (*(jzdata->jvm))->AttachCurrentThread(jzdata->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzdata->cls, jzdata->cbk, ((JDataArg)jdata_arg)->arg);
    (*(jzdata->jvm))->DetachCurrentThread(jzdata->jvm);
}

static void deviceCallback(const ZWay zway, ZWDeviceChangeType type, ZWNODE node_id, ZWBYTE instance_id, ZWBYTE command_id, void *arg) {
    (void) zway;

    JZWay jzway = (JZWay) arg;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->cls, jzway->deviceCallbackID, type, node_id, instance_id, command_id);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);
}

static void terminateCallback(const ZWay zway, void* arg) {
    (void) zway;
    
    JZWay jzway = (JZWay)arg;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->cls, jzway->terminateCallbackID, NULL);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);
    
    free(jzway);
    jzway = NULL;
}

// Command Classes methods

// BEGIN AUTOMATED CODE: CC FUNCTIONS
static void jni_cc_switch_binary_set(JNIEnv *env, jobject obj, jlong jzway, jint deviceId, jint instanceId, jboolean value, jint duration, jlong arg) {
    (void)env;
    (void)obj;

    ZWay zway = ((JZWay)jzway)->zway;

    JArg jarg = (JArg)malloc(sizeof(struct JArg));
    jarg->jzway = (JZWay)jzway;
    jarg->arg = (void *)arg;

    ZWError err = zway_cc_switch_binary_set(zway, deviceId, instanceId, value, duration, (ZJobCustomCallback) successCallback, (ZJobCustomCallback) failureCallback, (void*)jarg);
    if (err != NoError) {
        free(jarg);
        JNI_THROW_EXCEPTION();
    }
}
// END AUTOMATED CODE: CC FUNCTIONS

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
	{ "jni_zdataCommandFind", "(Ljava/lang/String;IIIJ)J", (void *)&jni_zdata_command_find },
    { "jni_zdataAddCallbackEx", "(J)V", (void *)&jni_zdata_add_callback_ex },
	{ "jni_zdataRemoveCallbackEx", "(J)V", (void *)&jni_zdata_remove_callback_ex },
	{ "jni_zdataGetName", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_name },
    { "jni_zdataGetType", "(J)I", (void *)&jni_zdata_get_type },
    { "jni_zdataGetBoolean", "(J)Z", (void *)&jni_zdata_get_boolean },
    { "jni_zdataGetInteger", "(J)I", (void *)&jni_zdata_get_integer },
    { "jni_zdataGetFloat", "(J)F", (void *)&jni_zdata_get_float },
    { "jni_zdataGetString", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_string },
    { "jni_zdataGetBinary", "(J)[I", (void *)&jni_zdata_get_binary },
    { "jni_zdataGetIntArray", "(J)[I", (void *)&jni_zdata_get_intArray },
    { "jni_zdataGetFloatArray", "(J)[F", (void *)&jni_zdata_get_floatArray },
    { "jni_zdataGetStringArray", "(J)[Ljava/lang/String;", (void *)&jni_zdata_get_stringArray },
	// BEGIN AUTOMATED CODE: CC FUNCTIONS SIGNATURE
	{ "jni_cc_switchBinarySet", "(JIIZIJJJ)V", (void *)&jni_cc_switch_binary_set } //,
	// END AUTOMATED CODE: CC FUNCTIONS SIGNATURE
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv *env;
	jclass  cls;
	jint    reg_res;

	(void)reserved;

	if ((*vm)->GetEnv(vm, (void **)&env, JNI_VERSION_1_8) != JNI_OK)
		return -1;

	cls = (*env)->FindClass(env, JNIT_CLASS);
	if (cls == NULL)
		return -1;

	reg_res = (*env)->RegisterNatives(env, cls, funcs, sizeof(funcs)/sizeof(funcs[0]));
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

	cls = (*env)->FindClass(env, JNIT_CLASS);
	if (cls == NULL)
		return;

	(*env)->UnregisterNatives(env, cls);
}
