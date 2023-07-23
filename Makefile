### Variables ###
TARGET = libjzway
TARGET_DIR = $(TARGET)
TARGET_SO = $(TARGET_DIR)/$(TARGET).so

JAVA_DIR = src/main/java/me/zwave/zway
JAVA_EXAMPLE_DIR = example/src/main/java/
SCRIPT_DIR = src/main/scripts
NATIVE_DIR = src/main/native

# Z-Way
ZWAY_ROOT ?= /opt/z-way-server

ZWAY_INC_DIRS ?= $(ZWAY_ROOT)/libzway
ZWAY_LIB_DIRS ?= $(ZWAY_ROOT)/libs

ZWAY_LIBS = zway zs2 zcommons

# JNI
JNI_ROOT = /usr/lib/jvm/java-8-openjdk-amd64
JNI_INCLUDES = -I$(JNI_ROOT)/include/ -I$(JNI_ROOT)/include/linux

C_OBJECTS = $(patsubst %.c,%.o,$(wildcard $(NATIVE_DIR)/*.c))
J_OBJECTS = $(patsubst %.java,%.class,$(wildcard $(JAVA_DIR)/*.java))

INCLUDES = $(patsubst %,-I%,$(ZWAY_INC_DIRS)) $(JNI_INCLUDES)

LIBDIR = $(patsubst %,-L%,$(ZWAY_LIB_DIRS))
LIBS += $(patsubst %,-l%,$(ZWAY_LIBS))

CFLAGS += -Wall -Werror -Wextra

LDFLAGS += -shared
LIBS += -lpthread -lxml2 -lz -lm -lcrypto -larchive

### Targets ###

all: prepare $(TARGET_SO) $(J_OBJECTS)

$(TARGET_SO): $(C_OBJECTS)
	$(CC) $(LIBDIR) $(TARGET_LIBDIR) $(LDFLAGS) $(TARGET_ARCH) -o $@ $< $(LIBS)

%.class: %.java
	$(JNI_ROOT)/bin/javac $<

%.o: %.c $(wildcard *.h)
	$(CC) $(INCLUDES) $(TARGET_INCLUDES) $(CFLAGS) $(TARGET_ARCH) -c $< -o $@

prepare:
	python2 $(SCRIPT_DIR)/autogenerate_code.py generate $(ZWAY_ROOT) $(NATIVE_DIR)/jni_zway_wrapper.c $(JAVA_DIR)/ZWay.java
	mkdir -p $(TARGET)
	
clean:
	rm -fR src/native/*.o $(TARGET_DIR) $(TARGET_SO)
	rm -fR $(JAVE_DIR)/*.class $(JAVA_EXAMPLE_DIR)/*.class target example/target hs_err_pid*.log
	python2 $(SCRIPT_DIR)/autogenerate_code.py clean $(ZWAY_ROOT) $(NATIVE_DIR)/jni_zway_wrapper.c $(JAVA_DIR)/ZWay.java

mvn:
	mvn clean compile package install

run:
	#mvn -f example clean package
	sudo LD_LIBRARY_PATH=$(TARGET_DIR):$(subst  ,:,$(ZWAY_LIB_DIRS)) java -Djava.library.path=$(TARGET_DIR) -cp example/target/z-way-example-4.1.0.jar Main

.PHONY: all prepare clean run
