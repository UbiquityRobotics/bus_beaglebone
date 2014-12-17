// Copyright (c) 2014 by Wayne C. Gramlich.  All rights reserved.
//
// This code drives the bus_beagle_bone_black board.

#include "Bus.h"

#define TEST_BUS_OUTPUT 1
#define TEST_BUS_ECHO 2
#define TEST_BUS_COMMAND 3

#define TEST TEST_BUS_COMMAND

#define UART1_DISABLED

#define BUS_STANDBY       3
#define BUS_NO_ADDRESS   ((UShort)-1)
#define LED		 13

#define ADDRESS 0x21
#define LED_GET 0
#define LED_PUT 1
#define BUFFER_SIZE 16


Bus bus;

UShort Bus_Serial__address = BUS_NO_ADDRESS;
//Byte Bus_Serial__put_index = 0;
Byte Bus_Serial__put_check_sum = 0;
Byte Bus_Serial__get_index = 0;
Byte Bus_Serial__get_check_sum = 0;
Byte Bus_Serial__get_size = 0;
//Byte Bus_Serial__put_buffer[BUFFER_SIZE];
Byte Bus_Serial__get_buffer[BUFFER_SIZE];

Logical Bus_Serial__address_put() {
  Logical error = (Logical)0;

  // Make sure that the input buffer has nothing:
  while (bus.can_receive()) {
    bus.frame_get();
  }

  Serial.write('R');
  Serial.print(Bus_Serial__address, HEX);

  // Send the address and get the *address_echo*:
  bus.frame_put(Bus_Serial__address);
  UShort address_echo = bus.frame_get();
  if (Bus_Serial__address != address_echo) {
    Serial.write('!');
    error = (Logical)1;
  }

  // If the address is an upper half address, wait for an acknoledge:
  if ((Bus_Serial__address & 0x80) == 0) {
    UShort address_acknowledge = bus.frame_get();
	
    Serial.write('r');
    Serial.print(address_acknowledge, HEX);

    if (address_acknowledge != 0) {
      Serial.write('!');
      error = (Logical)1;
    }
  }
  return error;
}

Logical Bus_Serial__buffer_get() {
  Logical error = (Logical)0;
  // Read combined *count_check_sum*:
  Byte count_check_sum = (Byte)bus.frame_get();

  Serial.write(':');
  Serial.write('G');
  Serial.print(count_check_sum, HEX);

  // Split out the *size* and *checksum*:
  Byte size = (count_check_sum >> 4) & 0xf;
  Byte check_sum = count_check_sum & 0xf;

  // Read in *size* bytes and compute *computed_check_sume;
  Byte computed_check_sum = 0;
  Serial.write(';');
  for (Byte index = 0; index < size; index++) {
    Byte byte = (Byte)bus.frame_get();
    Bus_Serial__get_buffer[index] = byte;
    computed_check_sum += byte;

    Serial.write('G');
    Serial.print(byte, HEX);
  }
  computed_check_sum += computed_check_sum >> 4;
  computed_check_sum &= 0xf;

  Bus_Serial__get_index = 0;
  Bus_Serial__get_size = size;

  if (check_sum != computed_check_sum) {
    Serial.write('!');
    error = (Logical)1;
  }
  return error;
}

//Logical Bus_Serial__flush(void) {
//  Serial.write("{");
//  Logical error = (Logical)1;
//  for (Byte retries = 0; error && retries < 5; retries++) {
//    Serial.write("<");
//
//    // Select the address:
//    error = Bus_Serial__address_put();
//    if (error) {
//      Serial.write('!');
//    } else {
//      error = Bus_Serial__buffer_put();
//      if (error) {
//        Serial.write('@');
//      } else {
//        error = Bus_Serial__buffer_get();
//      }
//    }
//
//    Serial.write(">");
//  }
//  Serial.write("}\r\n");
//  if (error) {
//    Serial.write('#');
//  }
//  return error;
//}

//void Bus_Serial__byte_put(Byte byte) {
//  Bus_Serial__put_buffer[Bus_Serial__put_index++] = byte;
//  Bus_Serial__put_check_sum += byte;
//}

//Byte Bus_Serial__byte_get(void) {
//  Serial.write('e');
//  Bus_Serial__flush();
//  Serial.write('f');
//  Byte byte = Bus_Serial__get_buffer[Bus_Serial__get_index++];
//  Serial.write('g');
//  return byte;
//}

//void Bus_Serial__command_byte_put(UShort address, Byte command, Byte byte) {
//  bus.command_put(address, command);
//  bus.byte_put(byte);
//}

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
      bus.command_ubyte_put(ADDRESS, LED_PUT, (UByte)HIGH);
      Logical led_get = bus.command_ubyte_get(ADDRESS, LED_GET);
      led_set(led_get);
      delay(100);

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

