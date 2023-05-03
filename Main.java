class Main {
    public static void main(String args[]) {
        try {
            ZWay zway = new ZWay("zway", "/dev/ttyACM0", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX", 0);
            zway.discover();
            zway.addNodeToNetwork(true);
        } catch (java.lang.Exception e) {
            System.out.println("Exception was caught");
        }
    }
}
