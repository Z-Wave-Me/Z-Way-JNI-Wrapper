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

    public void addNodeToNetwork(boolean startStop) {
        jni_addNodeToNetwork(zway, startStop);
    }

    public void removeNodeFromNetwork(boolean startStop) { jni_removeNodeFromNetwork(zway, startStop); }

    private static native long jni_zway_init(String name, String port, int speed, String config_folder, String translations_folder, String zddx_folder, long ternminator_callback);
    private static native void jni_discover(long ptr);
    private static native void jni_addNodeToNetwork(long ptr, boolean startStop);

    private static native void jni_removeNodeFromNetwork(long ptr, boolean startStop);
}
