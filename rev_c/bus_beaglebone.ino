// Copyright (c) 2014 by Wayne C. Gramlich.  All rights reserved.
//
// This code drives the bus_beagle_bone_black board.

#include "Bus.h"

#define TEST_BUS_OUTPUT 1
#define TEST_BUS_ECHO 2
#define TEST_BUS_COMMAND 3

#define TEST TEST_BUS_COMMAND

#define UART1_DISABLED

// Pin defintions for bus_beaglebone:
#define BUS_STANDBY       3
#define LED		 13

#define ADDRESS 0x21
#define LED_GET 0
#define LED_PUT 1
#define BUFFER_SIZE 16


// The *Bus* object is defined here:
Bus bus;

// Set the *LED* to the value of *led*:
void led_set(Logical led) {
  if (led) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}

// The *setup* routine runs on power up and when you press reset:

void setup() {
  // Initialize the debugging serial port:
  Serial.begin(115200);
  Serial.write("\r\nbus_beaglebone\r\n");

  // Turn the *LED* on:
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Force the standby pin on the CAN transeciever to *LOW* to force it
  // into active mode:
  pinMode(BUS_STANDBY, OUTPUT);
  digitalWrite(BUS_STANDBY, LOW);

  // Enable/disable interrupts as needed:
  switch (TEST) {
    case TEST_BUS_OUTPUT:
    case TEST_BUS_ECHO:
    case TEST_BUS_COMMAND:
      bus.interrupts_disable();
      break;
  }
}

// The *loop* routine is repeatably called until the processor
// looses power.  *setup* is called first.

void loop() {
  switch (TEST) {
    case TEST_BUS_COMMAND: {
      // Blink the *LED* some:
      Serial.write("[");

      // Set the *LED* to *HIGH* and then wait a little:
      bus.command_ubyte_put(ADDRESS, LED_PUT, HIGH);
      Logical led_get = bus.command_ubyte_get(ADDRESS, LED_GET);
      led_set(led_get);
      delay(100);

      // Set the *LED* to *LOW* and then wait a little:
      bus.command_ubyte_put(ADDRESS, LED_PUT, LOW);
      led_get = bus.command_ubyte_get(ADDRESS, LED_GET);
      led_set(led_get);
      delay(100);

      Serial.write("]\r\n\r\n");
      break;
    }
    case TEST_BUS_ECHO: {
      static UShort send_frame;

      if (send_frame < '@' || send_frame > '_') {
	send_frame = (UShort)'@';
      }

      // Output *frame*:
      bus.frame_put(send_frame);
      UShort echo_frame = bus.frame_get();
      if (echo_frame != send_frame) {
        Serial.write('!');
      }
      UShort receive_frame = bus.frame_get();
  
      if (send_frame == receive_frame) {
	Serial.write(receive_frame);
      } else {
	Serial.write('$');
      }
  
      led_set((receive_frame & 1) == 0);
      delay(100);

      led_set((receive_frame & 1) != 0);
      delay(100);

      if (send_frame == '_') {
	Serial.write("\r\n");
	send_frame = (UShort)'@';
      } else {
	send_frame += (UShort)1;
      }
      break;
    }
    case TEST_BUS_OUTPUT: {
      // This verision of loop simply outputs 8-bit characters (in 9-bit mode)
      // to the bus starting from '@' to '_' and repeating.  The primary purpose
      // is to verify that both UART's are properly initialized to reasonable
      // baud rates.  We also ensure that the bus is terminated and the
      // CAN bus transceiver is on.

      // *frame* is a static variable:
      static UShort frame;

      // Make sure *frame* is "in bounds":
      if (frame < '@' || frame > '_') {
	frame = '@';
      }

      // Output *frame* to bus:
      bus.frame_put(frame);

      // Set LED to be the same as the LSB of *frame*:
      led_set((frame & 1) != 0);
      delay(100);

      // Output *frame* back to user for debugging:
      Serial.write(frame);
      if (frame >= '_') {
	Serial.write("\r\n");
        frame = '@';
      } else {
	frame += 1;
      }

      // Set LED to be the opposite of the *frame* LSB:
      led_set((frame & 1) == 0);
      delay(100);

      break;
    }
  }
}

