# NxpNfcCockpit SPI Adapter

The [Nfc Cockpit by NXP](https://www.nxp.com/products/rfid-nfc/nfc-hf/nfc-readers/nfc-cockpit-configuration-tool-for-nfc-ics:NFC-COCKPIT) is a GUI program, that allows you to play around with
- CLRC66303HN
- PN5180
- PN5190
- PN7220
- PN7462
- PN7642
provided you can make the stupid GUI app run. Under Linux it works surprisingly well, check out the section below.
For the computer to be able to communicate to your device, it needs an adapter board, that translates between USB and SPI.
While NXP provides some code themselves, I wanted to have something arduino-based, hence this project was born.

This code appears to the PC as a serial device, while translating to SPI commands for the NFC device.
It supports just enough of the BAL protocol for the Cockpit to work.

## Configuration

Although this is a platformIO project, with some work it can surely made to run in native arduino.

In the [platformio.ini](platformio.ini), you'll find two predefined configurations.
Just connect your board to the NFC reader, add a new configuration or reuse an old one, adjust the pins and upload.

## Supported boards

The Cockpit does something weird to the serial device, so not all boards fully work.
The serial adapter on my bootled esp32 board does not spit out any usable data. You'll have to check yourself.

I can personally only recommend an rp2040 based board (I personally used a `rp2040 zero`, original ones aswell as clones), since they have a usable USB-Serial implementation aswell as an RGB LED (enable through the WS2812_PIN macro).

## Running the Cockpit under Windows

No idea. You're on your own. This project should appear as a regular COM device. You probably just have to select it in the Cockpit.

## Running the Cockpit under Linux

1. Flash your board according to the instructions above
2. Create an account and download the [Cockpit](https://www.nxp.com/products/rfid-nfc/nfc-hf/nfc-readers/nfc-cockpit-configuration-tool-for-nfc-ics:NFC-COCKPIT).
3. Create a 32-bit wine prefix 
4. Copy the installation .exe onto drive C (or don't and open directly from wine).
5. Open the installation .exe and install wherever you want
6. Connect the board to your computer
7. Check which serial port it appears under (e.g. `/dev/ttyACM0`)
8. Check under /home/YOUR_USER/.local/share/wineprefixes/YOUR_PREFIX/dosdevices/ what COM port your serial device corresponds to
9. Under the Cockpit install dir, adjust `/cfg/NxpNfcCockpit_Configuration.ini` according to your board/COM port to read this:
```
[EnforceBoardSelection]
;    PN5190_K8x=COM66
;    PN5190=COM55
PN5180=COM100
;    RC663=COM33
;    PN7462=COM1
;    PN7642=COM1
```
10. cd into `bin`
11. Activate your prefix using `export WINEPREFIX=/home/YOUR_USER/.local/share/wineprefixes/YOUR_PREFIX`
12. Check the connection to the board using `wine UcBalPCTestApp.exe /GetBusy`
13. Check the SPI connection by installing `pyserial` and running `python connection_test.py /dev/ttyACM0` from this repository (with the right serial device, duh)
14. If all works, you ready to launch `wine NxpNfcCockpitGUI.exe`
15. If something fails, you can check the logs under `_NxpNfcCockpitLog_*.log`