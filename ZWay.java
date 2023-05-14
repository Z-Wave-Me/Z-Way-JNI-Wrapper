public class ZWay {
    private long zway = 0;

    static {
        System.loadLibrary("jzway");
    }

    public ZWay(String name, String port, int speed, String config_folder, String translations_folder, String zddx_folder, long ternminator_callback) {
        zway = jni_zway_init(name, port, speed, config_folder, translations_folder, zddx_folder, ternminator_callback);
    }

    public void discover() {
        jni_discover(zway);
    }

    // Controller methods

    public void addNodeToNetwork(boolean startStop) {
        jni_addNodeToNetwork(zway, startStop);
    }

    public void removeNodeFromNetwork(boolean startStop) { jni_removeNodeFromNetwork(zway, startStop); }

    public void setDefault() { jni_setDefault(zway); }

    public boolean isRunning() { return jni_isRunning(zway); }

    // Command Classes methods
    // TODO executable
    public void cc_switchBinarySet(int deviceId, int instanceId, boolean value, int duration, long successCallback, long failureCallback, long callbackArg) {
        //long LsuccessCallback = successCallback.hashCode();
        jni_cc_switchBinarySet(zway, deviceId, instanceId, value, duration, successCallback, failureCallback, callbackArg);
    }

    // Callback stub
    private void callbackStub() {
        System.out.println("In Java");
    }

    // JNI functions
    private native long jni_zway_init(String name, String port, int speed, String config_folder, String translations_folder, String zddx_folder, long ternminator_callback);
    private native void jni_discover(long ptr);
    private native void jni_addNodeToNetwork(long ptr, boolean startStop);
    private native void jni_removeNodeFromNetwork(long ptr, boolean startStop);
    private native void jni_setDefault(long ptr);
    private native boolean jni_isRunning(long ptr);
    private native void jni_cc_switchBinarySet(long ptr, int deviceId, int instanceId, boolean value, int duration, long successCallback, long failureCallback, long callbackArg);
    // added for terminator callback
}
