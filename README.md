# bus_beagle_bone

A Beagle Bone Black that can be connected to a bus.

The bus is a half-duplex multi-drop bus that runs at 500 Kbps.
It uses a Microchip MCP2562 CAN bus transceivers to connect to
the bus.

## Revision A

The schematic can be found in the `rev_a` folder as
`bus_raspberry_pi.pdf`.

### Power Supply

The power comes in on either N1 or N2.  These two connectors
are "daisy-chained" so that power can be routed to other boards.
The power goes through Q1 -- a P-channel MOSFET with a very low
R<Sub>ds</Sub>.  Q1 is used to prevent reverse polarity damage
just in case the power comes in reversed.  The power continues
to N6 which is a 5V DC-to-DC power converter that provides up
to 1.5 Amperes of output.  The 5V converter takes a nominal 24V
input with an absolute maximum of 36V.

### Bus Connection

The bus connection is on N3, which is a 2x5 .1 inch pitch
shrouded header.  LGND1 and LGND2 are connected to ground.
CANH and CANL are routed over to U2 which is the CAN bus
(more correctly -- a ISO-11898) transceiver -- a Microchip
MCP2562.  The TXD and RXD outputs are routed to the RXD1 and
TXD1 pins on the ATmega324P processor (U3).

### ISP Connection

The ISP connector is the 2x3 .1 inch pitch connector (N7)
that is connected to the MOSI, MISO, #RESET, and SCK pins
on the processor (U3).

### JTAG Connection

The JTAG connector is N9 which is also a .1 inch 2x5 shrouded
header.  The TDI, TDO, DMS, TCK, and #RESET lines are connected
to the appropriate JTAG pins on the processor (U3).

### 3.3 vs 5V Volt Issues

The processor (U3), AND gates (U1), and CAN bus transceiver (U2)
all run at 5V.  The USB to serial connectors (N4, N5) and the
Raspberry Pi connector (N10) use 3.3V signal levels.  Thus,
there are numerous places where 3.3V vs. 5V signal levels have
to be dealt with.

The signal level conversion is done with a 74HCT08 quad 2-input
AND gate (U1).  The HCT logic family is capable of converting TTL
voltage levels into "high" and "low" signals.  In particular,
the HCT logic family treats any voltage above 2.0V as a logic
"high."

There are 7 separate 22K/33K voltage dividers used for voltage
level issues.  The seven 22K resistors are contained in an 8-pin
SIP (Serial In-line Package.)  These seven resistors are named
R1A through R1F.  Likewise, the seven 33K resistors are contained
in another 8-pin SIP and are named R2A through R2F.  The voltage
dividers are deliberately arranged so that they have the letter
suffix -- R1A/R2A, ... , R1F/R2F.

The nominal center tap voltage of a 22K/33K voltage divider is:

    Vin * 33K/(22K+33K) = Vin * 3/5.

With Vin equal to 5 volts, the center tap voltage is 5V * 3/5 = 3V.

These 8-pin serial in-line packages have a resistance tolerance
of 2%.  This results in a center tap voltage range from 2.88V
to 3.12V.  2.88V is greater than the 2.0 Vih<Sub>min</Sub> for
the HCT logic family.  2.88V is also greater than .7*Vcc
(=.7*3.3V=2.31V) which is the standard Vin<Sub>min</Sub> for
standard CMOS.  3.12V is less than VCC (=3.3V).  Hence, the
voltage divider does not produce illegal 3.3V signaling levels.

Whenever a 5V output needs to be converted from 5 volts to 3V,
it directly fed into a voltage divider.  This occurs with R1D/R2D
connected to TXD0 of the processor (U4) and R1E/R2E and the
output of U1C.

The remaining five voltage dividers are all inputs of the 
74HCT08 quad AND gate package.  There are three cases to
be considered:

* When there is no connection, the voltage divider will 
  pull the input pin to a nominal 3V which is treated as
  a logic high.

