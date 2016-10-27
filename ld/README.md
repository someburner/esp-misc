## Linker Info

In order maximize our utility of the way the SDK breaks up linked sections on
flash, we use rBoot to load custom ROM locations.

## Two Apps

**Setup App**:
The setup app (misc.app.setup) is 512KB in size, comprising of 2 slots both
contained in the 1st MB of flash. So the first setup rom (rom0) is at the first
available slot and the second (rom1) is at flashsize/8 (0x80000).

**Main App**:
The main app (misc.app.main) is 1MB in size, mapped to the start of the 2nd
MB on flash or the 3rd MB in flash (depending on which rom is currently in use).
So the first main rom (rom3) is at 0x100000 and the second main rom (rom4) is
at 0x200000.

**SPIFFS**:
Given the above locations, this means all SPIFFS filesystems are located
between 0x300000 and 0x400000.

**Memory Map**:

## Memory Map

**NOTE**: The reason for the "mimic" sectors is to ensure that if a ROMs size is
slightly curtailed by something like rBoot or Espressif's SDK param area, that
*both* ROMs are the same size. As long as the total size of either is less than
the max of the curtailed size, its no problem. So this is more of a failsafe if
a ROM is pushing the size limit and/or to make sure we never try to access those
areas.

Also note that all SPIFFS images are in the last MB. Since SPIFFS is more
forgiving and easier to change the size of, it makes sense to put it in the same
MB that contains the SDK config so we don't have to deal with any of the "mimic"
stuff above that would be an issue if we had a program in that MB. It also just
seems like a more logical place for it.

| Start Address | End Address | Size  | Description  |
|:-------------:|:-----------:|:-----:|:------------:|
|  `0x000000`   |  `0x002000` |   8K  | Bootloader (rBoot) |
|  `0x002000`   |  `0x080000` | 504K  | rom0 (setup0) |
|  `0x080000`   |  `0x082000` |   8K  | Mimic rBoot space |
|  `0x082000`   |  `0x100000` | 504K  | rom1 (setup1) |
| -----End----- | ---Setup--- | <=1MB | --Sections--- |
|  `0x102000`   |  `0x200000` |  1MB  | rom2 (main0)  |
|  `0x202000`   |  `0x300000` |  1MB  | rom3 (main1)  |
| -----End----- | ----Main--- | <=3MB | --Sections--- |
|  `0x300000`   |  `0x310000` |  65K  |   spiffs0    |
|  `0x310000`   |  `0x360000` | 320K  |   spiffs1    |
|  `0x360000`   |  `0x3fc000` | 624K  | Currently Unused |
|  `0x3fc000`   |  `0x400000` | 16KB  | SDK config area |

#### Semi-Static FS (aka spiffs0 aka statfs)
 * Start Addr: `0x300000`
 * End Addr: `0x310000`
 * Total Len: `0x10000`

#### Dynamic FS (aka spiffs1 aka dynfs)
 * Start Addr: `0x360000`
 * End Addr: `0x3fc000`
 * Total Len: `0x50000`


### Generating the rBoot config

**Config Parameters**:
* SECTOR_SIZE = 0x1000
* BOOT_CONFIG_SECTOR = 1
* flashsize = 0x400000

**Default rBoot Config**:
The vanilla rBoot configuration has 2 roms, one on the first MB and the other
one halfway through your flash size, whatever it may be. So for an Esp-12e (4MB)
this is at the start of the 3rd MB:
```
romconf->count = 2;
romconf->roms[0] = SECTOR_SIZE * (BOOT_CONFIG_SECTOR + 1);
romconf->roms[1] = (flashsize / 2) + (SECTOR_SIZE * (BOOT_CONFIG_SECTOR + 1));
```

### Relavent info from rBoot README

Linking user code
-----------------
Each rom will need to be linked with an appropriate linker file, specifying
where it will reside on the flash. If you are only flashing one rom to multiple
places on the flash it must be linked multiple times to produce the set of rom
images. This is the same as with the SDK loader.

Because there are endless possibilities for layout with this loader I don't
supply sample linker files. Instead I'll tell you how to make them.

For each rom slot on the flash take a copy of the eagle.app.v6.ld linker script
from the sdk. You then need to modify just one line in it for each rom:
```
  irom0_0_seg :                         org = 0x40240000, len = 0x3C000
```

Change the org address to be 0x40200000 (base memory mapped location of the
flash) + flash address + 0x10 (offset of data after the header).

The logical place for your first rom is the third sector, address 0x2000:
```
  0x40200000 + 0x2000 + 0x10 = 0x40202010
```

If you use the default generated config the loader will expect to find the
second rom at flash address half-chip-size + 0x2000 (e.g. 0x82000 on an 8MBit
flash) so the irom0_0_seg should be:
```
  0x40200000 + 0x82000 + 0x10 = 0x40282010
```

Then simply compile and link as you would normally for OTA updates with the SDK
boot loader, except using the linker scripts you've just prepared rather than
the ones supplied with the SDK. Remember when building roms to create them as
'new' type roms (for use with SDK boot loader v1.2+). Or if using my esptool2
use the -boot2 option. Note: the test loads included with rBoot are built with
-boot0 because they do not contain a .irom0.text section (and so the value of
irom0_0_seg in the linker file is irrelevant to them) but 'normal' user apps
always do.
