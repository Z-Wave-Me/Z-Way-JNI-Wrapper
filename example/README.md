# Z-Way Example

This project shows how to use the Z-Way Java wrapper

The source code of the Z-Way Java wrapper is located at https://github.com/Z-Wave-Me/Z-Way-JNI-Wrapper

Z-Way is a Z-Wave controller library written in C https://z-wave.me/z-way

# Build

## Install Maven 

```
sudo apt-get install maven git
```

## Install Z-Way

Follow the guide at https://z-wave.me/z-way/download-z-way/

## Download the example file

```
git clone https://github.com/Z-Wave-Me/Z-Way-JNI-Wrapper
cd Z-Way-JNI-Wrapper/example/
```

Checkout to the latest release version listed here: https://github.com/Z-Wave-Me/Z-Way-JNI-Wrapper/releases/latest

```
git checkout $(wget https://github.com/Z-Wave-Me/Z-Way-JNI-Wrapper/releases/latest --max-redirect 0 2>&1 | grep Location | awk '{print $2}' | awk -F/ '{ print $NF }')
```

## Define the Z-Wave port

If required, change the port in src/main/java/Main.java

## Build with Maven

```
mvn clean package
```

## Run

```
sudo LD_LIBRARY_PATH=/opt/z-way-server/libs java -Djava.library.path=/opt/z-way-server/libs -classpath target/z-way-example-*.jar Main
```