* a 3.3V signal is connected, it will be at either 0V or 3.3V.
  In either case, the provided 3.3V voltage signal "overrides"
  the voltage divider.

* For the USB to serial connectors (N4, N5), it is possible
  to accidentally connect in a 5V USB to serial cable.  Again,
  in this case, the provided 5V voltage signal "overrides"
  the voltage divider.  The 74HCT05 is perfectly happy to
  tolerate a 5V input signal.

### Reset Circuitry

The reset signal can be sourced from four locations:

* The manual reset push button -- SW1,

* The #NRST on the JTAG connector (N9),

* The GPIO110_SPI1_SCLK pin on N10, and

* The #RTS signal (3.3V) on the programming FTDI connector (N4),

The push button is the simplest.  It connects the #RESET line
to ground when pushed.  Done.

Similarly, the #NRST line on the JTAG connector (N9) can be
driven low by a JTAG programmer/debugger.  Done.

Both the #RTS signal from N4 and the GPIO110_SPI1_SCLK pin
from N10 are used to trigger temporary reset pulse.  The basic
circuit is C6, D1 and R4 for #RTS/N4 and C8, D1 and R4.
Since the circuit is basically the same for both branches,
only the #RTS/N4 branch is explained.

The reset circuit from #RTS/N4 adheres to the Arduino defacto
standard for reset.  The signal comes in on pin 2 of U1A.
A voltage divider R1B/R2B holds the line at a nominal 3V
if no serial cable is plugged in.  Pin 1 of U1A is tied
to 5V so that U1A operates as a simple 3.3V to 5V voltage
level converter.

When the Arduino programmer (usually AVRDUDE) opens the connection
to the serial cable, #RTS (Request to Send) is driven low
for the duration of the programming session.  Capacitor C6, D1,
and R4 are used to convert the falling edge of #RTS into a
pulse onto the #RESET line.

* Prior to asserting #RTS low, the U1A 5V output and the
  R4 10K pull-up resistor hold both ends of capacitor C6
  at 5V.  This completely discharges C6.

* When #RTS goes low at the start of programming, the U1A
  output also goes low.  Since there is no voltage across
  C6, the other end of C6 is driven low as well.  This
  causes the #RESET line to be asserted low.

* Now C6 starts getting charged via R4 and starts to develop
  5V across C6.  Eventually the voltage across C6 will get
  high enough to de-assert #RESET and the processor (U3)
  will start to run the boot-loader.o

* When the programming session is over, #RTS is driven high.
  The causes the output of C6 to attempt to jump from 5V
  up to 10V.

<Pre>
        10V |                           *
            |                           **
            |                           * ***
            |                           *    ****
            |                           *        *****
        5V  ********              *******             *******
            |      *        ******      ^
            |      *    ****            |
            |      * ***                |
            |      **                   |
        0V  +------*--------------------|----------------------->
                                        |
                   ^                    |
                   |                    |
               #RTS goes low       #RTS goes high
</Pre>

* To prevent the voltage from going up to 10V, the Schottky
  diode D1 clamps the voltage to no more than 5.2V by very
  rapidly discharging C6.

The reason why a Schottky diode is used instead of a silicon
diode is because it unclear what the voltage tolerance of #NRST
of the JTAG programmer connector (N9) and the #RESET connector
of the ISP connector (N7) are.  By using a Schottky diode,
the voltage is clamped to 5.2V rather than 5.7.

### RXD and TXD Considerations

In general, the primary purpose of this board is to connect
TXD0 and RXD1 to GPIO2_UART2_RX and GPIO3_UART2_TX respectively.
In addition, it is desirable to allow the programming
connector (N5) to be used to program the processor (U4)
*and* to use the console connector (N5) to view the console
traffic from the Raspberry Pi on boot up.  To accomplish this
task some additional circuitry is used to OR the serial
cable TXD signals into the correct locations.

A key thing to understand about serial connections is that
the line idles "high".  U1B and U1D are AND gates.  Using
De Morgan's law, A.B = ~(~A + ~B).  Thus, using 2-input AND
gates, the signals a correctly OR'ed together so that when
both inputs are idling "high", the output is also "high".

