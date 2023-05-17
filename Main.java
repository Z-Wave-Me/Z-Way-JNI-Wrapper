class Main {
    public static void main(String[] args) throws InterruptedException {
        ZWay zway;
        try {
            System.out.println("init zway");
            zway = new ZWay("zway", "/dev/ttyACM0", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX", 0);
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
        //zway.controller.setDefault();
        //zway.controller.addNodeToNetwork(true);
        //zway.controller.removeNodeFromNetwork(true);


        boolean s = false;
        while (zway.isRunning()) {
            Thread.sleep(1000);
            try {
                System.out.println("switch");
                //zway.devices[2].instances[0].switchbinary(2, 0, s, 0, 0, 0, 0);
                ((ZWay.Device.Instance.SwitchBinary) zway.devices.get(2).instances.get(0).commandsByName.get("switchBinary")).set(s);
                //zway.switchBinary(2, 0).set(s);
            } catch (java.lang.Exception e) {
                System.out.println(e);
            }
            s = !s;
        }
    }
}
