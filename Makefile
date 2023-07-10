### Variables ###
TARGET = libjzway
TARGET_DIR = $(TARGET)
TARGET_SO = $(TARGET_DIR)/$(TARGET).so

JAVA_DIR = src/main/java/me/zwave/zway
SCRIPT_DIR = src/main/scripts
NATIVE_DIR = src/main/native

# Z-Way
ZWAY_ROOT = /opt/z-way-server/

ZWAY_INC_DIR = $(ZWAY_ROOT)/libzway
ZWAY_LIB_DIR = $(ZWAY_ROOT)/libs

ZWAY_LIBS = zway zs2 zcommons

# JNI
JNI_ROOT = /usr/lib/jvm/java-8-openjdk-amd64
JNI_INCLUDES = -I$(JNI_ROOT)/include/ -I$(JNI_ROOT)/include/linux

C_OBJECTS = $(patsubst %.c,%.o,$(wildcard $(NATIVE_DIR)/*.c))
J_OBJECTS = $(patsubst %.java,%.class,$(wildcard $(JAVA_DIR)/*.java))

INCLUDES = -I$(ZWAY_INC_DIR) $(JNI_INCLUDES)

LIBDIR = -L$(ZWAY_LIB_DIR)
LIBS += $(patsubst %,-l%,$(ZWAY_LIBS))

CFLAGS += -Wall -Werror -Wextra

LDFLAGS += -shared
LIBS += -lpthread -lxml2 -lz -lm -lcrypto -larchive

### Targets ###

all: prepare $(TARGET_SO) $(J_OBJECTS) copy

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
	rm -fR *.o *.class $(TARGET_DIR) $(TARGET_SO) hs_err_pid*.log
	python2 $(SCRIPT_DIR)/autogenerate_code.py clean $(ZWAY_ROOT) $(NATIVE_DIR)/jni_zway_wrapper.c $(JAVA_DIR)/ZWay.java

copy:
	cp $(patsubst %,$(ZWAY_LIB_DIR)/lib%.so,$(ZWAY_LIBS)) $(TARGET_DIR)

run:
	sudo LD_LIBRARY_PATH=$(ZWAY_LIB_DIR) java -Djava.library.path=$(TARGET_DIR) Main

.PHONY: all prepare clean copy run