There are three paths to follow:

* Processor (U4) <==> Raspberry Pi (N9),

* Processor (U4) <==> programmer connector (N2), and

* Processor (U4) <==> console connector (N3).

#### Processor to Raspberry Pi

In this situation, the 5V TXD0 output on the processor (U3)
is routed through the R1D/2D voltage divider to bring the
voltage down to a nominal 3.0V.  This signal is routed to U1C.
U1C converts the 3.0V signal to a 5V output which is again
divided back down to 3.0V via R1E/R2E.  This 3.0V signal goes
into GPIO2_UART2_RX on the Raspberry Pi.

In the reverse direction, the 3.3V output of GPIO15_TXD is
sent through the R1G/R2G voltage divider U1B which converts
it to a 5V output and directs it to RXD0 on the processor (U3).

#### Processor to Programmer Connector

In this situation, the 5V TXD0 output on the processor (U3)
is voltage divided by R1D/R2D down to 3.0V and goes to RXD
on the programmer connector (N2).

The 3.3V TXD output on the programmer connector (N2) is
routed to U1B which converts it up to 5V and routes the
signal into RXD0 input on the processor (U4).

#### Processor to Console Connector

In this situation, the 5V TXD0 output on processor (U4)
is voltage divided R1D/R2D down to 3.0V and to both U1C
and the RXD signal of N4.

The GPIO3_UART2_TX is routed through the R1G/R2G voltage
divider directly to the RXD pin of N5.

## LED Circuit

The LED circuit is provides a single LED (D2) that can be

* connected to the power supply to provide a power on LED, or

* connected to PB7/SCK (aka the Arduino D13 pin) of the
   processor (U3), or

* can be left unconnected.

This is accomplished using J1, R5, R6 and D2.

When pins 1 and 3 of J2 are shorted using a jumper, power is
routed from +5V through R6 through the LED (D2) to ground.

When pins 2 and 3 of J2 are shorted together, the output
of PB7/SCK of the processor (U3) is routed through R5 
through the LED to ground.

When pins 3 and 4 of J2 are shorted together, there is
no circuit path through LED (D2).

When pins 1 and 2 of J2 are accidentally shorted together,
resistors R5 and R6 form a voltage divider that simply
wastes power.  At least it does not accidentally short
the power supply to ground.

## Rev. A Issues

* JTAG NRST is not connected to #RESET. [x]

* R1D is connected to +5V not TXD0. [x]

* R1E is connected to +5V not U1C output. [x]

* R1G is connected to +5V not GPIO2_UART2_RX. [x]

* C6 and C8 should be 1uF. [x]

* The MCP2562 (U2) RxD and Txd (pins 1 and 4) are swapped. [x]

* Add 100K pull up resistor inputs that can be disconnected. [x]

* Make sure the BBB connector edge is clear of obsticles.
  Think about making N10 (the BBB connector) the full length. [x]

* Think about connecting MCP2562 (U2) STBY (pin 8) to a processor pin.

* Rotate N3 (the bus connector); it is pointing the wrong way. [x]

* C1 can be moved to be in line with C2. [x]



* N4 is labeled "CONSOLE" and N5 is labeled "PROGRAM".  This is backwards. [x]

* The artwork for N6 (the power supply connector) is backwards. [x]


## Rev. B Issues

* Move N10 closer to the PCB edge. [x]

* Think about add two pins next to N10 to prevent miss aligning
  N10 along the P9 row.  [Added N11, N12, and N13]

* Added real time clock. [x]

## Rev. C Issues

* Pin 10 of U1 has no 100K pull-up. [done]

* ISP connector is too close to real time clock to work with the
  Atmel-ICE programmer.  There is actually plenty of room around
  the crystal and load capacitors. [done]

* The hole for the plus terminal of the Litium battery is way
  too big.

* The 5mm Terminal blocks are oriented backwards. [removed]

