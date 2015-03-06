// Copyright (c) 2014 by Wayne C. Gramlich.  All rights reserved.
//
// This code drives the bus_beagle_bone_black board.

#include "Bus_Slave.h"
#include "Frame_Buffer.h"
#include "bus_server.h"

#define TEST TEST_BUS_LINE

void setup() {
  bridge_setup(TEST);
}

void loop() {
  bridge_loop(TEST);
}
