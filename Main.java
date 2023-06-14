import ZWayJNIWrapper.*;

import java.lang.reflect.Type;

class Main {
    public static void main(String[] args) throws InterruptedException {

        ZWay zway = init();

        discover(zway);

        Thread.sleep(1000);
        //zway.controller.setDefault();
        //zway.controller.addNodeToNetwork(true);
        //zway.controller.removeNodeFromNetwork(true);

        //((ZWay.Device.Instance.SwitchBinary) zway.devices.get(2).instances.get(0).commandClassesByName.get("switchBinary")).data.get("level").bind(myCallback);
        try {
            System.out.println("switch level: " + String.valueOf(((ZWay.Device.Instance.SwitchBinary) zway.devices.get(2).instances.get(0).commandClassesByName.get("switchBinary")).data.get("level").getBool()));
        } catch (java.lang.Exception e) {
            System.out.println(e);
        }

        boolean s = false;
        while (zway.isRunning()) {
            Thread.sleep(1000);
            try {
                System.out.println("switch");
                ((ZWay.Device.Instance.SwitchBinary) zway.devices.get(2).instances.get(0).commandClassesByName.get("switchBinary")).set(s, 0);
            } catch (java.lang.Exception e) {
                System.out.println(e);
            }
            s = !s;
        }
    }

    public static ZWay init() {
        ZWay zway;
        try {
            System.out.println("init zway");
            zway = new ZWay("zway", "/dev/ttyACM2", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX", 0);
        } catch (java.lang.Exception e) {
            System.out.println(e);
            throw new RuntimeException();
        }
        return zway;
    }

    public static void discover(ZWay zway) {
        try {
            System.out.println("discover");
            zway.discover();
        } catch (java.lang.Exception e) {
            System.out.println(e);
        }
    }
}
