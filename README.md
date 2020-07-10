# Interrupt experiments with PIC16F84A

- RB2 and RB3 are connected to LEDs which light
  on logic-high output.

- RB4 is connected to a push button switch that
  pulls the input low on button push.

The effect of a button push is to toggle the two
LEDs.

Also tracked and stored is the number of times button
has been pushed since the device was programmed. The storage is
only one byte wide and will overflow after 256 pushes.

This count may be read on with a pickit2 programmer thus:
```
./pk2cmd -ppic16f84a -ge0-4 -r -t
```
