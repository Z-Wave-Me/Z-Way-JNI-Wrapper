# Z-Way JNI Wrapper

Z-Way JNI Wrapper is a Java Native Interface wrapper of the Z-Wave.Me Z-Way C library providing Java API.

# Using

## Initializing the library
Import the library using `import ZWayJNIWrapper.*;`

Create the Z-Way object and start communications with the Z-Wave chip.

```
ZWay zway;
try {
    zway = new ZWay("zway", "/dev/ttyACM0", 115200, "z-way-root/config", "z-way-root/translations", "z-way-root/ZDDX", 0);
} catch (java.lang.Exception e) {
    System.out.println(e);
}

try {
    zway.discover();
} catch (java.lang.Exception e) {
    System.out.println(e);
}
```

## Network management and running the library

Network management functions:
```
zway.controller.setDefault(); // reset the Z-Wave chip to factory default
zway.controller.addNodeToNetwork(true); // Add node
zway.controller.removeNodeFromNetwork(true); // Remove node
````

More functions:
```
zway.isRunning(); // returns true if Z-Wave is running or false when Z-Way has stopped
```

## Working with devices, instances and Command Classes

Getting controller object, device object, Instance Object and Command Class object:
```
ZWay.Controller controller = zway.controller;
ZWay.Device device = zway.devices.get(nodeId);
ZWay.Device.Instance instance = zway.devices.get(nodeId).instances.get(instanceId);
ZWay.Device.Instance.SwitchBinary switchBinary = zway.devices.get(nodeId).instances.get(instanceId).commandClassesByName.get("switchBinary");
ZWay.Device.Instance.SwitchBinary switchBinary = zway.devices.get(nodeId).instances.get(instanceId).commandClassesById.get(0x25);
```
devices, instances and commandClassesBy* are iterable lists.

## Working with Data

Getting data on controller, device, instance and command class:
```
ZWay.Data data = controller.data;
ZWay.Data data = device.data;
ZWay.Data data = instance.data;
ZWay.Data data = commandClass.data;
```

Finding data object subtree:
```
ZWay.Data levelData = switchBinary.data.get("level");
ZWay.Data config11Data = configuration.data.get("11.val");
ZWay.Data config11Data = configuration.data.get("11").get("val");
Data children[] = data.datagetChildren();
```

Getting data type and values:
```
Type t = data.getValueType();
String ts = data.getValueTypeStr();

Boolean b = data.getBool();
Integer d = data.getInt();
Float f = data.getFloat();
String s = data.getString();
Integer ai[] = data.getByteList();
Integer ai[] = data.getIntList();
Float af[] = data.getFloatList();
String as[] = data.getStringList();
```

Setting data values (in most cases you don't need to do it):
```
data.setBool(true);
data.setInt(55);
data.setFloat(55.66);
data.setString("string");
data.setByteList({1, 2, 3});
data.setIntList({1, 2, 3});
data.setFloatList({1.1, 2.2, 3.3});
data.setStringList({"1", "2", "3"});
data.setEmpty();
```

Subscribing to data change:
```
levelData.bind(new MyLevelChange());

static class MyLevelChange implements ZWay.DataCallback {
    public void dataCallback(ZWay.Data data, Integer type) {
        ....
    }
}
```
Change type is one of the:
* `updated` (data was updated)
* `invalidated` (data was marked as outdated and new value is expected)
* `deleted` (this is the last update of the device before it is deleted, all further operations on this object will raise NotAlive)
* `childCreated` (child data element was created)
* `phantomUpdate` (bit flag, set if the data value is updated, but the new value is the same as the old one)

## Calling Command Classes methods:
```
switchBinary.get();
switchBinary.set(s, 0);
```
The full list of methods is listed in the manual [Z-Way manual](https://z-wave.me/manual/z-way/Command_Class_Reference.html).

## Error handling

* `ZWay.Data.NotFound` is raised when data is not found.
* `ZWay.Data.NotAlive` data object was deleted in the underlying library and is not valid anymore

# Building

## Ubuntu

### Install

Install JDK8

`sudo apt-get install openjdk-8-jdk`

[Install Z-Way library](https://z-wave.me/z-way/download-z-way/)

### Compile and run

In Terminal in the wrapper folder:

`make clean all run`
