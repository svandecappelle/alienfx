# To build

Using Cmake
```
mkdir build
cd build
cmake ../
make
```

# To launch
```
Script mode: 
./bin/controller -k r,g,b -l r,g,b <...>

UI mode:
./bin/alienFx
```

Be sure you are able to connect to usbdevices or use script in sudo mode

## UI

The UI shows the laptop with its lights: click a zone on the drawing (or pick it
in the side panel) then choose a color, a brightness or one of the presets.

- Zones: the 4 keyboard zones, touchpad, media bar, speakers and logo
- The preview shows the colors as the hardware renders them (16 levels per channel)
- Colors are saved in `~/.config/alienfx/zones.ini` and restored on the next launch
- Without a device (or without USB permissions) the UI starts in *preview mode*

For UI you shoudl configure the rights in `/etc/udev/rules.d/usb.rules` with this content
```
SUBSYSTEM=="usb", MODE="0666"
```

# Supported devices

| Device          | supported |
|-----------------|-----------|
| M14xR1 & M14xR2 | yes       |

---

For script mode
parameters options:

| Option |                |
|--------|----------------|
| k      | Keyboard color |
| t      | Touchpad color |
| m      | Mediabar color |
| l      | Logo color     |
| s      | Speaker color  |

And value in format "r,g,b". With each color value is an integer between 0 and 15. (0 means no color, 15 means full color)

