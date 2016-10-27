# ESP-MISC

Repo for my miscellaneous projects, drivers, experiments for the ESP8266.

## Features
-----------

Subject to change but so far:

 * Demonstrates 2 roms on a single MB of flash with rBoot. See
   "Project Structure" section below.
 * Demonstrates using HSPI overlap mode to read a UUID from SPI Flash. This is
   lasered in to *some* SPI Flash chips, so you have to see if your ESPs flash
   chip supports the UUID command.
 * WS2812 driver using HSPI, see comments in `app_setup/driver/ws2812.c`
 * Non-blocking Onewire driver, so far working with the Maxim DS18B20/MAX31820.
   Uses the FRC1 hardware timer to step through a state machine, and thus does
   not tie up the processor with delays (as it does in the popular DS18B20
   Arduino driver).
 * Demonstrates Public Key crypto using NaCl. Code taken from NaCl for AVR.

Future possibilities:

 * Extend WS2812 driver into a library with more effects
 * Use hw_timer to drive the WS2812 in a similar fashion to the Onewire driver.
   This would allow us to drive the DS18B20 without using
   ets_intr_lock()/ets_intr_unlock(), and would also prevent it from messing up
   the Onewire state machine.
 * Icing on the cake would be using SPI interrupts to remove loops from the
   WS2812 driver.

## Serial Monitor
-----------------

rBoot bootloader baud is 74880. In user_init baud is switched to the user-defined
rate. If platformIO is installed, serial monitor can be invoked as such:

```
platformio serialports monitor -b 74880 -p /dev/ftdi_esp # boot message
platformio serialports monitor -b 921600 -p /dev/ftdi_esp # run
```

## Configuring
--------------

**NOTE**: Onewire, WS2812, Crypto stuff currently on setup0/setup1. Copy needed
code, driver files into app_main if you want those on app_main.

**Common**:

 * `app_common/include/hardware_defs.h`:
   - `FLASH_UNIQUE_ID_ENABLE`: Comment/Uncomment to attempt to read the SPI
   Flash chip's UUID. **NOTE**: Only works on certain flash chips that have a
   UUID. If you're unsure, find the datasheet for your ESP's SPI Flash chip and
   look for a UUID command.
 * `app_common/include/server_config.h`:
   - `NTP_SERVER_0/1`: Set server to use for SNTP.
   - `BROKER_SET`: Set this to `PUBLIC_BROKER` to use eclipse.org, or
   `MY_BROKER` to define your own.
   - `INC_TEST_TOPIC_SET`: Include a test topic to pub/sub to

**Setup**:

 * `app_setup/include/user_config.h`:
   - `EN_TEMP_SENSOR`: Comment/Uncomment to initialize Onewire non-blocking
     driver. Uses timer to do a conversion every few seconds and publish on the
     MQTT test topic.
   - `EN_WS8212_HPSI`: Comment/Uncomment to initialize WS8212 HSPI driver and
     arm a timer to drive with a multi-color fade-in/fade-out effect.
   - `TEST_CRYPTO_SIGN_OPEN`: Comment/Uncomment to test opening a signed
      crypto_box message.
   - `TEST_CRYPTO_PK_ENCRYPT`: Comment/Uncomment to test encrpyting a message
      using asymmetric public key auth.

## Usage
--------

### Edit Makefile(s)

 * Update `OPENSDK_ROOT` in **Makefile** with the absolute path to esp-open-sdk
 * Update `OPENSDK_ROOT` in **rboot/Makefile** as well
 * *Optional*: Set `STA_SSID`, and `STA_PASS`, `STA_PASS_EN` to your WiFi
   network's settings if you would like the chip to automatically connect to a
   network via DHCP. `STA_PASS_EN` can be set to 0 if no password is needed.

### Compile

```
make clean
make
```

**NOTE**: To compile the setup1 rom, go into `app_setup/Makefile` and change
`APP_TARGET = 0` to `APP_TARGET = 1`. App main is the same for both slots. See
"Project Structure" section for details.

### Flashing

To simplify flashing, I've created a simple utility for generating the rBoot
config sector. So once you have compiled everything, set `ROM_TO_BOOT` in
**Makefile** with the rom you want to boot. Also make sure that `FLASH_RCONF`,
`FLASH_RBOOT` are set to "yes", as well as `FLASH_ROMX` where X is the rom,
0-3 you want to flash. Generally you want to flash the same rom as the one you
want to boot, but you can flash whatever you want.

Then:

```
make rconf
make flash
```


## Pin Configuration
--------------------

### ESP8266 Required

**Bold** == for flashing only

|    Signal    | ESP-12E  |
|:------------:|:--------:|
| 10K PULLUP   | CH_PD/EN |
| 10K PULLDOWN | GPIO15   |
| 10K PULLUP   | GPIO0    |
| **GND**      | GPIO0    |
| BUTTON-GND   | RST      |

### WS2812

|  WS2812  |  ESP-12E  |
|:--------:|:---------:|
|   +5V    |    N/C    |
|   GND    |    GND    |
|  Data    | GPIO13 (MOSI) |

