
# MQTTC_PREFIX may need to be changed
MQTTC_PREFIX := /usr/local


client_SOURCES := client.c
client_LDFLAGS := -L$(MQTTC_PREFIX)/lib -lpaho-mqtt3c -Wl,-rpath,$(MQTTC_PREFIX)/lib
client_CPPFLAGS := -I$(MQTTC_PREFIX)/include


include ../../quickbuild.make
