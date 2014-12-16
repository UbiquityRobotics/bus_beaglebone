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
#define BUS_BAUD_500KBPS  3 // Fosc=16MHz & U2X1=1
#define BUS_NO_ADDRESS   ((UShort)-1)
#define LED		 13

Bus bus;

void Bus_Serial__initialize(Byte baud_lsb) {
    // Clear out the three USART1 control/status registers:
    UCSR1A = 0;
    UCSR1B = 0;
    UCSR1C = 0;

    // Set the UART1 into Asynchronous mode (set PC:UMSEL11,PC:UMSEL10) = (0,0):
    //UCSR1C &= ~(1<<UMSEL11);
    //UCSR1C &= ~(1<<UMSEL10);

    // Set the UART1 into no parity mode (set PC:UPM11,PC:UPM10) = (0,0):
    //UCSR1C &= ~(1<<UPM11);
    //UCSR1C &= ~(1<<UPM10);

    // Set the UART1 to have 1 stop bit (Set PC:USBS1) = (0):
    //UCSR1C &= ~(1<<USBS1);

    // Put USART1 into 9 bit mode (set PB:UCSZ12,PC:UCSZ11,PC:UCSZ10) = (1,1,1):
    UCSR1B |= 1<<UCSZ12;
    UCSR1C |= 1<<UCSZ11 | 1<<UCSZ10;

    // Ignore Clock Polarity:
    //UCSR1C &= ~(1<<UCPOL1);

    // Disable multi-processor comm. mode (PA:MCPM1) = (0)
    //UCSR1A &= ~(1<<MPCM1);

    // Set UART1 baud rate for double data rate:
    UBRR1H = 0;
    UBRR1L = baud_lsb;
    UCSR1A |= 1<<U2X1;

    // Enable UART1:
    UCSR1B |= 1<<RXEN1;
    UCSR1B |= 1<<TXEN1;
} 

Logical Bus_Serial__can_receive(void) {
    Logical can_receive = (Logical)0;
    if ((UCSR1A & (1<<RXC1)) != 0) {
	can_receive = (Logical)1;
    }
    return can_receive;
}

Logical Bus_Serial__can_transmit(void) {
    Logical can_transmit = (Logical)0;
    if ((UCSR1A & (1<<UDRE1)) != 0) {
	can_transmit = (Logical)1;
    }
    return can_transmit;
}

UShort Bus_Serial__frame_get(void) {
    // Wait for a frame (9-bits) to show up in the receive buffer:
    while (!((UCSR1A & (1<<RXC1)) != 0)) {
	// Do nothing:
    }

    // Deal with address bit:
    UShort frame = 0;
    if ((UCSR1B & (1<<RXB81)) != 0) {
	// Set the address bit:
	frame = 0x100;
    }

    // Read in the rest of the *frame*:
    frame |= (UShort)UDR1;
    return frame;
}

void Bus_Serial__frame_put(UShort frame) {
    // Wait for space in the transmit buffer:
    while (!((UCSR1A & (1<<UDRE1)) != 0)) {
        // do_nothing:
    }

    // Properly deal with adress bit:
    if ((frame & 0x100) == 0) {
	// No address bit:
	UCSR1B &= ~(1<<TXB81);
    } else {
	// Set the address bit:
	UCSR1B |= 1<<TXB81;
    }

    // Force *frame* out:
    UDR1 = (Byte)frame;
}

// The setup routine runs once when you press reset:
void setup() {
  // Initialize the debugging serial port:
  Serial.begin(115200);
  Serial.write("\r\nbus_beaglebone\r\n");

  // Initialzed the bus serial port:
  //Bus_Serial__initialize(BUS_BAUD_500KBPS);

  // Turn the *LED* on:
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Force the standby pin on the CAN transeciever to low to force it
  // into active mode:
  pinMode(BUS_STANDBY, OUTPUT);
  digitalWrite(BUS_STANDBY, LOW);

  switch (TEST) {
    case TEST_BUS_OUTPUT:
    case TEST_BUS_ECHO:
    case TEST_BUS_COMMAND:
      bus.interrupts_disable();
      break;
  }
}

// The loop routine runs over and over again forever:
//UShort send_frame = (UShort)'@';

#define ADDRESS ((UShort)0x121)
#define LED_GET 0
#define LED_PUT 1
#define BUFFER_SIZE 16

UShort Bus_Serial__address = BUS_NO_ADDRESS;
Byte Bus_Serial__put_index = 0;
Byte Bus_Serial__put_check_sum = 0;
Byte Bus_Serial__get_index = 0;
Byte Bus_Serial__get_check_sum = 0;
Byte Bus_Serial__get_size = 0;
Byte Bus_Serial__put_buffer[BUFFER_SIZE];
Byte Bus_Serial__get_buffer[BUFFER_SIZE];