* Think about changing the 1x1 mis-alignment pins for the
  BBB connector to 1x2's.  They would be easier to install. [done]

* Think about removing the R9/R10 pull-up resistors.  The BBB
  already has pull-up resistors. [done]

* The Lithium battery overlaps with the JTAG connector.  Perhaps
  rearrange. [done]

* There is no filter capacitor for the real time clock.

* The RC constant of R4 * (C6 + C8) needs to be small enough
  that the circuit will actually reset before jumping to location 0.
  When both C6 and C8 are 1uF, the RC constant is too big.
  Thus, probably C6 and C8 should be set to .1uF. [done]

* N7 is too close to N6.  It is really hard to jumper the LED
  to be connected to D13. [5V Regulator changed]

* N6 is artwork is not next to actual N6 connector. [done]

* N6 would have mechanical interferance with N2. [done]

* Think about adding lines to the BBB POWER button and reset.
  [not done]

* The Ground on pins 1 and 2 of the BBB connector is not very good.
  [done]

* Pin 31 is hooked to a high frequency SPI signal.  It should be
  connected to a slower GPIO line. [done]

* Think about adding a female USB A connector to provide 5V to
  other devices that need 5V. [see bus_usb_power]

## ROS Integration

It is desirable to have excellent ROS integration with "the bus".
The bus has the following features:

* Discovery.  The head processor can scan the bus and find all
  modules that are plugged into it.  Currently, hot plugging
  is not allowed, but it could if future versions.

* Flexibility.  While bus modules can be dedicated to a single
  task, some of them are more fleximble.  For example,

  * bus_grove12 can be connected to 100+ different Grove system modules.

  * bus_micro?? can be connected to 100+ diffeent MikroBus modules.

  * bus_dynabus can be connected to dozens of Dynamixel servos.

* Field Upgradable. The firmware for each module can be upgraded
  of the bus.

The bus is basically operated in a master/slave mode.  ROS talks
to a bus master (e.g. bus_beaglebone) via a serial protocol.
The bus master talks to the bus modules using bus protocol.

The ROS bus node is a ROS node that talks to the bus master
over a serial line.  Only the ROS bus node is allowed to use
the serial line connected to the bus master.  Any other ROS
nodes that need to access the bus must do so through the ROS
Bus node.

It is anticipated that each bus module will have a corresponding
ROS node that is responsible for the bus module.  

correspondance to bus modules and a ROS node to control the
bus module.  For example, the bus_sonar10 module will have
a didicated ROS Sonar10 node.  

### ROS Bus Node

The basic concept behind the ROS Bus Node is that allows other
ROS nodes to talk to the bus via a modes number of ROS service
(i.e. Remote Procedure Call) requests.

        login("node id TBD") => session
            Returns a session handle for subsequent requests.

        logout(session)
            Lets Bus Node that session is not to be accepted
            anymore.

        bus_search(session, vendor_id, product_id) => [bus_uid,...]
            Return list of bus_uid's for bus modules that match
            vendor_id and product_id.  vendor_id and product_id
            can be set to 0 to matach all.

        hardware_version(session, bus_uid) => hardware_version
            Return the hardware version for bus_uid.

        firmware_version(session, bus_uid) => (major, minor)
            Return the major and minor software version numbers
            for bus_uid.

        attach(session, bus_uid, share) => module
            Returns a module handle for talking to the bus_id.
            Set share to 1 for a sharable module and to 0 for
            exclusive use.

        release(session, module)
            Releases module.

        transfer(session, module, priority, [byte, ...]) => [byte, ...]
            This is the generic bus message send receive message.
            Mostly, people will use higher level commands.  Priority
            specifies the immediacy of the message.

        register_get(session, module, priority, register) => value
            Set register to value.  Value is is a scalar value
            that is up to 32-bits.

        register_set(session, module, priority, register, value, size)
            Set register to value.  Value is is a scalar value
            that is up to 32-bits in size.  size specifies the number
            bytes.

        string_get(session, module, priority, register) => "text"
            Get a string value from a string register.

