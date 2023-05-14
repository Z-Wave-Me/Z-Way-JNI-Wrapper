class Main {
    public static void main(String args[]) throws InterruptedException {
        ZWay zway;
        try {
            System.out.println("init zway");
            zway = new ZWay("zway", "/dev/ttyACM1", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX", 0);
        } catch (java.lang.Exception e) {
            System.out.println(e);
            throw new RuntimeException();
        }

        try {
            System.out.println("discover");
            zway.discover();
        } catch (java.lang.Exception e) {
            System.out.println(e);
        }

        Thread.sleep(1000);
        //zway.setDefault();
        //zway.addNodeToNetwork(true);
        //zway.removeNodeFromNetwork(true);

        boolean s = false;
        while (zway.isRunning()) {
            Thread.sleep(1000);
            try {
                System.out.println("switch");
                zway.cc_switchBinarySet(2, 0, s, 0, 0, 0, 0);
            } catch (java.lang.Exception e) {
                System.out.println(e);
            }
            s = !s;
        }
    }
}
