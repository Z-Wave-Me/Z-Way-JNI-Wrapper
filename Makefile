### Variables ###
TARGET = libjzway
TARGET_DIR = $(TARGET)
TARGET_SO = $(TARGET_DIR)/$(TARGET).so

DOMAIN = me/zwave/zway
JAVA_DIR = src/main/java/$(DOMAIN)
JAVA_EXAMPLE_DIR = example/src/main/java/
SCRIPT_DIR = src/main/scripts
NATIVE_DIR = src/main/native

# Z-Way
ifeq ($(WITH_JNI),yes)
	ZWAY_ROOT = ../z-way
	ZWAY_INC_DIRS = ../z-commons ../z-way ../z-way/FunctionClasses ../z-way/CommandClasses ../z-s2
	ZWAY_LIB_DIRS = ../z-commons ../z-way ../z-s2
	ZWAY_VERSION = $(shell git -C .. tag | grep 'v[0-9]\.[0-9]\.[0-9]' | tail -n 1 | cut -b 2-)
else ifeq ($(TARGET_ARCH_NAME),)
	ZWAY_ROOT = /opt/z-way-server
	ZWAY_INC_DIRS = $(ZWAY_ROOT)/libzway
	ZWAY_LIB_DIRS = $(ZWAY_ROOT)/libs
	ZWAY_VERSION = 
endif

ZWAY_LIBS = zway zs2 zcommons

# JNI
JNI_ROOT ?= $(shell update-alternatives --query javadoc | grep Value: | head -n1 | sed 's/Value: //' | sed 's|bin/javadoc$$||')
JNI_INCLUDES = -I$(JNI_ROOT)/include/ -I$(JNI_ROOT)/include/linux

C_OBJECTS = $(patsubst %.c,%.o,$(wildcard $(NATIVE_DIR)/*.c))
J_OBJECTS = $(patsubst %.java,%.class,$(wildcard $(JAVA_DIR)/*.java))

INCLUDES = $(patsubst %,-I%,$(ZWAY_INC_DIRS)) $(JNI_INCLUDES)

LIBDIR = $(patsubst %,-L%,$(ZWAY_LIB_DIRS))
LIBS += $(patsubst %,-l%,$(ZWAY_LIBS))

CFLAGS += -Wall -Werror -Wextra

LDFLAGS += -shared
LIBS += -lpthread -lxml2 -lz -lm -lcrypto -larchive

ifeq ($(WITH_JNI),yes)
	# for execution from inside Z-Way build
	TARGET_ALL = $(TARGET_SO)
else ifeq ($(TARGET_ARCH_NAME),)
	# for standalone execution
	VERBOSE_ECHO = @true
	TARGET_ALL = $(TARGET_SO) $(J_OBJECTS)
endif

# Java

export JAVA_HOME ?= $(JNI_ROOT)

### Targets ###

all: $(TARGET_ALL)

$(TARGET_SO): $(C_OBJECTS)
	$(VERBOSE_ECHO) $(TARGET) [CC] $@
	$(CC) $(LIBDIR) $(TARGET_LIBDIR) $(LDFLAGS) $(TARGET_ARCH) -o $@ $< $(LIBS)

%.o: %.c $(wildcard *.h) prepare
	$(VERBOSE_ECHO) $(TARGET) [CC] $<
	$(CC) $(INCLUDES) $(TARGET_INCLUDES) $(CFLAGS) $(TARGET_ARCH) -c $< -o $@

%.class: %.java
	$(JNI_ROOT)/bin/javac $<

prepare:
	python2 $(SCRIPT_DIR)/autogenerate_code.py generate $(ZWAY_ROOT) $(NATIVE_DIR)/jni_zway_wrapper.c $(JAVA_DIR)/ZWay.java
	if [ -n "$(ZWAY_VERSION)" ]; then sed -i '0,/<version>/ { s|<version>.*</version>|<version>'"$(ZWAY_VERSION)"'</version>| }' pom.xml; fi
	mkdir -p $(TARGET)
	
clean:
	rm -fR $(NATIVE_DIR)/*.o $(TARGET_DIR) $(TARGET_SO)
	rm -fR $(JAVA_DIR)/*.class $(JAVA_EXAMPLE_DIR)/$(DOMAIN)/*.class target example/target hs_err_pid*.log
	test -z "$(ZWAY_ROOT)" || python2 $(SCRIPT_DIR)/autogenerate_code.py clean $(ZWAY_ROOT) $(NATIVE_DIR)/jni_zway_wrapper.c $(JAVA_DIR)/ZWay.java

mvn:
	mvn -Dzway.root=../z-way clean compile package install

deploy:
	mvn clean deploy -Pci-cd

run: mvn
	mvn -f example clean package
	sudo LD_LIBRARY_PATH=$(TARGET_DIR):$(subst  ,:,$(ZWAY_LIB_DIRS)) java -Djava.library.path=$(TARGET_DIR) -classpath example/target/z-way-example-*.jar Main

.PHONY: all prepare clean run
