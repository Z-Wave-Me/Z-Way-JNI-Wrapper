import me.zwave.zway.*;

class Main {
    private static ZWay zway;
    
    public static void main(String[] args) throws Exception {
        zway = init();

        zway.bind(new DeviceDemo());
        zway.bind(new StatusDemo());
        zway.bind(new TerminateDemo());

        discover(zway);

        /*
        zway.controller.setDefault();
        Thread.sleep(10000);
        zway.controller.removeNodeFromNetwork(true);
        Thread.sleep(5000);
        zway.controller.addNodeToNetwork(true);
        */

        int nodeId = 2;
        
        boolean s = false;
        while (zway.isRunning()) {
            Thread.sleep(1000);
            try {
                System.out.println("switch");
                ((ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(0).commandClassesByName.get("switchBinary")).set(s, 0, "SwitchBinary 2.0.37");
            } catch (java.lang.Exception e) {
                System.out.println(e);
            }
            s = !s;
        }
    }

    static class Demo implements ZWay.DataCallback {
        public void dataCallback(ZWay.Data data, Integer type) {
            if (type == ZWay.Data.deleted) return;
            try {
                System.out.println(data.path + " = " + data.getBool() + " (type = " + type + ")"); 
            } catch (ZWay.Data.NotAlive e) {
                System.out.println("Data no alive " + e.data.path + " " + e);
            }
        }
    }

    static class DeviceDemo implements ZWay.DeviceCallback {
        public void deviceCallback(Integer type, Integer deviceId, Integer instanceId, Integer commandClassId) {
            if (type == ZWay.deviceAdded) {
                System.out.println("Device added " + deviceId);
            } else if (type == ZWay.deviceRemoved) {
                System.out.println("Device removed " + deviceId);
            } else if (type == ZWay.instanceAdded) {
                System.out.println("Instance added " + deviceId + ":" + instanceId);
            } else if (type == ZWay.instanceRemoved) {
                System.out.println("Instance removed " + deviceId + ":" + instanceId);
            } else if (type == ZWay.commandAdded) {
                System.out.println("Command Class added " + deviceId + ":" + instanceId + " " + commandClassId);
            } else if (type == ZWay.commandRemoved) {
                System.out.println("Command Class removed " + deviceId + ":" + instanceId + " " + commandClassId);
            }

            // subscribe to switchBinary.data.level
            if (type == zway.commandAdded && commandClassId == ZWay.commandClassIdByName.get("switchBinary")) {
                try {
                    ((ZWay.Device.Instance.SwitchBinary) zway.devices.get(deviceId).instances.get(instanceId).commandClassesByName.get("switchBinary")).data.get("level").bind(new Demo());
                } catch (ZWay.Data.NotFound e) {
                    System.out.println("Data is not found ");
                } catch (ZWay.Data.NotAlive e) {
                    System.out.println("Data is not alive " + e.data.path);
                } catch (java.lang.Exception e) {
                    System.out.println(e);
                }
            }
        }
    }

    static class StatusDemo implements ZWay.StatusCallback {
        public void statusCallback(boolean result, Object obj) {
            if (result) {
                System.out.println("Function completed successfully with an argument: " + obj);
            } else {
                System.out.println("Function failed with an argument: " + obj);
            }
        }
    }

    static class TerminateDemo implements ZWay.TerminateCallback {
        public void terminateCallback() {
            System.out.println("Object terminated");
        }
    }
    
    public static ZWay init() {
        ZWay zway;
        try {
            System.out.println("init zway");
            zway = new ZWay("zway", "/dev/ttyACM0", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX");
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
