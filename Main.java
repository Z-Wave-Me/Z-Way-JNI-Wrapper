import ZWayJNIWrapper.*;

import java.lang.reflect.Type;

class Main {
    public static void main(String[] args) throws InterruptedException {
        ZWay zway = init();

        discover(zway);

        Thread.sleep(1000);
        
        /*
        zway.controller.setDefault();
        Thread.sleep(10000);
        zway.controller.removeNodeFromNetwork(true);
        Thread.sleep(5000);
        zway.controller.addNodeToNetwork(true);
        */

        int nodeId = 2;
        
        try {
            ((ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(0).commandClassesByName.get("switchBinary")).data.get("level").bind(new Demo());
        } catch (ZWay.Data.NotFound e) {
            System.out.println("Data is not found ");
        } catch (ZWay.Data.NotAlive e) {
            System.out.println("Data is not alive " + e.data.path);
        } catch (java.lang.Exception e) {
            System.out.println(e);
        }
        
        try {
            System.out.println("switch level: " + String.valueOf(((ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(0).commandClassesByName.get("switchBinary")).data.get("level").getBool()));
        } catch (ZWay.Data.NotFound e) {
            System.out.println("Data is not found ");
        } catch (ZWay.Data.NotAlive e) {
            System.out.println("Data is not alive " + e.data.path);
        } catch (java.lang.Exception e) {
            System.out.println(e);
        }

        boolean s = false;
        while (zway.isRunning()) {
            Thread.sleep(1000);
            try {
                System.out.println("switch");
                ((ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(0).commandClassesByName.get("switchBinary")).set(s, 0);
            } catch (java.lang.Exception e) {
                System.out.println(e);
            }
            s = !s;
        }
    }

    static class Demo implements ZWay.DataCallback {
        public void dataCallback(ZWay.Data data, Integer type) {
            try {
                System.out.println(data.path + " = " + data.getBool() + " (type = " + type + ")"); 
            } catch (ZWay.Data.NotAlive e) {
                System.out.println("Data no alive " + e.data.path + " " + e);
            }
        }
    }
    
    public static ZWay init() {
        ZWay zway;
        try {
            System.out.println("init zway");
            zway = new ZWay("zway", "/dev/ttyACM0", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX", 0);
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
