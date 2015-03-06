ARDUINO_DIR = ../Arduino
ARDMK_DIR = ../Arduino-Makefile
ALTERNATE_CORE = ubiquity/avr
ALTERNATE_CORE_PATH = $(ARDUINO_DIR)/hardware/$(ALTERNATE_CORE)
BOARD_TAG = bus_beaglebone
ARDUINO_PORT = /dev/ttyUSB0
ARDUINO_LIBS := . ../bus_slave ../bus_server
USER_LIB_PATH = $(realpath .)
MCU=atmega324p
F_CPU = 16000000L
VARIANT = atmega324p
AVRDUDE_ARD_PROGRAMMER = arduino
AVRDUDE_ISP_BAUDRATE = 115200
AVRDUDE_ARD_BAUDRATE = 115200

include $(ARDMK_DIR)/Arduino.mk