## Bus_Bridge_Encoders_Sonar Node

Basically this node has to emulate the functionality of the
ROS Arduino Bridge.

* Publish odometry data

* Accept drive requests (i.e. speed and twist)

* Publish sonar data

## Serial Line Protocol

        Host => Coprocessor:

            1100 00hh        Set hh
            1100 0100        Do Discovery scan
            1100 0101        Reset bus
            1100 0110        Reserved
            1100 0111        Reserved

            xyyy yyyy        if (hh > 0) { send (h hyyy yyyy); hh = -1; }
                                     else      { send (0 xyyy yyyy); }

        Coprocessor => Host:
            a xxxx xxxx	     Send xxxx xxxx to host (9th bit is lost)

## Installing ROS Hydro Onto a Micro-SD Card

1. Create a ros-bbb directory:

        cd ...
        mkdir ros-bbb
        cd ros-bbb

2. Download the following three files from ros-training.com:

        wget http://www.ros-training.com/static/media/ros-bbb/ubuntu_ros_partitions.sfdisk
        wget http://www.ros-training.com/static/media/ros-bbb/ubuntu_ros_boot.partimg.gz.000
        wget http://www.ros-training.com/static/media/ros-bbb/ubuntu_ros_rootfs.partimg.gz.000

3. Verify that you got files that are the right size:

        ls -la
        -rw-rw-r-- 1 wayne wayne   6916451 Aug 22 08:01 ubuntu_ros_boot.partimg.gz.000
        -rw-rw-r-- 1 wayne wayne       259 Aug 22 08:01 ubuntu_ros_partitions.sfdisk
        -rw-rw-r-- 1 wayne wayne 345502689 Aug 22 08:09 ubuntu_ros_rootfs.partimg.gz.000

4. *Before* put your micro-SD card in, see what /dev/sd* says:

        ls -l /dev/sd*
        brw-rw---- 1 root disk 8,  0 Aug 22 10:33 /dev/sda
        brw-rw---- 1 root disk 8,  1 Aug 22 10:33 /dev/sda1
        brw-rw---- 1 root disk 8,  2 Aug 22 10:33 /dev/sda2
        brw-rw---- 1 root disk 8,  3 Aug 22 10:33 /dev/sda3
        brw-rw---- 1 root disk 8,  4 Aug 22 10:33 /dev/sda4
        brw-rw---- 1 root disk 8,  5 Aug 22 10:33 /dev/sda5
        brw-rw---- 1 root disk 8,  6 Aug 22 10:33 /dev/sda6
        brw-rw---- 1 root disk 8,  7 Aug 22 10:33 /dev/sda7

5. Now insert your 8GB (or larger) micro-SD card into the slot and
   do another ls command:

        ls -l /dev/sd*
        brw-rw---- 1 root disk 8,  0 Aug 22 10:33 /dev/sda
        brw-rw---- 1 root disk 8,  1 Aug 22 10:33 /dev/sda1
        brw-rw---- 1 root disk 8,  2 Aug 22 10:33 /dev/sda2
        brw-rw---- 1 root disk 8,  3 Aug 22 10:33 /dev/sda3
        brw-rw---- 1 root disk 8,  4 Aug 22 10:33 /dev/sda4
        brw-rw---- 1 root disk 8,  5 Aug 22 10:33 /dev/sda5
        brw-rw---- 1 root disk 8,  6 Aug 22 10:33 /dev/sda6
        brw-rw---- 1 root disk 8,  7 Aug 22 10:33 /dev/sda7
        brw-rw---- 1 root disk 8, 16 Aug 22 18:50 /dev/sdb
        brw-rw---- 1 root disk 8, 17 Aug 22 18:50 /dev/sdb1

   Notice that /dev/sdb and /dev/sdb1 showed up.  If something
   else shows up, you are going to need to manually edit
   ubuntu_ros_partitions.sfdisk file.  In addition, everywhere in
   the instructions below, you are going to have to change
   "/dev/sdb..." into /dev/sdX..." where "X" is the correct
   micro-SD drive letter.  If this does not make any sense,
   *get some help*.

