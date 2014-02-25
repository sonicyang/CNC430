CNC430
======
Miniature CNC Build using 2 DVD drive and a floppy drive

Controller is MSP430, code is written with energia.
Library used : Standard Stepper motor Library.
Hardware: 3 L293D Quad Half H-Bridge, operating under 5V.

Computer side G-code sender is written in python, just a simple code reading file and send out via UART.

Please upload /MSP430/CNC430.ino to your lanuchpad, and start /python/main.py to Enter Shell Program.

There is a mul define in CNC430.ino, change its value according your scaling.