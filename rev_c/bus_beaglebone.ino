// Copyright (c) 2014 by Wayne C. Gramlich.  All rights reserved.
//
// This code drives the bus_beagle_bone_black board.

#define UART1_DISABLED

#define BUS_STANDBY       3
#define BUS_BAUD_500KBPS  3 // Fosc=16MHz & U2X1=1
#define LED		 13

// typedef's go here:
typedef unsigned char Bool8;
typedef unsigned char Byte;
typedef unsigned short Frame;
typedef struct Frame_Buffer__Struct *Frame_Buffer;
typedef unsigned short UInt16;

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

Bool8 Bus_Serial__can_receive(void) {
    Bool8 can_receive = (Bool8)0;
    if ((UCSR1A & (1<<RXC1)) != 0) {
	can_receive = (Bool8)1;
    }
    return can_receive;
}

Bool8 Bus_Serial__can_transmit(void) {
    Bool8 can_transmit = (Bool8)0;
    if ((UCSR1A & (1<<UDRE1)) != 0) {
	can_transmit = (Bool8)1;
    }
    return can_transmit;
}

Frame Bus_Serial__frame_get(void) {
    // Wait for a frame (9-bits) to show up in the receive buffer:
    while (!((UCSR1A & (1<<RXC1)) != 0)) {
	// Do nothing:
    }

    // Deal with address bit:
    Frame frame = 0;
    if ((UCSR1B & (1<<RXB81)) != 0) {
	// Set the address bit:
	frame = 0x100;
    }

    // Read in the rest of the *frame*:
    frame |= (Frame)UDR1;
    return frame;
}

void Bus_Serial__frame_put(Frame frame) {
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
  Bus_Serial__initialize(BUS_BAUD_500KBPS);

  // Turn the *LED* on:
  pinMode(LED, OUTPUT);
  digitalWrite(LED, HIGH);

  // Force the standby pin on the CAN transeciever to low to force it
  // into active mode:
  pinMode(BUS_STANDBY, OUTPUT);
  digitalWrite(BUS_STANDBY, LOW);
}

// The loop routine runs over and over again forever:
Frame send_frame = (Frame)'@';
void loop() {
  // Output *frame*:
  Bus_Serial__frame_put(send_frame);
  Frame echo_frame = Bus_Serial__frame_get();
  if (echo_frame != send_frame) {
    Serial.write('@');
  }
  Frame receive_frame = Bus_Serial__frame_get();

  if (send_frame == receive_frame) {
    Serial.write(receive_frame);
  } else {
    Serial.write('!');
  }

  if (send_frame == '_') {
    Serial.write('\r');
    Serial.write('\n');
    send_frame = (Frame)'@';
  } else {
    send_frame += (Frame)1;
  }

  // Blink the *LED* some:
  delay(100);
  digitalWrite(LED, LOW);
  delay(100);
  digitalWrite(LED, HIGH);

}