6. I do the next commands as root:

        sudo bash
        Password: {your root password here}

7. Make sure that the partimage command is installed:

        apt-get install partimage
        {Respond positively to all prompts}

8. Make darn sure that /dev/sdb* is not installed:

        umount /dev/sdb*
        {It should say everything is unmounted}

9. Install a partition table on /dev/sdb:

        sfdisk --force /dev/sdb < ubuntu_ros_partitions.sfdisk
        partprobe /dev/sdb
        partprobe -s /dev/sdb

   The partprobe -s command may mention "msdos", just ignore that.

10. Now install boot image on micro-SD card:

        partimage restore /dev/sdb1 ubuntu_ros_boot.partimg.gz.000

    This will bring up one of the fake ASCII window system.
    You can use [Tab] to advance from one option to the next
    You can use [space] to toggle an option.
    You can type [enter] to trigger everything.
    For this screen I only toggle the option that writes 0's
    to empty blocks.  It is probably unnecessary, but I like
    to be sure.  Type [enter] at the continue option.  Keep
    typing [Tab]'s and [returns] until it actually write the
    partition bits.  The program will finish and then pause
    for a while as it finishes flushing the bits out to the
    micro-SD card.  Just be patient.

11. Now install rootfs partition onto the micro-SD card:

        partimage restore /dev/sdb2 ubuntu_ros_rootfs.partimg.gz.000
                
    Go through the same steps as in step 10.

12. Make darn sure that /dev/sdb* is unmounted:

        umount /dev/sdb*
        {It should say everything is no longer mounted}

14. Make sure that minicom is installed:

        apt-get install minicom

14. Exit from root:

        exit

15. Yank the micro-SD card out of your machine.

16. Grab your Beaglebone Black (henecforth called BBB.)  Flip it
    over and install the micro-SD card.

17. Find two open USB ports on your laptop/desktop.  Plug your
    FTDI 3.3V USB to serial cable into one of the USB ports.
    Make *darn* sure that you have a 3.3V FTDI cable.  If you
    are using one of Wayne's FTDI cables with tape wrapped
    arround the pin, peel the tape back and make *sure* it
    says "3V3" on the USB connector.  Plug the short USB to
    mini-USB cable that came with your BBB into the other USB
    port on laptop/desktop.  Do *not* plug the other end of
    this cable into the BBB yet.

18. On the top of the BBB, there is a 1x6 pin header labeled J1.
    One of the pins has a white dot next to the pin.  Plug in
    the 1x6 female receptical on the FTDI cable such that the
    black wire goes into the next the white dot.

19. Fire up the minicom terminal emulator:

        minicom

    Make sure the baud rate is "115200 8N1".  It should specify
    that it is using a serail cable like /dev/ttyUSB0.

20. Locate the little black button on the top surface of the BBB
    on that is near the micro-SD card (which is on the back surface).
    This button is very close to one of the mounting holes.  Also
    locate the micro-USB connector on the back side of the BBB
    near the Ethernet Jack (which is on the front side.)

21. Now we will carefully power up the BBB.  This is done by:

    A. Press down the black button and *hold* it down.
    B. While leaving the black button held down, plug in
       the power micro-SD.
    C. The blue LED's flash on the BBB.
    D. Boot up text spews out on the minicom window.

22. In fairly short order, Linux 13.04 will boot up and prompt
    you to login.  You can login as either user "ubuntu" with
    password "temppwd" or as user "ros" with password "training".

23. After you have logged in, you can poke around.  Verity that
    the directory "/opt/ros/hydro" exists.

24. Now you can shut down the BBB:

        sudo halt
        Password: {Use "temppwd" or "training")

    Some lights flash and some text spews.  Eventually the BBB
    stops.

25. Remove power cable and USB-to Serial cable.

