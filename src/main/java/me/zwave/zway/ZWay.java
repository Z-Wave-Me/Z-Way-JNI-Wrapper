//
//  ZWay.java
//  Part of Z-Way.C JNI wrapper library
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

package me.zwave.zway;

import java.lang.reflect.Type;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public final class ZWay {
    public enum DataEventType {
        UPDATED        (0x01),
        INVALIDATED    (0x02),
        DELETED        (0X03),
        CHILD_CREATED  (0X04),
        PHANTOM_UPDATE (0X40),
        CHILD_EVENT    (0X80);

        public final int code;

        DataEventType(int code) {
            this.code = code;
        }
    }

    public enum EventType {
        DEVICE_ADDED     (0x01),
        DEVICE_REMOVED   (0x02),
        INSTANCE_ADDED   (0X04),
        INSTANCE_REMOVED (0X08),
        COMMAND_ADDED    (0X10),
        COMMAND_REMOVED  (0X20);

        public final int code;

        EventType(int code) {
            this.code = code;
        }
    }

    public enum DataType {
        EMPTY        (0),
        BOOL         (1),
        INT          (2),
        FLOAT        (3 ),
        STRING       (4),
        BINARY       (5),
        INT_ARRAY    (6),
        FLOAT_ARRAY  (7),
        STRING_ARRAY (8);

        public final int code;

        DataType(int code) {
            this.code = code;
        }
    }

    private final long jzway;

    public Controller controller;
    public Map<Integer, Device> devices;

    private Set<DeviceCallback> deviceCallbacks;
    private Set<StatusCallback> statusCallbacks;
    private Set<TerminateCallback> terminateCallbacks;

    static {
        System.loadLibrary("jzway");
    }

    public ZWay(String name, String port, int speed, String config_folder, String translations_folder, String zddx_folder) throws Exception {
        jzway = jni_zwayInit(name, port, speed, config_folder, translations_folder, zddx_folder);

        controller = new Controller();
        devices = new HashMap<>();

        deviceCallbacks = new HashSet<>();
        statusCallbacks = new HashSet<>();
        terminateCallbacks = new HashSet<>();
    }

    @Override
    public void finalize() {
        jni_finalize(jzway);
    }

    public void bind(DeviceCallback func) throws Exception {
        deviceCallbacks.add(func);
    }

    public void unbind(DeviceCallback func) throws Exception {
        deviceCallbacks.remove(func);
    }

    public void bind(StatusCallback func) {
        statusCallbacks.add(func);
    }

    public void unbind(StatusCallback func) {
        statusCallbacks.remove(func);
    }

    public void bind(TerminateCallback func) {
        terminateCallbacks.add(func);
    }

    public void unbind(TerminateCallback func) {
        terminateCallbacks.remove(func);
    }

    public static final Map<String, Integer> commandClassIdByName = new HashMap<>();
    static {
        /* BEGIN AUTOGENERATED CC TEMPLATE
        commandClassIdByName.put("%cc_camel_case_name%", %cc_id%);
        * END AUTOGENERATED CC TEMPLATE */
    }
    
    public static final Map<Integer, String> commandClassNameById = new HashMap<>();
    static {
        /* BEGIN AUTOGENERATED CC TEMPLATE
        commandClassNameById.put(%cc_id%, "%cc_camel_case_name%");
        * END AUTOGENERATED CC TEMPLATE */
    }
    
    // Controller methods

    public boolean isRunning() { return jni_isRunning(jzway); }
    public boolean isIdle() { return jni_isIdle(jzway); }
    public void stop() { jni_stop(jzway); }
    public void discover() { jni_discover(jzway); }

    /* BEGIN AUTOGENERATED CC TEMPLATE
    public Device.Instance.%cc_capitalized_name% %cc_camel_case_name%(Integer deviceId, Integer instanceId) {
        return ((ZWay.Device.Instance.%cc_capitalized_name%) devices.get(deviceId).instances.get(instanceId).commandClassesByName.get("%cc_camel_case_name%"));
    }
    * END AUTOGENERATED CC TEMPLATE */

    public static Type commandClassClassById(Integer id) {
        switch (id) {
            /* BEGIN AUTOGENERATED CC TEMPLATE
            case %cc_id%:
                return ZWay.Device.Instance.%cc_capitalized_name%.class;
            * END AUTOGENERATED CC TEMPLATE */
            default:
                return null;
        }
    }

    public static String commandClassNameById(Integer id) {
        switch (id) {
            /* BEGIN AUTOGENERATED CC TEMPLATE
            case %cc_id%:
                return "%cc_camel_case_name%";
            * END AUTOGENERATED CC TEMPLATE */
            default:
                return null;
        }
    }

    // Callback stub
    private void statusCallback(boolean result, Object obj) {
        for (StatusCallback sc : statusCallbacks) {
            sc.statusCallback(result, obj);
        }
    }

    private void deviceCallback(int type, int deviceId, int instanceId, int commandClassId) throws Exception {
        if (type == EventType.DEVICE_ADDED.code) {
            Device device = new Device(this, deviceId);
            this.devices.put(deviceId, device);
            device.instances.put(0, device.new Instance(this, device, 0));
        } else if (type == EventType.DEVICE_REMOVED.code) {
            devices.remove(deviceId);
        } else if (type == EventType.INSTANCE_ADDED.code) {
            Device device = devices.get(deviceId);
            device.instances.put(instanceId, device.new Instance(this, device, instanceId));
        } else if (type == EventType.INSTANCE_REMOVED.code) {
            devices.get(deviceId).instances.remove(instanceId);
        } else if (type == EventType.COMMAND_ADDED.code) {
            Device device = devices.get(deviceId);
            Device.Instance instance = device.instances.get(instanceId);
            switch (commandClassId) {
                /* BEGIN AUTOGENERATED CC TEMPLATE
                case %cc_id%:
                    Device.Instance.%cc_capitalized_name% command%cc_capitalized_name% = instance.new %cc_capitalized_name%(this, instance);
                    instance.commandClasses.put(commandClassId, command%cc_capitalized_name%);
                    instance.commandClassesByName.put("%cc_camel_case_name%", command%cc_capitalized_name%);
                    break;
                 * END AUTOGENERATED CC TEMPLATE */
            }
        } else if (type == EventType.COMMAND_REMOVED.code) {
            Device.Instance instance = devices.get(deviceId).instances.get(instanceId);
            instance.commandClasses.remove(commandClassId);
            switch (commandClassId) {
                /* BEGIN AUTOGENERATED CC TEMPLATE
                case %cc_id%:
                    instance.commandClassesByName.remove("%cc_camel_case_name%");
                    break;
                 * END AUTOGENERATED CC TEMPLATE */
            }
        } else {
            System.out.println("Unhandled deviceCallback: type = " + type + ", id = " + deviceId + ", instance = " + instanceId + ", commandClass = " + commandClassId);
        }

        for (DeviceCallback dc : deviceCallbacks) {
            dc.deviceCallback(type, deviceId, instanceId, commandClassId);
        }
    }

    private void terminateCallback() {
        for (TerminateCallback tc : terminateCallbacks) {
            tc.terminateCallback();
        }
    }

    // JNI functions
    private native long jni_zwayInit(String name, String port, int speed, String config_folder, String translations_folder, String zddx_folder);
    private native void jni_finalize(long ptr);
    private native void jni_discover(long ptr);
    private native void jni_stop(long ptr);
    private native boolean jni_isIdle(long ptr);
    private native boolean jni_isRunning(long ptr);
    private native void jni_addNodeToNetwork(long ptr, boolean startStop);
    private native void jni_removeNodeFromNetwork(long ptr, boolean startStop);
    private native void jni_removeFailedNode(long ptr, int nodeId);
    private native void jni_controllerChange(long ptr, boolean startStop);
    private native void jni_setSUCNodeId(long ptr, int nodeId);
    private native void jni_setSISNodeId(long ptr, int nodeId);
    private native void jni_disableSUCNodeId(long ptr, int nodeId);
    private native void jni_setDefault(long ptr);
    private native void jni_requestNetworkUpdate(long ptr);
    private native void jni_setLearnMode(long ptr, boolean startStop);
    private native int[] jni_backup(long ptr);
    private native void jni_restore(long ptr, int[] data, boolean full);
    private native void jni_nodeProvisioningDSKAdd(long ptr, int[] dsk);
    private native void jni_nodeProvisioningDSKRemove(long ptr, int[] dsk);
    private native long jni_zdataFind(long dh, String path, long jzway);
    private native long jni_zdataControllerFind(String path, long jzway);
    private native long jni_zdataDeviceFind(String path, int deviceId, long jzway);
    private native long jni_zdataInstanceFind(String path, int deviceId, int instanceId, long jzway);
    private native long jni_zdataCommandClassFind(String path, int deviceId, int instanceId, int commandClassId, long jzway);
    private native void jni_deviceSendNOP(long ptr, int nodeId, Object arg);
    private native void jni_deviceAwakeQueue(long ptr, int nodeId);
    private native void jni_deviceInterviewForce(long ptr, int deviceId);
    private native boolean jni_deviceIsInterviewDone(long ptr, int deviceId);
    private native void jni_deviceDelayCommunication(long ptr, int deviceID, int delay);
    private native void jni_deviceAssignReturnRoute(long ptr, int deviceID, int nodeId);
    private native void jni_deviceAssignPriorityReturnRoute(long ptr, int deviceID, int nodeId, int repeater1, int repeater2, int repeater3, int repeater4);
    private native void jni_deviceDeleteReturnRoute(long ptr, int deviceID);
    private native void jni_deviceAssignSUCReturnRoute(long ptr, int deviceID);
    private native void jni_deviceAssignPrioritySUCReturnRoute(long ptr, int deviceID, int repeater1, int repeater2, int repeater3, int repeater4);
    private native void jni_deviceDeleteSUCReturnRoute(long ptr, int deviceID);
    private native void jni_ZDDXSaveToXML(long ptr);
    private native void jni_commandInterview(long ptr, int deviceId, int instanceId, int ccId);


    /* BEGIN AUTOGENERATED FC TEMPLATE
    private native void jni_fc_%function_short_camel_case_name%(long ptr, %params_java_declarations%Object callbackArg);
     * END AUTOGENERATED FC TEMPLATE */

    /* BEGIN AUTOGENERATED CC TEMPLATE
     * BEGIN AUTOGENERATED CMD TEMPLATE
    private native void jni_cc_%function_short_camel_case_name%(long ptr, int deviceId, int instanceId, %params_java_declarations%Object callbackArg);
     * END AUTOGENERATED CMD TEMPLATE
     * END AUTOGENERATED CC TEMPLATE */

    /* BEGIN AUTOGENERATED FC TEMPLATE
    public void %function_command_camel_name%(%params_java_declarations%Object callbackArg) {
        jni_fc_%function_short_camel_case_name%(jzway, %params_java%callbackArg);
    }
    public void %function_command_camel_name%(%params_java_declarations_no_comma%) {
        jni_fc_%function_short_camel_case_name%(jzway, %params_java%0);
    }
    
     * END AUTOGENERATED FC TEMPLATE */


    public interface DataCallback {
        public void dataCallback(Data data, Integer type);
    }

    public interface DeviceCallback {
        public void deviceCallback(Integer type, Integer deviceId, Integer instanceId, Integer commandClassId);
    }

    public interface StatusCallback {
        public void statusCallback(boolean result, Object obj);
    }

    public interface TerminateCallback {
        public void terminateCallback();
    }

    public final class Data {


        private Object value;
        private Type valueType;
        private String valueTypeStr;

        private long dh;

        public final String name;
        public final String path;

        private long updateTime;
        private long invalidateTime;

        private Boolean isAlive;
        
        private Set<DataCallback> callbacks;
        
        // exceptions
        
        public final class NotAlive extends Throwable {
            public final Data data;
            
            public NotAlive(Data data) {
                super();
                this.data = data;
            }
        }

        public final class NotFound extends Throwable {
            public NotFound() {
                super();
            }
        }

        // constructors
        
        public Data(String path, long dhParent) throws NotFound, Exception {
            this(jni_zdataFind(dhParent, path, jzway));
        }

        public Data(String path) throws NotFound, Exception {
            this(jni_zdataControllerFind(path, jzway));
        }

        public Data(String path, int deviceId) throws NotFound, Exception {
            this(jni_zdataDeviceFind(path, deviceId, jzway));
        }

        public Data(String path, int deviceId, int instanceId) throws NotFound, Exception {
            this(jni_zdataInstanceFind(path, deviceId, instanceId, jzway));
        }

        public Data(String path, int deviceId, int instanceId, int commandClassId) throws NotFound, Exception {
            this(jni_zdataCommandClassFind(path, deviceId, instanceId, commandClassId, jzway));
        }

        private Data(long dh) throws NotFound, Exception {
            if (dh == 0) {
                throw new NotFound();
            }
            this.dh = dh;
            name = jni_zdataGetName(dh);
            path = jni_zdataGetPath(dh);
            isAlive = true;
            callbacks = new HashSet<>();
            updateTime = jni_zdataGetUpdateTime(dh);
            invalidateTime = jni_zdataGetInvalidateTime(dh);
            jni_zdataAddCallback(dh);
            getValue();
        }

        @Override
        public void finalize() {
            jni_zdataRemoveCallback(dh);
        }

        private void isAlive() throws NotAlive {
            if (!isAlive) {
                throw new NotAlive(this);
            }
        }
        
        public void bind(DataCallback func) throws NotAlive, Exception {
            isAlive();
            
            callbacks.add(func);
        }
        
        public void unbind(DataCallback func) throws NotAlive, Exception {
            isAlive();
            
            callbacks.remove(func);
        }

        public long getUpdateTime() {
            return updateTime;
        }

        public long getInvalidateTime() {
            return invalidateTime;
        }

        private void dataCallback(int type) throws Exception {
            // get type of the event
            boolean isPhantom = (DataEventType.PHANTOM_UPDATE.code & type) > 0;
            boolean isChild = (DataEventType.CHILD_EVENT.code & type) > 0;
            type = type & (~DataEventType.PHANTOM_UPDATE.code) & (~DataEventType.CHILD_EVENT.code);
            if (type == DataEventType.UPDATED.code) {
                getValue();
            } else if (type == DataEventType.INVALIDATED.code) {
                jni_zdataGetInvalidateTime(dh);
            } else if (type == DataEventType.DELETED.code) {
                isAlive = false;
            } else if (type == DataEventType.CHILD_CREATED.code) {
                // nothing to do
            } else {
                throw new Exception("Type of the event is an invalid integer.");
            }

            if (isPhantom) {
                jni_zdataGetUpdateTime(dh);
            }
            
            for (DataCallback dc : callbacks) {
                dc.dataCallback(this, type);
            }
        }

        private void getValue() throws Exception {
            int dataType = jni_zdataGetType(dh);
            if (dataType == DataType.BOOL.code) {
                valueType = Boolean.class;
                valueTypeStr = "Boolean";
                value = jni_zdataGetBoolean(dh);
            } else if (dataType == DataType.INT.code) {
                valueType = Integer.class;
                valueTypeStr = "Integer";
                value = jni_zdataGetInteger(dh);
            } else if (dataType == DataType.FLOAT.code) {
                valueType = Float.class;
                valueTypeStr = "Float";
                value = jni_zdataGetFloat(dh);
            } else if (dataType == DataType.STRING.code) {
                valueType = String.class;
                valueTypeStr = "String";
                value = jni_zdataGetString(dh);
            } else if (dataType == DataType.BINARY.code) {
                valueType = Byte[].class;
                valueTypeStr = "Byte[]";
                int[] val = jni_zdataGetBinary(dh);
                value = Arrays.stream(val).boxed().toArray( Integer[]::new );
            } else if (dataType == DataType.INT_ARRAY.code) {
                valueType = Integer[].class;
                valueTypeStr = "Integer[]";
                int[] val = jni_zdataGetIntArray(dh);
                value = Arrays.stream(val).boxed().toArray( Integer[]::new );
            } else if (dataType == DataType.FLOAT_ARRAY.code) {
                valueType = Float[].class;
                valueTypeStr = "Float[]";
                float[] val = jni_zdataGetFloatArray(dh);
                int len = val.length;
                Float[] newValue = new Float[len];
                for (int i = 0; i < len; i++) {
                    newValue[i] = val[i];
                }
                value = newValue;
            } else if (dataType == DataType.STRING_ARRAY.code) {
                valueType = String[].class;
                valueTypeStr = "String[]";
                value = jni_zdataGetStringArray(dh);
            } else if (dataType == DataType.EMPTY.code) {
                valueType = Object.class;
                valueTypeStr = "Null";
                value = null;
            } else {
                throw new Exception("Type of the value in data holder is an invalid integer.");
            }
        }

        public Data[] getChildren() throws NotAlive, Exception {
            isAlive();
            
            long[] list = jni_zdataGetChildren(dh);
            int length = list.length;
            Data[] children = new Data[length];
            for (int i = 0; i < length; i++) {
                try {
                    children[i] = new Data(list[i]);
                } catch (NotFound e) {
                    // should never happen
                    throw new RuntimeException();
                }
            }
            return children;
        }
        
        public Data get(String path) throws NotAlive, NotFound, Exception {
            isAlive();
            
            return new Data(path, this.dh);
        }

        public Type getValueType() throws NotAlive {
            isAlive();
            
            return valueType;
        }

        public String getValueTypeStr() throws NotAlive {
            isAlive();
            
            return valueTypeStr;
        }

        public void setBool(Boolean data) throws NotAlive {
            isAlive();
            
            jni_zdataSetBoolean(dh, data);
        }

        public void setInt(Integer data) throws NotAlive {
            isAlive();
            
            jni_zdataSetInteger(dh, data);
        }

        public void setFloat(Float data) throws NotAlive {
            isAlive();
            
            jni_zdataSetFloat(dh, data);
        }

        public void setString(String data) throws NotAlive {
            isAlive();
            
            jni_zdataSetString(dh, data);
        }

        public void setByteList(Integer[] data) throws NotAlive {
            isAlive();
            
            int size = data.length;
            int[] rdata = new int[size];
            for (int i = 0; i < size; i++) {
                rdata[i] = data[i];
            }
            jni_zdataSetBinary(dh, rdata);
        }

        public void setIntList(Integer[] data) throws NotAlive {
            isAlive();
            
            int size = data.length;
            int[] rdata = new int[size];
            for (int i = 0; i < size; i++) {
                rdata[i] = data[i];
            }
            jni_zdataSetIntArray(dh, rdata);
        }

        public void setFloatList(Float[] data) throws NotAlive {
            isAlive();
            
            int size = data.length;
            float[] rdata = new float[size];
            for (int i = 0; i < size; i++) {
                rdata[i] = data[i];
            }
            jni_zdataSetFloatArray(dh, rdata);
        }

        public void setStringList(String[] data) throws NotAlive {
            isAlive();

            jni_zdataSetStringArray(dh, data);
        }

        public void setEmpty() throws NotAlive {
            isAlive();
            
            jni_zdataSetEmpty(dh);
        }

        public Boolean getBool() throws NotAlive {
            isAlive();
            
            if (valueType == Boolean.class && valueTypeStr.equals("Boolean")) {
                return (Boolean) value;
            } else {
                throw new ClassCastException("Illegal call: value is not Boolean");
            }
        }

        public Integer getInt() throws NotAlive {
            isAlive();
            
            if (valueType == Integer.class && valueTypeStr.equals("Integer")) {
                return (Integer) value;
            } else {
                throw new ClassCastException("Illegal call: value is not Integer");
            }
        }

        public Float getFloat() throws NotAlive {
            isAlive();
            
            if (valueType == Float.class && valueTypeStr.equals("Float")) {
                return (Float) value;
            } else {
                throw new ClassCastException("Illegal call: value is not Float");
            }
        }

        public String getString() throws NotAlive {
            isAlive();
            
            if (valueType == String.class && valueTypeStr.equals("String")) {
                return (String) value;
            } else {
                throw new ClassCastException("Illegal call: value is not String");
            }
        }

        public Integer[] getByteList() throws NotAlive {
            isAlive();
            
            if (valueType == Byte[].class && valueTypeStr.equals("Byte[]")) {
                return (Integer[]) value;
            } else {
                throw new ClassCastException("Illegal call: value is not Integer[]");
            }
        }

        public Integer[] getIntList() throws NotAlive {
            isAlive();
            
            if (valueType == Integer[].class && valueTypeStr.equals("Integer[]")) {
                return (Integer[]) value;
            } else {
                throw new ClassCastException("Illegal call: value is not Integer[]");
            }
        }

        public Float[] getFloatList() throws NotAlive {
            isAlive();
            
            if (valueType == Float[].class && valueTypeStr.equals("Float[]")) {
                return (Float[]) value;
            } else {
                throw new ClassCastException("Illegal call: value is not Float[]");
            }
        }

        public String[] getStringList() throws NotAlive {
            isAlive();
            
            if (valueType == String[].class && valueTypeStr.equals("String[]")) {
                return (String[]) value;
            } else {
                throw new ClassCastException("Illegal call: value is not String[]");
            }
        }

        // JNI functions
        private native void jni_zdataAddCallback(long dh);
        private native void jni_zdataRemoveCallback(long dh);
        private native String jni_zdataGetName(long data);
        private native int jni_zdataGetType(long dh);
        private native boolean jni_zdataGetBoolean(long dh);
        private native int jni_zdataGetInteger(long dh);
        private native float jni_zdataGetFloat(long dh);
        private native String jni_zdataGetString(long dh);
        private native int[] jni_zdataGetBinary(long dh);
        private native int[] jni_zdataGetIntArray(long dh);
        private native float[] jni_zdataGetFloatArray(long dh);
        private native String[] jni_zdataGetStringArray(long dh);
        private native void jni_zdataSetEmpty(long dh);
        private native void jni_zdataSetBoolean(long dh, boolean data);
        private native void jni_zdataSetInteger(long dh, int data);
        private native void jni_zdataSetFloat(long dh, float data);
        private native void jni_zdataSetString(long dh, String data);
        private native void jni_zdataSetBinary(long dh, int[] data);
        private native void jni_zdataSetIntArray(long dh, int[] data);
        private native void jni_zdataSetFloatArray(long dh, float[] data);
        private native void jni_zdataSetStringArray(long dh, String[] data);
        private native long[] jni_zdataGetChildren(long dh);
        private native String jni_zdataGetPath(long dh);
        private native long jni_zdataGetUpdateTime(long dh);
        private native long jni_zdataGetInvalidateTime(long dh);
    }

    public final class Controller {
        public final Data data;

        public Controller() throws Exception {
            try {
                data = new Data("");
            } catch (Data.NotFound e) {
                // should never happen
                throw new RuntimeException();
            }
        }
        
        public void addNodeToNetwork(boolean startStop) {
            jni_addNodeToNetwork(jzway, startStop);
        }

        public void removeNodeFromNetwork(boolean startStop) {
            jni_removeNodeFromNetwork(jzway, startStop);
        }

        public void removeFailedNode(int nodeId) {
            jni_removeFailedNode(jzway, nodeId);
        }

        public void change(boolean startStop) {
            jni_controllerChange(jzway, startStop);
        }

        public void getSUCNodeId() {
            jni_fc_getSucNodeId(jzway, 0);
        }

        public void setSUCNodeId(int nodeId) {
            jni_setSUCNodeId(jzway, nodeId);
        }

        public void setSISNodeId(int nodeId) {
            jni_setSISNodeId(jzway, nodeId);
        }

        public void disableSUCNodeId(int nodeId) {
            jni_disableSUCNodeId(jzway, nodeId);
        }

        public void sendNodeInformation(int nodeId) {
            jni_fc_sendNodeInformation(jzway, nodeId, 0);
        }

        public void setDefault() {
            jni_setDefault(jzway);
        }

        public void requestNetworkUpdate() {
            jni_requestNetworkUpdate(jzway);
        }

        public void setLearnMode(boolean startStop) {
            jni_setLearnMode(jzway, startStop);
        }

        public int[] backup() {
            return jni_backup(jzway);
        }

        public void restore(int[] data, boolean full) {
            jni_restore(jzway, data, full);
        }

        public void nodeProvisioningDSKAdd(int[] dsk) {
            jni_nodeProvisioningDSKAdd(jzway, dsk);
        }

        public void nodeProvisioningDSKRemove(int[] dsk) {
            jni_nodeProvisioningDSKRemove(jzway, dsk);
        }

        public void ZDDXSaveToXML() { jni_ZDDXSaveToXML(jzway); }
    }

    public final class Device {
        public final Integer id;
        public final Data data;

        public Map<Integer, Instance> instances;

        public Device(ZWay zway, Integer device_id) throws Exception {
            id = device_id;
            try {
                data = new Data("", device_id);
            } catch (Data.NotFound e) {
                // should never happen
                throw new RuntimeException();
            }
            instances = new HashMap<>();
        }

        public void deviceSendNOP(Integer nodeId, Object arg) { jni_deviceSendNOP(jzway, nodeId, arg); }
        public void deviceAwakeQueue(Integer nodeId) { jni_deviceAwakeQueue(jzway, nodeId); }
        public void deviceInterviewForce(Integer deviceId) { jni_deviceInterviewForce(jzway, deviceId); }
        public boolean deviceIsInterviewDone(Integer deviceId) { return jni_deviceIsInterviewDone(jzway, deviceId); }
        public void deviceDelayCommunication(Integer deviceId, Integer delay) { jni_deviceDelayCommunication(jzway, deviceId, delay); }
        public void deviceAssignReturnRoute(Integer deviceId, Integer nodeId) { jni_deviceAssignReturnRoute(jzway, deviceId, nodeId); }
        public void deviceAssignPriorityReturnRoute(Integer deviceId, Integer nodeId, Integer repeater1, Integer repeater2, Integer repeater3, Integer repeater4) { jni_deviceAssignPriorityReturnRoute(jzway, deviceId, nodeId, repeater1, repeater2, repeater3, repeater4); }
        public void deviceDeleteReturnRoute(Integer deviceId) { jni_deviceDeleteReturnRoute(jzway, deviceId); }
        public void deviceAssignSUCReturnRoute(Integer deviceId) { jni_deviceAssignSUCReturnRoute(jzway, deviceId); }
        public void deviceAssignPrioritySUCReturnRoute(Integer deviceId, Integer repeater1, Integer repeater2, Integer repeater3, Integer repeater4) { jni_deviceAssignPrioritySUCReturnRoute(jzway, deviceId, repeater1, repeater2, repeater3, repeater4); }
        public void deviceDeleteSUCReturnRoute(Integer deviceId) { jni_deviceDeleteSUCReturnRoute(jzway, deviceId); }

        public final class Instance {
            private final Device device;

            public final Integer id;
            public final Data data;

            public Map<Integer, CommandClass> commandClasses;
            public Map<String, CommandClass> commandClassesByName;
            
            public Instance(ZWay zway, Device dev, Integer instance_id) throws Exception {
                device = dev;
                id = instance_id;
                try {
                    data = new Data("", dev.id, instance_id);
                } catch (Data.NotFound e) {
                    // should never happen
                    throw new RuntimeException();
                }
                commandClasses = new HashMap<>();
                commandClassesByName = new HashMap<>();
            }

            public class CommandClass {
                protected final Instance instance;

                public final Data data = null;
                
                public CommandClass(ZWay zway, Instance inst) throws Exception {
                    instance = inst;
                }

                public void interview(int deviceId, int instanceId, int ccId) { jni_commandInterview(jzway, deviceId, instanceId, ccId); }
            }

            /* BEGIN AUTOGENERATED CC TEMPLATE
            public final class %cc_capitalized_name% extends CommandClass {
                public final static int id = %cc_id%;
                public final Data data;

                public %cc_capitalized_name%(ZWay zway, Instance instance) throws Exception {
                    super(zway, instance);
                    try {
                        data = new Data("", instance.device.id, instance.id, %cc_id%);
                    } catch (Data.NotFound e) {
                        // should never happen
                        throw new RuntimeException();
                    }
                }

             * BEGIN AUTOGENERATED CMD TEMPLATE
                public void %function_command_camel_name%(%params_java_declarations%Object callbackArg) {
                    jni_cc_%function_short_camel_case_name%(jzway, instance.device.id, instance.id, %params_java%callbackArg);
                }
                public void %function_command_camel_name%(%params_java_declarations_no_comma%) {
                    jni_cc_%function_short_camel_case_name%(jzway, instance.device.id, instance.id, %params_java%0);
                }
                
             * END AUTOGENERATED CMD TEMPLATE
            }
            
             * END AUTOGENERATED CC TEMPLATE */
        }
    }
}
