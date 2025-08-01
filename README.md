# DiscEmu

A CD-ROM drive emulator based on Linux USB Gadget, on the Milk-V Duo (CV1800B(64M Model) and SG2000 variant(256M Model)) board.

Currently at prototype stage.

![prototype](img/prototype.jpg)

## Hardware info

- Milk-V Duo board
- SSD1306 SPI 128x64 OLED **(256M and 64M)**/SSD1306 I2C 128x64 OLED **(64M only)**
- EC11 rotary encoder / 3 * GPIO Button
### Pinout

**Default Use SPI OLED and EC11.**

SPI OLED Pinout: 

|Function | Pin# | GP#  |
|---------|------|------|
|OLED MOSI| 10   | GP7  |
|OLED SCK | 9    | GP6  |
|OLED RES | 29   | GP22 |
|OLED DC  | 27   | GP21 |

I2C OLED Pinout **(64M only)**: 

|Function | Pin# | GP#  |
|---------|------|------|
|OLED SCL | 1    | GP0  |
|OLED SDA | 2    | GP1  |

GPIO Pinout:
|Function | Pin# | GP#  |
|---------|------|------|
|Up Btn   | 26   | GP20 |
|Down Btn | 25   | GP19 |
|Enter Btn| 24   | GP18 |

EC11 Pinout:
|Function | Pin# | GP#  |
|---------|------|------|
|EC11 S1  | 26   | GP20 |
|EC11 S2  | 25   | GP19 |
|EC11 KEY | 24   | GP18 |


## Build

You can build this with the [official SDK](https://github.com/milkv-duo/host-tools).

This app depends on the libu8g2arm library, please use [my fork(for 256M Model)](https://github.com/slzkud/libu8g2arm-milkvduo-256m) or [driver1998 fork(for 64M Model)](https://github.com/driver1998/libu8g2arm-milkvduo) with Milk-V Duo GPIO pin number mapping. (GPIO pin number of Duo in Linux userspace are three digits, which break assumptions in libu8g2arm. A lookup table which maps Linux pin number to actual physical pin number is added as a workaround.) 

Change makefile with the new TOOLCHAIN_PREFIX and U8G2_PREFIX, and use ``make`` to compile program.

- If you need to use I2C OLED and GPIO Button, use ``make KEYPAD_INPUT=1 I2C_DISPLAY=1`` to compile program.

- If you need to disable USB because need to debug, use ``make USB_ON=0`` to compile program. 

## Usage

- This app is statically linked by default, and should run fine on Milk-V Duo official images. 

- `run_usb.sh` in this repo is modified from the official image, with CD-ROM support added, please use this verison with DiscEmu instead of the stock one.

- Place `disc-emu` and `run_usb.sh` at `/mnt/data`, and make it start at boot up by creating `/mnt/data/auto.sh`

```bash
OLD_PWD=$PWD
cd /mnt/data
nohup ./disc-emu
cd $OLD_PWD
```

- Disable RNDIS at boot up by removing `/mnt/system/usb.sh` to avoid conflicts. You can start RNDIS at the main menu of DiscEmu.

- If you need to use "File Transfer", you need to compile [uMTP-Responder](https://github.com/viveris/uMTP-Responder). After compiling, place the compiled `umtprd` in `/usr/bin` and the [`umtprd.conf`](umtprd/umtprd.conf) in `/etc/umtprd`.

- DiscEmu looks for ISO images at `isos` in the working directory, so `/mnt/data/isos` as configured above. You might want to mount a partition with large enough space to this directory.

- Due to Linux kernel limitations, DVD images are not fully supported. Also there is a ~2.2GiB file size limit, there is a third-party patch to bypass this, see https://lkml.org/lkml/2015/3/7/388 for more details.

