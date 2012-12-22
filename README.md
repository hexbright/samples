HexBright Code (jaebird)
=======================

Wiki of mods here: https://github.com/jaebird/HexBrightFLEX/wiki

hexbright_extra
-----------------
This is similar to the software that ships with the Hexbright FLEX.  Button presses cycle
through off, low, medium, and high modes.  Long Press while off for extended modes (dazzle, SOS).

Button Presses:
* Normal Press is less than 0.5 Sec
* Long Press is more than 0.5 Sec

Extra:
* Added quick power off in all modes. Use Long Press to turn off flashlight.
* Changed factory blinky mode to dazzle mode
* Added SOS. To get there: Long Press for dazzle, then Normal Press for SOS. The only way to turn of SOS is long press
* Inactivity shutdown after 10 min unless in SOS mode.

NOTE: Some features cherry picked from Brandon Himoff in loose collaboration (https://github.com/bhimoff/HexBrightFLEX)

hexbright_factory
-----------------
This is the software that ships with the Hexbright Flex.  Button presses cycle
through off, low, medium, and high modes.  Hold down the button while off for 
blinky mode.

hexbright4
---------------------
Fancier than the factory program, but designed for everyday usability.  Button
presses cycle through off, low and high modes.  Hold the light horizontally,
hold the button down, and rotate about the long axis clockwise to increase
brightness, and counter-clockwise to decrease brightness- the brightness sticks
when you let go of the button.  While holding the button down, give the light a
firm tap to change to blinky mode, and another to change to dazzle mode.

hexbright_demo_morse
--------------------
Flashes out a message in morse code every time you press the button.  Nothing 
else.  The message and speed are easy to change- you can see and change both 
in the first lines of code.

hexbright_demo_taps
-------------------
Hold the button down, and with your other hand firmly tap on the light.  Tap
some more times, and let go of the button.  The exact sequence of taps will
be recorded and then played back as flashes until you press the button again
to turn off.

hexbright_demo_momentary
------------------------  
Light turns on only while the button is being held down.  That's it.

hexbright_demo_dazzle
---------------------
Light runs in dazzle mode only as long as the button is being held down.

hexbright_demo_fades
--------------------  
Hold the button down, and light fades up and down.  Let go, and it holds the 
current brightness.  Another press to turn off.
