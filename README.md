# Linux Software for the Ajazz
This is a software to control LEDs, lighting modes, LCD screen and macros

## TODO
 - [ ] Install and easy use
 - [x] Lighting modes
 - [ ] Individual LEDs
 - [ ] Macros
 - [ ] LCD screen images & GIF
 - [ ] MULTI LANGUAGE SUPPORT
 - [ ] Sound Reaction (??)
 - [ ] Battery Status

## Installation
Usual cmake build. You also need to install [hidapi](https://github.com/libusb/hidapi)
```bash
sudo apt install libhidapi-dev
```

## How did I do it
At first, I thought it would be a good idea to reverse engineer the
official software but since it's made by chinese developers, even
the decompilation had weird control flow and logic so instead I used
Wireshark with PCapUSB to sniff packets and compare them
[I made a parser for pcap files that parses packets and show their infos](https://github.com/TaxMachine/ajazz-keyboard-software-linux/blob/master/analyze/hidfilter.py)

## New feature

Note from Vixcho: The way of working is very strange but to put images of my cat on my keyboard, I will do what I can