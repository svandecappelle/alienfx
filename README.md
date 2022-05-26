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
./bin/controller-ui
```

Be sure you are able to connect to usbdevices or use script in sudo mode

For UI you shoudl configure the rights in `/etc/udev/rules.d/usb.rules` with this content
```
SUBSYSTEM=="usb", MODE="0666"
```


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

