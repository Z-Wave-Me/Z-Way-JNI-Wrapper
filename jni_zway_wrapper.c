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

// TODO: use an appropriate callback for java - terminator_callback
// TODO: stdout (logger)
// TODO: add loglevel
static jlong jni_zway_init(JNIEnv *env, jobject obj, jstring name, jstring port, jint speed, jstring config_folder, jstring translations_folder, jstring zddx_folder, jlong ternminator_callback) {
    (void)obj;
    (void)env;

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

    ZWay zway = NULL;

    ZWError err = zway_init(&zway, str_port, (speed_t)speed, str_config_folder, str_translations_folder, str_zddx_folder, str_name, logger);

    if (err != NoError) {
        JNI_THROW_EXCEPTION_RET(0);
    }

    err = zway_start(zway, /*ZWaveContext::TerminationCallbackStub*/ NULL, /*context*/ NULL);

    if (err != NoError) {
        zway_terminate(&zway);

        JNI_THROW_EXCEPTION_RET(0);
    }

    (*env)->ReleaseStringUTFChars(env, name, str_name);
    (*env)->ReleaseStringUTFChars(env, port, str_port);
    (*env)->ReleaseStringUTFChars(env, config_folder, str_config_folder);
    (*env)->ReleaseStringUTFChars(env, translations_folder, str_translations_folder);
    (*env)->ReleaseStringUTFChars(env, zddx_folder, str_zddx_folder);

    return (jlong)zway;
}

static void jni_discover(JNIEnv *env, jobject obj, jlong ptr) {
    (void)obj;
    (void)env;

    ZWay zway = (ZWay) ptr;

    ZWError err = zway_discover(zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_addNodeToNetwork(JNIEnv *env, jobject obj, jlong ptr, jboolean startStop) {
    (void)obj;
    (void)env;

    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_add_node_to_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_removeNodeFromNetwork(JNIEnv *env, jobject obj, jlong ptr, jboolean startStop) {
    (void)obj;
    (void)env;
    
    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_remove_node_from_network(zway, startStop);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_setDefault(JNIEnv *env, jobject obj, jlong ptr) {
    (void)obj;
    (void)env;

    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_set_default(zway);

    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static jboolean jni_isRunning(JNIEnv *env, jobject obj, jlong ptr) {
    (void)obj;
    (void)env;

    ZWay zway = (ZWay) ptr;

    jboolean ret = zway_is_running(zway);

    return ret;
}

typedef struct jni_callback_data {
    JavaVM *jvm;
    jobject cls;
    jmethodID method;
    jobject arg;
} jni_callback_data;

// Callback stub
// TODO(change type to ZJobCustomCallback and fix warning from GCC)
static void callback_stub(const ZWay zway, ZWBYTE funcId, void *arg) {
    (void)funcId;

    jni_callback_data *cbkData = (jni_callback_data *) arg;

    JNIEnv* env;
    (*(cbkData->jvm))->AttachCurrentThread(cbkData->jvm, (void**) &env, NULL);
    (*env)->CallVoidMethod(env, cbkData->cls, cbkData->method); //, cbkData->arg
    (*(cbkData->jvm))->DetachCurrentThread(cbkData->jvm);  

    free(arg);
}

// TODO callbackArg
static void jni_cc_switchBinarySet(JNIEnv *env, jobject obj, jlong ptr, jint deviceId, jint instanceId, jboolean value, jint duration, jlong successCallback, jlong failureCallback, jlong callbackArg) {
    ZWay zway = (ZWay) ptr;

    jclass cls = (*env)->FindClass(env, JNIT_CLASS);
    jmethodID mid = (*env)->GetMethodID(env, cls, "callbackStub", "()V");
    if (mid == 0) {
        zway_log(zway, Critical, ZSTR("Callback method not found"));
        ZWError err = InvalidArg;
        JNI_THROW_EXCEPTION();
    }
    
    jni_callback_data *cbkData = malloc(sizeof(jni_callback_data));
    (*env)->GetJavaVM(env, &(cbkData->jvm));
    //cbkData->env = env;
    cbkData->cls = cls;
    cbkData->method = mid;

    ZWError err = zway_cc_switch_binary_set(zway, deviceId, instanceId, value, duration, (ZJobCustomCallback) callback_stub, (ZJobCustomCallback) failureCallback, (void*)cbkData);
    if (err != NoError) {
        free(cbkData);
        JNI_THROW_EXCEPTION();
    }
}

// TODO add switch binary set

static JNINativeMethod funcs[] = {
	{ "jni_zway_init", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;J)J", (void *)&jni_zway_init },
	{ "jni_discover", "(J)V", (void *)&jni_discover },
	{ "jni_addNodeToNetwork", "(JZ)V", (void *)&jni_addNodeToNetwork },
	{ "jni_removeNodeFromNetwork", "(JZ)V", (void *)&jni_removeNodeFromNetwork },
	{ "jni_setDefault", "(J)V", (void *)&jni_setDefault },
	{ "jni_cc_switchBinarySet", "(JIIZIJJJ)V", (void *)&jni_cc_switchBinarySet },
	{ "jni_isRunning", "(J)Z", (void *)&jni_isRunning }
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
