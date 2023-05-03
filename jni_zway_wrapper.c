#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include <ZWayLib.h>
#include <ZLogging.h>

#define JNIT_CLASS "ZWay"

#define JNI_THROW_EXCEPTION() \
    jclass Exception = (*env)->FindClass(env, "java/lang/Exception"); \
    (*env)->ThrowNew(env, Exception, zstrerror(err));

#define JNI_THROW_EXCEPTION_RET(ret) \
    JNI_THROW_EXCEPTION(); \
    return (ret);

// to do: use an appropriate callback for java - terminator_callback
// to do: stdout (logger)
// to do: add loglevel
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

/*static void jni_controllerChange(JNIEnv *env, jobject obj, jlong ptr, jboolean startStop) {
    (void)obj;
    (void)env;
    
    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_change(zway, startStop);
    
    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_getSUCNodeId(JNIEnv *env, jobject obj, jlong ptr) {
    (void)obj;
    (void)env;
    
    ZWay zway = (ZWay) ptr;

    ZWError err = zway_fc_get_suc_node_id(zway, NULL, NULL, NULL);
    
    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_setSUCNodeId(JNIEnv *env, jobject obj, jlong ptr, jint node_id) {
    (void)obj;
    (void)env;
    
    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_set_suc_node_id(zway, node_id);
    
    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_setSISNodeId(JNIEnv *env, jobject obj, jlong ptr, jint node_id) {
    (void)obj;
    (void)env;
    
    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_set_sis_node_id(zway, node_id);
    
    if (err != NoError) {
        JNI_THROW_EXCEPTION();
    }
}

static void jni_disableSUCNodeId(JNIEnv *env, jobject obj, jlong ptr, jint node_id) {
    (void)obj;
    (void)env;
    
    ZWay zway = (ZWay) ptr;

    ZWError err = zway_controller_disable_suc_node_id(zway, node_id);
    
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
*/

static JNINativeMethod funcs[] = {
	{ "jni_zway_init", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;J)J", (void *)&jni_zway_init },
	{ "jni_discover", "(J)V", (void *)&jni_discover },
	{ "jni_addNodeToNetwork", "(JZ)V", (void *)&jni_addNodeToNetwork },
	{ "jni_removeNodeFromNetwork", "(JZ)V", (void *)&jni_removeNodeFromNetwork }
	/*{ "jni_controllerChange", "(JI)V", (void *)&c_subtract },
	{ "jni_getSUCNodeId", "(J)V", (void *)&c_increment },
	{ "jni_setSUCNodeId", "(J)V", (void *)&c_decrement },
	{ "jni_setSISNodeId", "(J)I", (void *)&c_getval },
	{ "jni_setDefault", "(J)Ljava/lang/String;", (void *)&c_toString }*/
};

JNIEXPORT void JNICALL Java_java_lang_Object_registerNatives(JNIEnv *env, jclass cls) {
    printf("%i\n", (*env)->RegisterNatives(env, cls, funcs, sizeof(funcs)/sizeof(funcs[0])));
}

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
	//Java_java_lang_Object_registerNatives(env, cls);

	printf("Register > %i\n", reg_res);

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