### 1-wire/DS18B20

| DS18B20 |   ESP-12E   |
|:-------:|:-----------:|
| DIO     | GPIO4       |
| DIO     | 4.7K PULLUP |
| GND     | GND         |
| VDD     | GND **see note** |

**NOTE**: If using parasitic power for onewire, tie DS18B20 VDD to GND. Other-
wise, route to +3.3V rail. As it stands, parasitic power is not supported. To
do so you'll have to edit the Onewire driver to use different delays/sequences.

**NOTE2**: Currently only supports a single Onewire device on the bus. To add
more devices, you'll need to edit the delays a bit and add another sequence to
send the UUID before sending the command. See NOTE in
`app_setup/include/driver/maxim28.h` for info on how you can do this.


## Project Structure
--------------------

* Built/Tested with ESP-12E units
* Compiled with esp-open-sdk and Espressif SDK 1.5.4.1
* rBoot 4+ ROM configuration

## Setup vs. Main Programs

This repo contains three folders, app_common, app_main, and app_setup. Please
read the explanations below.

### app_common

...contains code that can be used by both roms, i.e. code that
should execute the same way regardless of which rom is using it. Note that this
code will still be compiled into both roms (if both roms #include it). But this
way we only need to compile the common code once. More importantly, changes made
in app_common will affect both roms the same way.

### app_setup and app_main

These folders contain code that will be compiled into setup0/setup1.bin and
main.bin. Right now they effectivately produce the same code (minus rBoot
switching code), however they can contain totally different code if desired.

**app_setup** uses the `misc.app.setup0.ld` and `misc.app.setup1.ld` ld files
in order to correctly map code to the correct places in memory. This is needed
because these roms are flashed to the first and second 512KBytes of SPI Flash.

**app_main** uses a single ld file, `misc.app.main.ld`. Only one is needed since
this rom spans (almost) an entire MB of flash.

### Why?

For most users, a single program (and space to OTA that program) is probably
enough. But there are some situations where it could be beneficial to have
smaller roms, each with different functionality. For example, you might have
(as the folder names imply), a "setup" rom, which contains logic to connect to
WiFi, and grab initial config information from a server. Once that is done, the
ESP can switch to the "main" app which has all the "normal use" code.

There are both advantages and disadvantages to this approach. These are the main
ones as I see it:

**Pros**:
 * Better/cleaner code organization. By taking care of one-time/rarely used
   procedures in the setup app, the main app can assume things like WiFi are
   already in place. This greatly reduces the complexity of keeping track of
   what state the device is in. Another place this comes in handy is developing:
   if you want to try out some new code really quick you can write it into the
   setup rom without having to worry about breaking your main app.
 * Performance/efficiency. In this arrangement you can fine tune each app for
   their respective purposes. For example, you could put all OTA logic only in
   the setup app. If you are using MQTT for this, that means your main app no
   no longer has to subscribe to special topics for OTA or check responses for
   OTA stuff.
 * Flexibility. Perhaps you want to connect to one server for OTAs and another
   for normal use stuff. Having two separate roms for that saves you from
   worrying about properly disconnecting and re-initializing MQTT.
 * More spaces for SPIFFS. If your program is only ~512K, you could just use
   the setup rom space for your program, and have 3MB for SPIFFS while still
   being able to do an OTA.

**Cons**:
 * Adds to compile time, obviously.
 * Adds complexity in a few spots. One being linker scripts/Makefiles, but this
   shouldn't be a big deal if you can follow what I've done in this repo.
   Another spot is switching between roms, but this isn't too bad IMO. The big
   one is probably OTA logic on your backend since you need to be able to
   send the correct rom and make sure the ESP writes it to the correct slot.

## GCC Warn Options
-------------------
```
-Wunused-function
-Wunused-label
-Wunused-value
-Wunused-variable
```

## Module List
--------------

### non-OS:

* [rBoot](https://github.com/raburton/rboot)
* [NodeMCU](https://github.com/nodemcu/nodemcu-firmware/tree/dev)
* [SPI Driver](https://github.com/MetalPhreak/ESP8266_SPI_Driver)
* [SPIFFS](https://github.com/pellepl/spiffs)
* [MQTT](https://github.com/tuanpmt/esp_mqtt)
* [jsmn](https://github.com/zserge/jsmn)
* [JSONTREE](https://github.com/contiki-os/contiki/tree/master/apps/json)

### OS:

* [Espressif Doc Portal](http://www.espressif.com/support/download/documents)
* [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)
* [esp-open-lwip](https://github.com/pfalcon/esp-open-lwip)
* [Espressif SDK](http://bbs.espressif.com/viewforum.php?f=46)
* [ESP-Touch/SmartConfig (PDF)](http://bbs.espressif.com/download/file.php?id=1328)

### Other Tidbits
-----------------

**Calculate total lines of code**:
```
( find . \( -name '*.c' -o -name '*.h' -o -name '*akefile' -o -name '*.ld' \
-o -name '*.ld' -o -name '*.py' \) -print0 | xargs -0 cat ) | wc -l
```

## License
----------

See LICENSE