Logical Bus_Serial__address_put() {
  Logical error = (Logical)0;

  // Make sure that the input buffer has nothing:
  while (Bus_Serial__can_receive()) {
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

// Send the contents of *Bus_Serial__put_buffer* to the bus:
Logical Bus_Serial__buffer_put() {
  Logical error = (Logical)0;

  // Compute the 4-bit *check_sum*:
  Byte check_sum = Bus_Serial__put_check_sum;
  check_sum = (check_sum + (check_sum >> 4)) & 15;

  // Send *count_check_sum*:
  Byte size = Bus_Serial__put_index & 0xf;
  UShort count_check_sum = (size << 4) | check_sum;

  Serial.write("P");
  Serial.print(count_check_sum, HEX);

  bus.frame_put(count_check_sum);
  UShort count_check_sum_echo = bus.frame_get();
  if (count_check_sum == count_check_sum_echo) {
    // Send the buffer:
    for (Byte index = 0; index < size; index++) {
      UShort put_frame = Bus_Serial__put_buffer[index];

      Serial.write("P");
      Serial.print(put_frame, HEX);

      bus.frame_put(put_frame);
      UShort put_frame_echo = bus.frame_get();
      if (put_frame != put_frame_echo) {
        Serial.write('&');
        error = (Logical)1;
        break;
      }
    }
  } else {
    // *count_check_sum_echo* did not match *count_check_sum*:
    error = (Logical)1;
  }

  // We mark *Bus_Serial__put_buffer* as sent:
  Bus_Serial__put_index = 0;
  Bus_Serial__put_check_sum = 0;
  return error;
}

Logical Bus_Serial__flush(void) {
  Serial.write("{");
  Logical error = (Logical)1;
  for (Byte retries = 0; error && retries < 5; retries++) {
    Serial.write("<");

    // Select the address:
    error = Bus_Serial__address_put();
    if (error) {
      Serial.write('!');
    } else {
      error = Bus_Serial__buffer_put();
      if (error) {
        Serial.write('@');
      } else {
        error = Bus_Serial__buffer_get();
      }
    }

    Serial.write(">");
  }
  Serial.write("}\r\n");
  if (error) {
    Serial.write('#');
  }
  return error;
}

void Bus_Serial__byte_put(Byte byte) {
  Bus_Serial__put_buffer[Bus_Serial__put_index++] = byte;
  Bus_Serial__put_check_sum += byte;
}

Byte Bus_Serial__byte_get(void) {
  Serial.write('e');
  Bus_Serial__flush();
  Serial.write('f');
  Byte byte = Bus_Serial__get_buffer[Bus_Serial__get_index++];
  Serial.write('g');
  return byte;
}

void Bus_Serial__command_put(UShort address, Byte command) {
  Serial.write('A');
  if (Bus_Serial__address != address && Bus_Serial__address != BUS_NO_ADDRESS) {
    Serial.write('B');
    (void)Bus_Serial__flush();
    Serial.write('C');
  }
  Serial.write('D');
  Bus_Serial__address = address;
  Serial.write('E');
  Bus_Serial__byte_put(command);
  Serial.write('F');
}

void Bus_Serial__command_byte_put(UShort address, Byte command, Byte byte) {
  Bus_Serial__command_put(address, command);
  Bus_Serial__byte_put(byte);
}

Byte Bus_Serial__command_byte_get(UShort address, Byte command) {
  Serial.write('a');
  Bus_Serial__command_put(address, command);
  Serial.write('b');
  Byte byte = Bus_Serial__byte_get();
  Serial.write('c');
  Serial.print(byte, HEX);
  return byte;
}

void loop() {
  switch (TEST) {
    case TEST_BUS_COMMAND: {
      // Blink the *LED* some:
      Serial.write("[");
      Bus_Serial__command_byte_put(ADDRESS, LED_PUT, HIGH);
      Logical led_get = Bus_Serial__command_byte_get(ADDRESS, LED_GET);
      if (led_get == 0) {
	digitalWrite(LED, LOW);
      } else {
	digitalWrite(LED, HIGH);
      }
      Serial.write("]\r\n\r\n");
      delay(100);

      Bus_Serial__command_byte_put(ADDRESS, LED_PUT, LOW);
      led_get = Bus_Serial__command_byte_get(ADDRESS, LED_GET);
      if (led_get == 0) {
	digitalWrite(LED, LOW);
      } else {
	digitalWrite(LED, HIGH);
      }
      delay(100);
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
  
      delay(100);
      if ((receive_frame & 1) == 0) {
	digitalWrite(LED, LOW);
      } else {
	digitalWrite(LED, HIGH);
      }

      delay(100);
      if ((receive_frame & 1) == 0) {
	digitalWrite(LED, HIGH);
      } else {
	digitalWrite(LED, LOW);
      }

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
      if ((frame & 1) == 0) {
	digitalWrite(LED, LOW);
      } else {
	digitalWrite(LED, HIGH);
      }

      // Delay some:
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
      if ((frame & 1) == 0) {
	digitalWrite(LED, HIGH);
      } else {
	digitalWrite(LED, LOW);
      }

      // Delay some more:
      delay(100);

      break;
    }
  }
}

