# TUNER_INTERFACE2

Arduino code for interfacing of LDG Z-11 ProII tuner with ICOM IC-7300.  

The interface adds advanced functionality allowing the on-radio (and remote software) ICOM tuner buttons to work the LDG tuner.
Modes can be -> Tune, Tuner Enable, Tuner Bypass.

The interface/code can also detect if the tuner is Enabled or Bypassed and update the ICOM radio with the current mode.  
(An additional wire must be attached to the circuit board for this detection to work - but is fairly easy to do.)

TODO:  Upload pinout for wiring Tuner and Radio to Arduino
