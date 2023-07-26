# Z-Way JNI Wrapper

Z-Way JNI Wrapper is a Java Native Interface wrapper of the Z-Wave.Me Z-Way C library providing Java API.

# Using

## Initializing the library
Import the library using `import me.zwave.zway.*;`

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
- Baud rate (in most cases it is 115200, but for large networks you might want to switch your hardware to a faster speed).
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

## Termination callback

Termination callback when Z-Way is terminated by user call or by port disconnect (for example, USB disconnection).

You can catch this event using:
```
OnTermination onTermination = new OnTermination();
zway.bind(onTermination);

static class OnTermination implements ZWay.TerminateCallback {
	public void terminateCallback() {
		....
	}
}
```

Unsubscribing the handler:
```
zway.unbind(onTermination);
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
ZWay.Device.Instance.SwitchBinary switchBinary = (ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(instanceId).commandClassesByName.get("switchBinary");
ZWay.Device.Instance.SwitchBinary switchBinary = (ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(instanceId).commandClassesById.get(0x25);
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
* `ZWay.Data.childCreated` (child data element was created)
* `ZWay.Data.phantomUpdate` (bit flag, set if the data value is updated, but the new value is the same as the old one)

Unsubscribing the handler:
```
levelData.unbind(myDataCbk);
```

## Calling Command Classes methods:

```
ZWay.Device.Instance.SwitchBinary switchBinary = (ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(instanceId).commandClassesByName.get("switchBinary");
switchBinary.get();
switchBinary.set(s, 0);
```

If a callback upon execution is needed:
```
switchBinary.get(callbackArg);
switchBinary.set(s, 0, callbackArg);
```

Those functions are asynchronous and will return immediately. A callback will be called once the command was transmitted to the target devic.

The full list of methods is listed in the manual [Z-Way manual](https://z-wave.me/manual/z-way/Command_Class_Reference.html).

## Calling Z-Wave Serial API commands (Function Classes):

```
zway.setPriorityRoute(nodeId, repeater1, repeater2, repeater3, repeater4, route_speed);
zway.setGetLongRangeChannel(channel);
```

If a callback upon execution is needed:
```
callback = new myFuncCallback();
zway.setPriorityRoute(nodeId, repeater1, repeater2, repeater3, repeater4, route_speed, callbackArg);
zway.setGetLongRangeChannel(channel, callbackArg);
```

Those functions are asynchronous and will return immediately. A callback will be called once the command was transmitted to the target devic.

The full list of methods is listed in the manual [Z-Way manual](https://z-wave.me/manual/z-way/Function_Class_Reference.html).

## Command Class and Function Class status callbacks

Command Class method and Function Class calls are asyncronous. It is possible to get notified upon the execution of the command. For this subscribe to the ZWay.StatusCallback and pass an additional parameter to the call (any primitive or object).

Upon the execution (successful or non-successful) the statusCallback callback will be called with the boolean result argument and the object passed during the original call.

Subscribing to execution status events:
```
MyStatusCallback statusCbk = new MyStatusCallback();
zway.bind(statusCbk);

static class MyStatusCallback implements ZWay.StatusCallback {
	public void statusCallback(boolean result, Object obj) {
		// result true for success and false for failure
		// obj is the object passed as callbackArg
		....
	}
}
```

Call the Command Class or Function Class (callbackArg is the object passed to statusCallback on callback):
```
	((ZWay.Device.Instance.SwitchBinary) zway.devices.get(nodeId).instances.get(0).commandClassesByName.get("switchBinary")).set(true, 0, callbackArg);
```

Unsubscribing the handler:
```
zway.unbind(statusCbk);
```

Note that for Set and Get commands the callback will be called on delivery confirmation and not on the reply. Replies are to be monitored by subscribing to the corresponding Data element change.

## Error handling

* `ZWay.Data.NotFound` is raised when data is not found.
* `ZWay.Data.NotAlive` data object was deleted in the underlying library and is not valid anymore

## Cleanup

Memory is cleaned with a finalizer on garbage colleaction of each ZWay.Data object.

To stop Z-Way library, use `zway.stop()` and `zway.terminate()` functions.

# Dependency

This library can be built from sources (see *Building* section below) or added via Maven dependecies.

## Maven

```
<dependency>
  <groupId>me.zwave</groupId>
  <artifactId>zway</artifactId>
  <version>4.1.1</version>
</dependency>
```

## Gradle

```
compile group: 'me.zwave', name: 'zway', version: '4.1.1'
```

# Building

## Ubuntu/Debian/Raspbian

### Building the library

Produce JNI library:

`make clean all`

You might need to specify the path to the jni folder:

`JNI_ROOT=/usr/lib/jvm/java-8-openjdk-armhf make clean all`

### Building the JAR package

Produce a JAR package using Maven:

`make mvn`

### Install

Install JDK8

`sudo apt-get install openjdk-8-jdk`

[Install Z-Way library](https://z-wave.me/z-way/download-z-way/)

Install Maven

`sudo apt-get install maven`

### Running your or test project

*Only one software at time can speak with the Z-WAve hardware port!*

Stop Z-Way server (or disable the app using the Z-Wave hardware port) before running your app:

`sudo /etc/init.d/z-way-server stop`

Run the test project:

`make mvn run`
