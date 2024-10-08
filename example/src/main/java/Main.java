//
//  Main.java
//  Example of usage for Z-Way.C JNI wrapper library
//
//  Created by Veniamin Poltorak and Serguei Poltorak on 7/07/2023.
//
//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements.  See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
//  
//    http://www.apache.org/licenses/LICENSE-2.0
//  
//  Unless required by applicable law or agreed to in writing,
//  software distributed under the License is distributed on an
//  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//  KIND, either express or implied.  See the License for the
//  specific language governing permissions and limitations
//  under the License.    
//

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
            if (type == ZWay.DataEventType.DELETED.code) return;
            try {
                System.out.println(data.path + " = " + data.getBool() + " (type = " + type + ")"); 
            } catch (ZWay.Data.NotAlive e) {
                System.out.println("Data no alive " + e.data.path + " " + e);
            }
        }
    }

    static class DeviceDemo implements ZWay.DeviceCallback {
        public void deviceCallback(Integer type, Integer deviceId, Integer instanceId, Integer commandClassId) {
            if (type == ZWay.EventType.DEVICE_ADDED.code) {
                System.out.println("Device added " + deviceId);
            } else if (type == ZWay.EventType.DEVICE_REMOVED.code) {
                System.out.println("Device removed " + deviceId);
            } else if (type == ZWay.EventType.INSTANCE_ADDED.code) {
                System.out.println("Instance added " + deviceId + ":" + instanceId);
            } else if (type == ZWay.EventType.INSTANCE_REMOVED.code) {
                System.out.println("Instance removed " + deviceId + ":" + instanceId);
            } else if (type == ZWay.EventType.COMMAND_ADDED.code) {
                System.out.println("Command Class added " + deviceId + ":" + instanceId + " " + commandClassId);
            } else if (type == ZWay.EventType.COMMAND_REMOVED.code) {
                System.out.println("Command Class removed " + deviceId + ":" + instanceId + " " + commandClassId);
            }

            // subscribe to switchBinary.data.level
            if (type == ZWay.EventType.COMMAND_ADDED.code && commandClassId == ZWay.commandClassIdByName.get("switchBinary")) {
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
            zway = new ZWay("zway", "/dev/ttyUSB0", 115200, "/opt/z-way-server/config", "/opt/z-way-server/translations", "/opt/z-way-server/ZDDX");
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
