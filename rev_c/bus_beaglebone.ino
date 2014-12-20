// Copyright (c) 2014 by Wayne C. Gramlich.  All rights reserved.
//
// This code drives the bus_beagle_bone_black board.

#include "Bus.h"
#include "Frame_Buffer.h"

#define TEST_BUS_OUTPUT 1
#define TEST_BUS_ECHO 2
#define TEST_BUS_COMMAND 3
#define TEST_BUS_BRIDGE 4

#define TEST TEST_BUS_OUTPUT

#define UART1_DISABLED

// Pin defintions for bus_beaglebone:
#define BUS_STANDBY       3
#define LED		 13

#define ADDRESS 0x21
#define LED_GET 0
#define LED_PUT 1
#define BUFFER_SIZE 16


// The *Bus* object is defined here:
NULL_UART null_uart;
AVR_UART1 avr_uart1(500000, (Character *)"9N1");
Bus bus(&avr_uart1, &null_uart);

// Set the *LED* to the value of *led*:
void led_set(Logical led) {
  if (led) {
    digitalWrite(LED, HIGH);
  } else {
    digitalWrite(LED, LOW);
  }
}

void led_blink(UShort on, UShort off) {
  while (1) {
    led_set((Logical)1);
    delay(on);
    led_set((Logical)0);
    delay(off);
  }
}

void bridge_host_to_bus() {
  Frame_Buffer			bus_frame_in;
  Frame_Buffer			bus_frame_out;
  Frame_Buffer			bus_frame_pending;
  UShort 			echo_suppress = 0xfefe;
  UShort			high_bits = 0xfefe;
  Frame_Buffer			host_frame_in;
  Frame_Buffer			host_frame_out;

  led_set((Logical)1);
  delay(200);
  led_set((Logical)0);

  // Just keep looping:
  while (1) {
    // Do we have an 8-bit byte to read from the host?:
    if (Serial.available() > 0 && !host_frame_in.is_full()) {
      // Receive 8-bit byte and save it into *host_frame_in*:
      UShort frame = (UShort)Serial.read();
      host_frame_in.append(frame);
    }

    // Do we have an 8-bit byte to send to the host?:
    if (!host_frame_out.is_empty() && Serial.room() >= 0) {
      // Send *frame* up to host:
      UShort frame = host_frame_out.lop();
      Serial.write(frame);
    }

    // Do we have a 9-bit byte to read from the bus?:
    if (bus.can_receive() && !bus_frame_in.is_full()) {
      // Recevie a bus *frame* and save it into *bus_frame_in*:
      UShort frame = bus.frame_get();
      bus_frame_in.append(frame);
    }

    // Is there a pending 9-bit frame to send to bus?:
    if (!bus_frame_out.is_empty() && bus.can_transmit()) {
      // Send *frame* out to the bus:
      UShort frame = bus_frame_out.lop();
      bus.frame_put(frame);

      // Remember that we want to *echo_suppress* *frame*:
      echo_suppress = frame;
    }

    // Forward all non echo suppressed traffic back up to host:
    if (!bus_frame_in.is_empty()) {
      // Grab a *frame* from *bus_frame_in*:
      UShort frame = bus_frame_in.lop();
      if (echo_suppress == (UShort)0xfefe) {
	// We are in non-*echo_suppress* mode.  We just forward
	// the byte up to the host:

	// We need to ensure that the 9th bit is not set:
	if ((frame & 0xff00) != 0) {
	  // Since we are the master, the 9th bit should never
	  // be set.  We indicate our confusion by setting the
	  // *led* high and masking off the 9th bit:
	  led_blink(200, 800);
	}

	// Send *frame* up to host:
	host_frame_out.append(frame & 0xff);
      } else {
	// We are in *echo_suppress* mode:

	// We need to check to ensure that the frame we sent
	// (=*echo_suppress*) is the same as the one we received
	// *frame*:
	if (echo_suppress != frame) {
	  // Since this should never happen, we indicate our
	  // confusion by setting *led* to high:
	  led_blink(500, 500);
	}

	// Now clear *echo_suppress*:
        echo_suppress = 0xfefe;
      }
    }

    if (!host_frame_in.is_empty() && !bus_frame_out.is_full()) {
      // Grab *frame* from *host_frame_in*:
      UShort frame = host_frame_in.lop();
	    
      // frames 0xc0 ... 0xc7 are reserved for "special" operations:
      if ((frame & 0xf8) == 0xc0) {
	switch (frame & 7) {
	  case 0:
	  case 1:
	  case 2:
	  case 3:
	    // 1100 00hh: Set *high_bits*:
	    // Set *high_bits* to (hh << 7):
	    high_bits = (frame & 3) << 7;
	    break;
	  case 4:
	    // Do bus discovery scan:
	    break;
	  case 5:
	    // Do bus break:
	    break;
	  case 6:
	    // Reserved:
	    led_set((Logical)0);
	    digitalWrite(LED, LOW);
	    break;
	  case 7:
	    // Reserved:
	    led_set((Logical)1);
	    break;
	}
      } else {
	if (high_bits == 0xfefe) {
	  // *high_bits* is not set, just forward *frame* to bus:
	  bus_frame_out.append(frame);
	} else {
	  // *high_bits* is set, forward modified *frame* to bus:
	  frame = high_bits | (frame & 0x7f);
	  bus_frame_out.append(frame);
	  high_bits = 0xfefe;
	}
      }
    }
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

  //Serial.print(" A:");
  //Serial.print(UCSR1A);
  //Serial.print(" B:");
  //Serial.print(UCSR1B);
  //Serial.print(" C:");
  //Serial.print(UCSR1C);
  //Serial.print(" H:");
  //Serial.print(UBRR1H);
  //Serial.print(" L:");
  //Serial.print(UBRR1L);
  //Serial.print("\r\n");

  // Force the standby pin on the CAN transeciever to *LOW* to force it
  // into active mode:
  pinMode(BUS_STANDBY, OUTPUT);
  digitalWrite(BUS_STANDBY, LOW);

  // Enable/disable interrupts as needed:
  switch (TEST) {
    case TEST_BUS_OUTPUT:
    case TEST_BUS_ECHO:
    case TEST_BUS_COMMAND:
    case TEST_BUS_BRIDGE:
      bus.interrupts_disable();
      break;
  }
}

// The *loop* routine is repeatably called until the processor
// looses power.  *setup* is called first.

void loop() {
  switch (TEST) {
    case TEST_BUS_BRIDGE: {
      bridge_host_to_bus();
      break;
    }
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

      // *character* is a static variable:
      static Character character;

      // Make sure *character* is "in bounds":
      if (character < '@' || character > '_') {
	character = '@';
      }

      // Output *character* to bus:
      avr_uart1.frame_put((UShort)character);

      // Set LED to be the same as the LSB of *frame*:
      led_set((character & 1) != 0);
      delay(100);

      // Output *frame* back to user for debugging:
      Serial.write(character);
      if (character >= '_') {
	Serial.write("\r\n");
        character = '@';
      } else {
	character += 1;
      }

      // Set LED to be the opposite of the *frame* LSB:
      led_set((character & 1) == 0);
      delay(100);

      break;
    }
  }
}

