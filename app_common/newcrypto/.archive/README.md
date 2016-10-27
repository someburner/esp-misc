# Crypto Lib

Files in this directory are for securing communication to/from the ESP8266 and
the Flume backend. NaCl was chosen for its lightweight code & superior defense
against things like timing attacks.

Go [here](http://cr.yp.to/ecdh.html) to read Dr. Bernstein's explanation of how
Curve25519 is used.

## ESP8266

**Entropy Source**:

So the ESP8266 SDK has always provided `os_random`. But a developer working
on esp-open-rtos found an apparently better source of entropy by scanning memory
regions of an idle system. Read about it [here](https://github.com/SuperHouse/esp-open-rtos/issues/3).

And [here](http://esp8266-re.foogod.com/wiki/Random_Number_Generator) is a more
detailed explanation. Apparently this location passes the NIST Statistical Test
Suite for randomness. At the end the author says that *"Starting with IoT SDK
V1.1.0, Espressif uses this register as an entropy source. Libphy function
phy_get_rand() (which is called by the general random number function
os_get_random()) XORs the value of this register with a value from RAM,
adc_rand_noise. adc_rand_noise is updated in the internal function
pm_wakeup_init, it gets set to the result of the function get_adc_rand()."*.

So it seems that register works as long as the phy module (i.e. WiFi radio) has
been initialized. Seeing as how we init WiFi on boot and never put it to sleep
this should be OK for us. I'm not 100% sure if the SDK is actually using this
register for `os_random` now, so we will define it ourselves just in case.

**Curve25519 Info**:

* [Example code for curve25519 on ESP8266](https://github.com/CSSHL/ESP8266-Arduino-cryptolibs/tree/master/curve25519-donna)
* [Another ESP8266 example](https://github.com/kaeferfreund/Curve25519_ESP8266)
* [Google curve25519 homepage](https://code.google.com/archive/p/curve25519-donna/)
* [agl curve25519-donna github](https://github.com/agl/curve25519-donna)




---
