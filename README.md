I) DESCRIPTION

This is a simple program to manage an NMC9314B EEPROM memory connected to a Raspberry Pi through GPIO pins.

II) DEPENDANCE

It use the C library for Raspberry Pi (http://www.open.com.au/mikem/bcm2835/index.html)

III) DISCLAMER

This program has been made for personal use, it works for me, but I offer no waranty as this program may (unlikely) damage your raspberry pi, your memory and/or any electronic device connected. Use it at your own risk.

IV) DETAILS

The interface used is a microwire serial bus (http://www.national.com/an/AN/AN-452.pdf), functions used in this program could be re-used to interface other microwire compatible devices with the Raspberry Pi.

V) CONNECTION

RASPBERRY PI              74F126N                     NMC9314
 ______________       ________________________     ____________
|              |  /->|1C (pin 1)             |    |            |
|              | /-->|2C (pin 4)             |    |            |
|   GPIO7 (CE1)|---->|3C (pin 10)            |    |            |
|              | \-->|4C (pin 13)            |    |            |
|              |  \->|4A (pin 13) 4Y (pin 11)|--->|CS (pin 1)  |
|              |     |                       |    |            |
| GPIO11 (SCLK)|---->|1A (pin 2)   1Y (pin 3)|--->|SK (pin 2)  |
|              |     |                       |    |            |
| GPIO10 (MISO)|---->|2A (pin 5)   2Y (pin 6)|--->|DI (pin 3)  |
|              |     |                       |    |            |
|              |  /--|3Y (pin 5)   3A (pin 6)|<---|DO (pin 4)  |
|              |  |  |_______________________|    |____________|
|              | | | 
|              | |R|
|              | |1|
|              | |_|
|              |  | 
|  GPIO9 (MISO)|<-|
|______________|  |
                 | |
                 |R|
                 |2|
                 |_|
                  |
                 GND 

R1=32 kohms
R2=68 kohms

VI) INSTALL

1) install the C library for Raspberry Pi (http://www.open.com.au/mikem/bcm2835/index.html)
2) get the NMC9314_pi files at https://github,com/superzerg/NMC9314_pi
3) compile with:
gcc -o NMC9314B_bit_banged -l rt NMC9314B_bit_banged.c -l bcm2835 -O3
4) detailed usage instruction are given with: 
./NMC9314B_bit_banged -h

VII) LICENCE

Copyright (C) 1994, 1995, 1996, 1999, 2000, 2001, 2002, 2004, 2005,
2006, 2007, 2008, 2009 Free Software Foundation, Inc.

   Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.  This file is offered as-is,
without warranty of any kind.

VIII) REMARKS

The microwire protocol is bit banged as I did not succeeded to use the SPI driver to send or recived message length of fractional bytes length (for example 9 bits for a read instruction).

