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

`new ZWay` loads the library and initialize required strutures. This call expects the following arguments:
- Name of the instance (for logging purpose, any string can be used).
- Path to the Z-Wave hardware port.
- Baudrate (in most cases it is 115200 but for larg networks you might want to switch your hardware to a faster speed).
- Path to the configuration file where all interview information is saved. You can use the same folder as z-way-server is using to be able to run z-way-server and your app (but only one at a time!).
- Path to translation files (shipped with Z-Way).
- Path to the ZDDX database files (shipped with Z-Way). You can point to an empty folder to skip using this database (only old non-Z-Wave Plus devices require this database for proper operation).

`zway.discover()` start communications with the Z-Wave hardware. After this call the library will be notified about new events and creation/removal of devices/instances/Command Classes using callback functions.

This call is syncronous.

## Devices/Instances/CommandClasses add/removal callback

static class DeviceDemo implements ZWay.DeviceCallback {
    public void deviceCallback(Integer type, Integer deviceId, Integer instanceId, Integer commandClassId) {

Subscribing to device/instance/Command Class add/remove event:
```
MyCallback myCbk = new MyCallback();
zway.bind(myCbk);

static class MyCallback implements ZWay.DeviceCallback {
    public void deviceCallback(Integer type, Integer deviceId, Integer instanceId, Integer commandClassId) {
        ....
    }
}
```

Event type is one of the:
* `ZWay.deviceAdded` (device was added)
* `ZWay.deviceRemoved` (device was removed)
* `ZWay.instanceAdded` (instance was added)
* `ZWay.instanceRemoved` (instance was removed)
* `ZWay.commandAdded` (Command Class was added)
* `ZWay.commandRemoved` (Command Class was removed)

Unsubscribing the handler:
```
zway.unbind(myCbk);
```

## Network management and running the library

Network management functions:
```
zway.controller.setDefault(); // reset the Z-Wave chip to factory default
zway.controller.addNodeToNetwork(true); // Add node
zway.controller.removeNodeFromNetwork(true); // Remove node
````

Those functions are asynchronous and will return immediately.

Z-Way thread will stop if the hardware is removed (port has gone) or stop() was called. To check if the Z-Way thread is running, use:
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

To get the Command Class Id by Name and vice versa:
```
Integer id = ZWay.commandClassIdByName.get("switchBinary");
String name = ZWay.commandClassNameById.get(26);
```

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
ZWay.Data children[] = data.datagetChildren();
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
MyLevelChange myDataCbk = new MyLevelChange();
levelData.bind(myDataCbk);

static class MyLevelChange implements ZWay.DataCallback {
    public void dataCallback(ZWay.Data data, Integer type) {
        ....
    }
}
```
Change type is one of the:
* `ZWay.Data.updated` (data was updated)
* `ZWay.Data.invalidated` (data was marked as outdated and new value is expected)
* `ZWay.Data.deleted` (this is the last update of the device before it is deleted, all further operations on this object will raise NotAlive)
* `ZWay.Data.BchildCreated` (child data element was created)
* `ZWay.Data.phantomUpdate` (bit flag, set if the data value is updated, but the new value is the same as the old one)

Unsubscribing the handler:
```
levelData.unbind(myDataCbk);
```

## Calling Command Classes methods:

```
switchBinary.get();
switchBinary.set(s, 0);
```

Those functions are asynchronous and will return immediately. A callback will be called once the command was transmitted to the target devic.

The full list of methods is listed in the manual [Z-Way manual](https://z-wave.me/manual/z-way/Command_Class_Reference.html).

## Error handling

* `ZWay.Data.NotFound` is raised when data is not found.
* `ZWay.Data.NotAlive` data object was deleted in the underlying library and is not valid anymore

## Cleanup

TBD

# Building

## Ubuntu/Debian/Raspbian

### Install

Install JDK8

`sudo apt-get install openjdk-8-jdk`

[Install Z-Way library](https://z-wave.me/z-way/download-z-way/)

### Compiling

In Terminal in the wrapper folder:

`make clean all run`

### Running your or test project

Only one software at time can speak with the Z-WAve hardware port!

Stop Z-Way server (or disable the app using the Z-Wave hardware port) before running your app:

`sudo /etc/init.d/z-way-server stop`

Run the test project:

`make run`
