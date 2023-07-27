#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <jni.h>

#include <ZWayLib.h>
#include <ZLogging.h>

// wrapper for unused variables
#ifdef __GNUC__
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_ ## x
#endif

#define JNIT_CLASS "me/zwave/zway/ZWay"
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
    jmethodID statusCallbackID;
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

static void statusCallback(const ZWay zway, ZWBOOL result, void *jarg);
static void successCallback(const ZWay zway, ZWBYTE funcId, void *jarg);
static void failureCallback(const ZWay zway, ZWBYTE funcId, void *jarg);
static void dataCallback(const ZWay zway, ZWDataChangeType type, ZDataHolder dh, void *jarg);
static void deviceCallback(const ZWay zway, ZWDeviceChangeType type, ZWNODE node_id, ZWBYTE instance_id, ZWBYTE command_class_id, void *jarg);
static void terminateCallback(const ZWay zway, void* arg);

// TODO: use an appropriate callback for java - terminator_callback
// TODO: stdout (logger)
// TODO: add loglevel
static jlong jni_zway_init(JNIEnv *env, jobject obj, jstring name, jstring port, jint speed, jstring config_folder, jstring translations_folder, jstring zddx_folder) {
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
    jmethodID statusCallbackID = (*env)->GetMethodID(env, cls, "statusCallback", "(ZLjava/lang/Object;)V");
    jmethodID deviceCallbackID = (*env)->GetMethodID(env, cls, "deviceCallback", "(IIII)V");
    jmethodID terminateCallbackID = (*env)->GetMethodID(env, cls, "terminateCallback", "()V");
    if (statusCallbackID == 0 || deviceCallbackID == 0 || terminateCallbackID == 0) {
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
    jzway->statusCallbackID = statusCallbackID;
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

    return (jlong)(uintptr_t)jzway;
}

static void jni_finalize(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong ptr) {
    free((JZWay)(uintptr_t)ptr);
}


static void jni_discover(JNIEnv *env, jobject UNUSED(obj), jlong ptr) {
    JZWay jzway = (JZWay)(uintptr_t)ptr;

    ZWError err = zway_discover(jzway->zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_stop(JNIEnv *env, jobject UNUSED(obj), jlong ptr) {
    JZWay jzway = (JZWay)(uintptr_t)ptr;

    ZWError err = zway_stop(jzway->zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jboolean jni_is_idle(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong jzway) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    return (jboolean)zway_is_idle(zway);
}

static jboolean jni_is_running(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong jzway) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    return (jboolean)zway_is_running(zway);
}

static void jni_add_node_to_network(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jboolean startStop) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_add_node_to_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_remove_node_from_network(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jboolean startStop) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_remove_node_from_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_controller_change(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jboolean startStop) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_change(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_set_suc_node_id(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint node_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_set_suc_node_id(zway, node_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_set_sis_node_id(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint node_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_set_sis_node_id(zway, node_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_disable_suc_node_id(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint node_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_disable_suc_node_id(zway, node_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_set_default(JNIEnv *env, jobject UNUSED(obj), jlong jzway) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_set_default(zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_request_network_update(JNIEnv *env, jobject UNUSED(obj), jlong jzway) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    int node_id;
    int suc_node_id;

    ZWError err;

    zdata_acquire_lock(ZDataRoot(zway));
    err = zdata_get_integer(zway_find_controller_data(zway, "nodeId"), &node_id);
    zdata_release_lock(ZDataRoot(zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }

    zdata_acquire_lock(ZDataRoot(zway));
    err = zdata_get_integer(zway_find_controller_data(zway, "SUCNodeId"), &suc_node_id);
    zdata_release_lock(ZDataRoot(zway));

    if (err != NoError) {
       JNI_THROW_EXCEPTION();
   }

   if (suc_node_id != 0 && node_id != suc_node_id) {
        ZWError err = zway_controller_set_default(zway);

        if (err != NoError) {
            JNI_THROW_EXCEPTION();
        }
   }
}

static void jni_set_learn_mode(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jboolean startStop) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_controller_set_learn_mode(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jintArray jni_backup(JNIEnv *env, jobject UNUSED(obj), jlong jzway) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWBYTE *backupData;
    size_t backupSize;

    ZWError err = zway_controller_config_save(zway, &backupData, &backupSize);

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    jintArray backup = (*env)->NewIntArray(env, (jsize)backupSize);
    jint fill[backupSize];
    for (size_t i = 0; i < backupSize; i++) {
        fill[i] = backupData[i];
    }
    (*env)->SetIntArrayRegion(env, backup, 0, backupSize, fill);
    return backup;
}

static void jni_restore(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jintArray data, jboolean full) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    jint *c_int_data = (*env)->GetIntArrayElements(env, data, NULL);
    jsize length = (*env)->GetArrayLength(env, data);

    ZWBYTE *c_byte_data = (ZWBYTE *)malloc(length);

    for (int i = 0; i < length; i++) {
        c_byte_data[i] = (ZWBYTE)c_int_data[i];
    }

    ZWError err = zway_controller_config_restore(zway, (const ZWBYTE *)c_byte_data, length, full);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_node_provisioning_dsk_add(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jintArray dsk) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    jint *c_int_dsk = (*env)->GetIntArrayElements(env, dsk, NULL);
    jsize length = (*env)->GetArrayLength(env, dsk);

    ZWBYTE *c_byte_dsk = (ZWBYTE *)malloc(length);

    for (int i = 0; i < length; i++) {
        c_byte_dsk[i] = (ZWBYTE)c_int_dsk[i];
    }

    ZWError err = zway_node_provisioning_dsk_add(zway, length, (const ZWBYTE *)c_byte_dsk, NULL);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_node_provisioning_dsk_remove(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jintArray dsk) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    jint *c_int_dsk = (*env)->GetIntArrayElements(env, dsk, NULL);
    jsize length = (*env)->GetArrayLength(env, dsk);

    ZWBYTE *c_byte_dsk = (ZWBYTE *)malloc(length);

    for (int i = 0; i < length; i++) {
        c_byte_dsk[i] = (ZWBYTE)c_int_dsk[i];
    }

    ZWError err = zway_node_provisioning_dsk_remove(zway, length, (const ZWBYTE *)c_byte_dsk);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_send_nop(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint node_id, jobject arg) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    JArg jarg = (JArg)malloc(sizeof(struct JArg));
    jarg->jzway = (JZWay)(uintptr_t)jzway;
    jarg->arg = (void *)((*env)->NewGlobalRef(env, arg));

    ZWError err = zway_device_send_nop(zway, (ZWNODE)node_id, (ZJobCustomCallback) successCallback, (ZJobCustomCallback) failureCallback, (void*)jarg);

    if (err != NoError) {
        free(jarg);
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_awake_queue(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong jzway, jint node_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    zway_device_awake_queue((const ZWay)zway, (ZWNODE)node_id);
}

static void jni_device_interview_force(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_interview_force((const ZWay)zway, (ZWNODE)device_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jboolean jni_device_is_interview_done(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong jzway, jint device_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWBOOL ret = zway_device_is_interview_done((const ZWay)zway, (ZWNODE)device_id);

    return (jboolean)ret;
}

static void jni_device_delay_communication(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id, jint delay) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_delay_communication(zway, (ZWNODE)device_id, delay);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_assign_return_route(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id, jint node_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_assign_return_route((const ZWay)zway, (ZWNODE) device_id, (ZWNODE) node_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_assign_priority_return_route(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id, jint node_id, jint repeater1, jint repeater2, jint repeater3, jint repeater4) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_assign_priority_return_route((const ZWay)zway, (ZWNODE) device_id, (ZWNODE) node_id, (ZWBYTE) repeater1, (ZWBYTE) repeater2, (ZWBYTE) repeater3, (ZWBYTE) repeater4);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_delete_return_route(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_delete_return_route((const ZWay)zway, (ZWNODE) device_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_assign_suc_return_route(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_assign_suc_return_route((const ZWay)zway, (ZWNODE) device_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_assign_priority_suc_return_route(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id, jint repeater1, jint repeater2, jint repeater3, jint repeater4) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_assign_priority_suc_return_route((const ZWay)zway, (ZWNODE) device_id, (ZWBYTE) repeater1, (ZWBYTE) repeater2, (ZWBYTE) repeater3, (ZWBYTE) repeater4);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_device_delete_suc_return_route(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_device_delete_suc_return_route((const ZWay)zway, (ZWNODE) device_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_command_interview(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint device_id, jint instance_id, jint cc_id) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zway_command_interview((const ZWay)zway, (ZWNODE) device_id, (ZWBYTE) instance_id, (ZWBYTE) cc_id);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zddx_save_to_xml(JNIEnv *env, jobject UNUSED(obj), jlong jzway) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    ZWError err = zddx_save_to_xml((const ZWay)zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

// zdata functions

static jlong jni_zdata_find(JNIEnv *env, jobject UNUSED(obj), jlong dh, jstring path, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)(uintptr_t)jzway;
    jzdata->self = NULL;

    const char *str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zdata_find(((JZData)(uintptr_t)dh)->dh, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)(uintptr_t)jzdata;
}

static jlong jni_zdata_controller_find(JNIEnv *env, jobject UNUSED(obj), jstring path, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)(uintptr_t)jzway;
    jzdata->self = NULL;

    const char *str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_controller_data(jzdata->jzway->zway, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)(uintptr_t)jzdata;
}

static jlong jni_zdata_device_find(JNIEnv *env, jobject UNUSED(obj), jstring path, jint device_id, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)(uintptr_t)jzway;
    jzdata->self = NULL;

    const char *str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_device_data(jzdata->jzway->zway, device_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)(uintptr_t)jzdata;
}

static jlong jni_zdata_instance_find(JNIEnv *env, jobject UNUSED(obj), jstring path, jint device_id, jint instance_id, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)(uintptr_t)jzway;
    jzdata->self = NULL;

    const char *str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_device_instance_data(jzdata->jzway->zway, device_id, instance_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)(uintptr_t)jzdata;
}

static jlong jni_zdata_command_class_find(JNIEnv *env, jobject UNUSED(obj), jstring path, jint device_id, jint instance_id, jint command_class_id, jlong jzway) {
    JZData jzdata = (JZData)malloc(sizeof(struct JZData));
    jzdata->jzway = (JZWay)(uintptr_t)jzway;
    jzdata->self = NULL;

    const char *str_path = (*env)->GetStringUTFChars(env, path, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    jzdata->dh = zway_find_device_instance_cc_data(jzdata->jzway->zway, device_id, instance_id, command_class_id, str_path);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseStringUTFChars(env, path, str_path);

    return (jlong)(uintptr_t)jzdata;
}

static void jni_zdata_add_callback(JNIEnv *env, jobject obj, jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;
    
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

static void jni_zdata_remove_callback(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_remove_callback(jzdata->dh, (ZDataChangeCallback)&dataCallback);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jstring jni_zdata_get_name(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    const char *str_name;

    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    str_name = zdata_get_name(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    return (*env)->NewStringUTF(env, str_name);
}

static jstring jni_zdata_get_path(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    const char *str_name;

    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    str_name = zdata_get_path(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    return (*env)->NewStringUTF(env, str_name);
}

static jint jni_zdata_get_type(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    ZWDataType type;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_get_type(jzdata->dh, &type);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    return (jint)type;
}

static jlongArray jni_zdata_get_children(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    int length = 0;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));

    ZDataIterator child = zdata_first_child((const ZDataHolder)jzdata->dh);

    while (child != NULL) {
        length++;
        child = zdata_next_child(child);
    }

    jlong fill[length];
    JZData jzchild = (JZData)malloc(sizeof(struct JZData));
        
    child = zdata_first_child((const ZDataHolder)jzdata->dh);
    for (int i = 0; i < length; i++) {
        jzchild->jzway = ((JZData)(uintptr_t)dh)->jzway;
        jzchild->self = ((JZData)(uintptr_t)dh)->self;
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

static jlong jni_zdata_get_update_time(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    time_t time = zdata_get_update_time(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    return time;
}

static jlong jni_zdata_get_invalidate_time(JNIEnv *UNUSED(env), jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    time_t time = zdata_get_invalidate_time(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    return time;
}

// zdata value: get by type

static jboolean jni_zdata_get_boolean(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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

static jint jni_zdata_get_integer(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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

static jfloat jni_zdata_get_float(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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

static jstring jni_zdata_get_string(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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

static jintArray jni_zdata_get_binary(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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
    for (size_t i = 0; i < length; i++) {
        fill[i] = ret[i];
    }
    (*env)->SetIntArrayRegion(env, intArray, 0, length, fill);
    return intArray;
}

static jintArray jni_zdata_get_intArray(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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
    for (size_t i = 0; i < length; i++) {
        fill[i] = ret[i];
    }
    (*env)->SetIntArrayRegion(env, intArray, 0, length, fill);
    return intArray;
}

static jfloatArray jni_zdata_get_floatArray(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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
    for (size_t i = 0; i < length; i++) {
        fill[i] = ret[i];
    }
    (*env)->SetFloatArrayRegion(env, floatArray, 0, length, fill);
    return floatArray;
}

static jobjectArray jni_zdata_get_stringArray(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

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
    for (size_t i = 0; i < length; i++) {
        (*env)->SetObjectArrayElement(env, stringArray, i, (*env)->NewStringUTF(env, (char*)ret[i]));
    }

    return stringArray;
}

// zdata value: set by type

static void jni_zdata_set_empty(JNIEnv *env, jobject UNUSED(obj), jlong dh) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_empty(jzdata->dh);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}


static void jni_zdata_set_boolean(JNIEnv *env, jobject UNUSED(obj), jlong dh, jboolean data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_boolean(jzdata->dh, data);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_integer(JNIEnv *env, jobject UNUSED(obj), jlong dh, jint data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_integer(jzdata->dh, data);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_float(JNIEnv *env, jobject UNUSED(obj), jlong dh, jfloat data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_float(jzdata->dh, data);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_string(JNIEnv *env, jobject UNUSED(obj), jlong dh, jstring data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    const char *str_data = (*env)->GetStringUTFChars(env, data, JNI_FALSE);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_string(jzdata->dh, (ZWCSTR) str_data, TRUE);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_binary(JNIEnv *env, jobject UNUSED(obj), jlong dh, jintArray data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    jint *c_int_data = (*env)->GetIntArrayElements(env, data, NULL);
    jsize length = (*env)->GetArrayLength(env, data);
    
    ZWBYTE *c_byte_data = (ZWBYTE *)malloc(length);
    
    for (int i = 0; i < length; i++) {
        c_byte_data[i] = (ZWBYTE)c_int_data[i];
    }
    
    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_binary(jzdata->dh, (const ZWBYTE *)c_byte_data, length, TRUE);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    free(c_byte_data);
    (*env)->ReleaseIntArrayElements(env, data, c_int_data, 0);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_intArray(JNIEnv *env, jobject UNUSED(obj), jlong dh, jintArray data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    jint *cdata = (*env)->GetIntArrayElements(env, data, NULL);
    jsize length = (*env)->GetArrayLength(env, data);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_integer_array(jzdata->dh, cdata, length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseIntArrayElements(env, data, cdata, 0);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_floatArray(JNIEnv *env, jobject UNUSED(obj), jlong dh, jfloatArray data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    jfloat *cdata = (*env)->GetFloatArrayElements(env, data, NULL);
    jsize length = (*env)->GetArrayLength(env, data);

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_float_array(jzdata->dh, cdata, length);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    (*env)->ReleaseFloatArrayElements(env, data, cdata, 0);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_zdata_set_stringArray(JNIEnv *env, jobject UNUSED(obj), jlong dh, jobjectArray data) {
    JZData jzdata = (JZData)(uintptr_t)dh;

    jsize length = (*env)->GetArrayLength(env, data);

    ZWSTR cdata[length];
    jstring str[length];
    
    for (int i = 0; i < length; i++) {
        str[i] = (*env)->GetObjectArrayElement(env, data, i);
        cdata[i] = (ZWSTR) (*env)->GetStringUTFChars(env, str[i], JNI_FALSE);
    }

    zdata_acquire_lock(ZDataRoot(jzdata->jzway->zway));
    ZWError err = zdata_set_string_array(jzdata->dh, (ZWCSTR*) cdata, length, TRUE);
    zdata_release_lock(ZDataRoot(jzdata->jzway->zway));

    for (int i = 0; i < length; i++) {
        (*env)->ReleaseStringUTFChars(env, str[i], cdata[i]);
    }

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

// Callback stubs

static void successCallback(const ZWay zway, ZWBYTE UNUSED(funcId), void *jarg) {
    statusCallback(zway, TRUE, jarg);
}

static void failureCallback(const ZWay zway, ZWBYTE UNUSED(funcId), void *jarg) {
    statusCallback(zway, FALSE, jarg);
}

static void statusCallback(const ZWay UNUSED(zway), ZWBOOL result, void *jarg) {
    JZWay jzway = ((JArg)jarg)->jzway;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->statusCallbackID, result, ((JArg)jarg)->arg);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);

    free(jarg);
}

static void dataCallback(const ZWay UNUSED(zway), ZWDataChangeType type, ZDataHolder UNUSED(dh), void *arg) {
    JZData jzdata = (JZData)arg;
    JNIEnv* env;
    (*(jzdata->jzway->jvm))->AttachCurrentThread(jzdata->jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzdata->self, jzdata->jzway->dataCallbackID, type);
    (*(jzdata->jzway->jvm))->DetachCurrentThread(jzdata->jzway->jvm);
}

static void deviceCallback(const ZWay UNUSED(zway), ZWDeviceChangeType type, ZWNODE node_id, ZWBYTE instance_id, ZWBYTE command_class_id, void *arg) {
    JZWay jzway = (JZWay)(uintptr_t)arg;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->deviceCallbackID, (int)type, (int)node_id, (int)instance_id, (int)command_class_id);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);
}

static void terminateCallback(const ZWay UNUSED(zway), void* arg) {
    JZWay jzway = (JZWay)(uintptr_t)arg;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->self, jzway->terminateCallbackID, NULL);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);

    jzway->zway = NULL; // to prevent further crashes in libzway calls
}

// Function Classes methods

/* BEGIN AUTOGENERATED FC TEMPLATE
static void jni_fc_%function_short_name%(JNIEnv *env, jobject UNUSED(obj), jlong jzway, %params_jni_declarations%jobject arg) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    JArg jarg = (JArg)malloc(sizeof(struct JArg));
    jarg->jzway = (JZWay)(uintptr_t)jzway;
    jarg->arg = (void *)((*env)->NewGlobalRef(env, arg));

    %params_parser%
    
    ZWError err = %function_name%(zway, %params_c%(ZJobCustomCallback) successCallback, (ZJobCustomCallback) failureCallback, (void*)jarg);
    if (err != NoError) {
        free(jarg);
        JNI_THROW_EXCEPTION();
    }
    
    %params_parser_release%
}

 * END AUTOGENERATED FC TEMPLATE */

// Command Classes methods

/* BEGIN AUTOGENERATED CC TEMPLATE
 * BEGIN AUTOGENERATED CMD TEMPLATE
static void jni_cc_%function_short_name%(JNIEnv *env, jobject UNUSED(obj), jlong jzway, jint deviceId, jint instanceId, %params_jni_declarations%jobject arg) {
    ZWay zway = ((JZWay)(uintptr_t)jzway)->zway;

    JArg jarg = (JArg)malloc(sizeof(struct JArg));
    jarg->jzway = (JZWay)(uintptr_t)jzway;
    jarg->arg = (void *)((*env)->NewGlobalRef(env, arg));

    %params_parser%
    
    ZWError err = %function_name%(zway, deviceId, instanceId, %params_c%(ZJobCustomCallback) successCallback, (ZJobCustomCallback) failureCallback, (void*)jarg);
    if (err != NoError) {
        free(jarg);
        JNI_THROW_EXCEPTION();
    }
    
    %params_parser_release%
}

 * END AUTOGENERATED CMD TEMPLATE
 * END AUTOGENERATED CC TEMPLATE */

static JNINativeMethod funcs[] = {
    { "jni_zwayInit", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)J", (void *)&jni_zway_init },
    { "jni_finalize", "(J)V", (void *)&jni_finalize },
    { "jni_discover", "(J)V", (void *)&jni_discover },
    { "jni_stop", "(J)V", (void *)&jni_stop },
    { "jni_isRunning", "(J)Z", (void *)&jni_is_running },
    { "jni_isIdle", "(J)Z", (void *)&jni_is_idle },
    { "jni_addNodeToNetwork", "(JZ)V", (void *)&jni_add_node_to_network },
    { "jni_removeNodeFromNetwork", "(JZ)V", (void *)&jni_remove_node_from_network },
    { "jni_controllerChange", "(JZ)V", (void *)&jni_controller_change },
    { "jni_setSUCNodeId", "(JI)V", (void *)&jni_set_suc_node_id },
    { "jni_setSISNodeId", "(JI)V", (void *)&jni_set_sis_node_id },
    { "jni_disableSUCNodeId", "(JI)V", (void *)&jni_disable_suc_node_id },
    { "jni_setDefault", "(J)V", (void *)&jni_set_default },
    { "jni_requestNetworkUpdate", "(J)V", (void *)&jni_request_network_update },
    { "jni_setLearnMode", "(JZ)V", (void *)&jni_set_learn_mode },
    { "jni_backup", "(J)[I", (void *)&jni_backup },
    { "jni_restore", "(J[IZ)V", (void *)&jni_restore },
    { "jni_nodeProvisioningDSKAdd", "(J[I)V", (void *)&jni_node_provisioning_dsk_add },
    { "jni_nodeProvisioningDSKRemove", "(J[I)V", (void *)&jni_node_provisioning_dsk_remove },
    { "jni_deviceSendNOP", "(JILjava/lang/Object;)V", (void *)&jni_device_send_nop },
    { "jni_deviceAwakeQueue", "(JI)V", (void *)&jni_device_awake_queue },
    { "jni_deviceInterviewForce", "(JI)V", (void *)&jni_device_interview_force },
    { "jni_deviceIsInterviewDone", "(JI)Z", (void *)&jni_device_is_interview_done },
    { "jni_deviceDelayCommunication", "(JII)V", (void *)&jni_device_delay_communication },
    { "jni_deviceAssignReturnRoute", "(JII)V", (void *)&jni_device_assign_return_route },
    { "jni_deviceAssignPriorityReturnRoute", "(JIIIIII)V", (void *)&jni_device_assign_priority_return_route },
    { "jni_deviceDeleteReturnRoute", "(JI)V", (void *)&jni_device_delete_return_route },
    { "jni_deviceAssignSUCReturnRoute", "(JI)V", (void *)&jni_device_assign_suc_return_route },
    { "jni_deviceAssignPrioritySUCReturnRoute", "(JIIIII)V", (void *)&jni_device_assign_priority_suc_return_route },
    { "jni_deviceDeleteSUCReturnRoute", "(JI)V", (void *)&jni_device_delete_suc_return_route },
    { "jni_commandInterview", "(JIII)V", (void *)&jni_command_interview },
    { "jni_ZDDXSaveToXML", "(J)V", (void *)&jni_zddx_save_to_xml },
    { "jni_zdataFind", "(JLjava/lang/String;J)J", (void *)&jni_zdata_find },
    { "jni_zdataControllerFind", "(Ljava/lang/String;J)J", (void *)&jni_zdata_controller_find },
    { "jni_zdataDeviceFind", "(Ljava/lang/String;IJ)J", (void *)&jni_zdata_device_find },
    { "jni_zdataInstanceFind", "(Ljava/lang/String;IIJ)J", (void *)&jni_zdata_instance_find },
    { "jni_zdataCommandClassFind", "(Ljava/lang/String;IIIJ)J", (void *)&jni_zdata_command_class_find },
    /* BEGIN AUTOGENERATED FC TEMPLATE
    { "jni_fc_%function_short_camel_case_name%", "(J%params_signature%Ljava/lang/Object;)V", (void *)&jni_fc_%function_short_name% },
     * END AUTOGENERATED FC TEMPLATE */
    /* BEGIN AUTOGENERATED CC TEMPLATE
     * BEGIN AUTOGENERATED CMD TEMPLATE
    { "jni_cc_%function_short_camel_case_name%", "(JII%params_signature%Ljava/lang/Object;)V", (void *)&jni_cc_%function_short_name% },
     * END AUTOGENERATED CMD TEMPLATE
     * END AUTOGENERATED CC TEMPLATE */
};

static JNINativeMethod funcsData[] = {
    { "jni_zdataAddCallback", "(J)V", (void *)&jni_zdata_add_callback },
    { "jni_zdataRemoveCallback", "(J)V", (void *)&jni_zdata_remove_callback },
    { "jni_zdataGetName", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_name },
    { "jni_zdataGetPath", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_path },
    { "jni_zdataGetChildren", "(J)[J", (void *)&jni_zdata_get_children },
    { "jni_zdataGetUpdateTime", "(J)J", (void *)&jni_zdata_get_update_time },
    { "jni_zdataGetInvalidateTime", "(J)J", (void *)&jni_zdata_get_invalidate_time },
    { "jni_zdataGetType", "(J)I", (void *)&jni_zdata_get_type },
    { "jni_zdataGetBoolean", "(J)Z", (void *)&jni_zdata_get_boolean },
    { "jni_zdataGetInteger", "(J)I", (void *)&jni_zdata_get_integer },
    { "jni_zdataGetFloat", "(J)F", (void *)&jni_zdata_get_float },
    { "jni_zdataGetString", "(J)Ljava/lang/String;", (void *)&jni_zdata_get_string },
    { "jni_zdataGetBinary", "(J)[I", (void *)&jni_zdata_get_binary },
    { "jni_zdataGetIntArray", "(J)[I", (void *)&jni_zdata_get_intArray },
    { "jni_zdataGetFloatArray", "(J)[F", (void *)&jni_zdata_get_floatArray },
    { "jni_zdataGetStringArray", "(J)[Ljava/lang/String;", (void *)&jni_zdata_get_stringArray },
    { "jni_zdataSetEmpty", "(J)V", (void *)&jni_zdata_set_empty },
    { "jni_zdataSetBoolean", "(JZ)V", (void *)&jni_zdata_set_boolean },
    { "jni_zdataSetInteger", "(JI)V", (void *)&jni_zdata_set_integer },
    { "jni_zdataSetFloat", "(JF)V", (void *)&jni_zdata_set_float },
    { "jni_zdataSetString", "(JLjava/lang/String;)V", (void *)&jni_zdata_set_string },
    { "jni_zdataSetBinary", "(J[I)V", (void *)&jni_zdata_set_binary },
    { "jni_zdataSetIntArray", "(J[I)V", (void *)&jni_zdata_set_intArray },
    { "jni_zdataSetFloatArray", "(J[F)V", (void *)&jni_zdata_set_floatArray },
    { "jni_zdataSetStringArray", "(J[Ljava/lang/String;)V", (void *)&jni_zdata_set_stringArray },
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* UNUSED(reserved)) {
    JNIEnv *env;
    jclass  cls;
    jint    reg_res;

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

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *UNUSED(reserved)) {
    JNIEnv *env;
    jclass  cls;

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
