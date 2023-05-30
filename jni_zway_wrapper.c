#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include <ZWayLib.h>
#include <ZLogging.h>

#define JNIT_CLASS "ZWay"

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

// Forward declarations

static void successCallback(const ZWay zway, ZWBYTE funcId, void *jarg);
static void failureCallback(const ZWay zway, ZWBYTE funcId, void *jarg);
//static void dataCallback(const ZWay zway, ZWDataChangeType type, void *jarg);
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

/*
static void dataCallback(const ZWay zway, ZWDataChangeType type, void *jarg) {
    (void)zway;

    JZWay jzway = ((JArg)jarg)->jzway;

    JNIEnv* env;
    (*(jzway->jvm))->AttachCurrentThread(jzway->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, jzway->cls, jzway->dataCallbackID, ((JArg)jarg)->arg);
    (*(jzway->jvm))->DetachCurrentThread(jzway->jvm);
}
*/

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

/* BEGIN AUTOGENERATED CC TEMPLATE
static void jni_cc_%function_short_name%(JNIEnv *env, jobject obj, jlong jzway, jint deviceId, jint instanceId, %params_java_declarations%jlong arg) {
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

 * END AUTOGENERATED CC TEMPLATE */

static JNINativeMethod funcs[] = {
	{ "jni_zwayInit", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;J)J", (void *)&jni_zway_init },
	{ "jni_discover", "(J)V", (void *)&jni_discover },
	{ "jni_addNodeToNetwork", "(JZ)V", (void *)&jni_add_node_to_network },
	{ "jni_removeNodeFromNetwork", "(JZ)V", (void *)&jni_remove_node_from_network },
	{ "jni_setDefault", "(J)V", (void *)&jni_set_default },
	{ "jni_isRunning", "(J)Z", (void *)&jni_is_running },
	/* BEGIN AUTOGENERATED CC TEMPLATE
	{ "jni_cc_%function_short_camel_case_name%", "(JII%params_signature%JJJ)V", (void *)&jni_cc_%function_short_name% },
	 * END AUTOGENERATED CC TEMPLATE */
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
