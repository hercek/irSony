# irSony
Receiver for Sony infrared remote and HID rudder pedals on Arduino Leonardo.

## Files and credist
* irSony.hs - Simple haskell script expected to run on a linux PC. It is receiving
  signals from a Sony IR remote and controling mplayer based on the received data.
  You probably do not need this or you want to modify it for your remote.
  Just cat /dev/ttyACM? to see what your remote sends.
* irSony.ino - Arduino sketch taking decoded data from IRSony and sending them
  to a serial line. It also handles rudder pedals and exposes them as a HID
  Joystick.
* IRSony.* - Implementation of Sony 12 IR protocol decoder
  It probably can be ported to other Sony protocols easily.
* PinChangeInterrupt*.* - PinChangeInterrupt from Andreas Rohner and NicoHood
  https://github.com/zeitgeist87/PinChangeInterruptHandler.git
  It did not seem to be packed as a proper Arduino library so I just copied
  the files. And I do not want to deal with the potential disapearance
  of PinChangedInterruptHandler repository anyway. It's simple enough.
* Joystick.* - Implementation of Joystick object for rudder pedals.

## Notes
Sorry, but infrared decoder and rudder pedals features are merged in one sketch.
I need it that way for me. Anyway, it is very easy to pull out the Sony infrared
receiver from the code. Take all files named IRSony* and PinChangeInterrupt* and
you are done. You can consult irSony.ino for example how to use IRSony decoder.

There is a pretty extensive debugging feature in the IRSony decoder. You can switch
it on by defining symbol IRSonyTrace in IRSony.h file. You can print a precise
timing of an infrared signal comming to the receiver. It can be used to analyze
other IR protocols. Consult irSony.ino for a simple use case.

This was written because Ken Shirriff's IRremote library did not decode Sony
protocol reliably and NicoHood's IRLremote library did not support Sony at all.
And I was lazy trying to undertand / fix / extend those two libraries. That was
the situation at the time of writing (April 2019).
