CC=gcc
WOLF_INSTALL_DIR=/usr/local
CFLAGS=-I$(WOLF_INSTALL_DIR)/include -Wall -DCERT_PATH=\"tmp_certs_and_keys/\"
LIBS=-L$(WOLF_INSTALL_DIR)/lib -lwolfssl -lwolftpm -lwolfmqtt

mqtt_tpm_simple: mqtt_tpm_simple.c
	gcc $^ -g -o $@ $(CFLAGS) $(LIBS)
