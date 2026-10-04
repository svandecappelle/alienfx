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
./bin/controller --all alien
./bin/controller -a off -k '#ff2bd6' -t 15,15,15

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
- *My profiles*: name the current colors to save them, click a profile to apply it.
  They are the same files as the script mode profiles (see below), so a profile
  saved in the UI can be loaded with `controller --load NAME` and vice versa
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

## Script mode

`./bin/controller --help` lists every option. Zones are applied in the order
given, so `-a blue -1 red` turns everything blue except the left keys.

| Option                          | Zone                          |
|---------------------------------|-------------------------------|
| `-a`, `--all`                   | every zone                    |
| `-k`, `--keyboard`              | the whole keyboard            |
| `-1` … `-4`, `--keyboard-left` … `--keyboard-right` | one keyboard zone, from left to right |
| `-t`, `--touchpad`              | touchpad                      |
| `-m`, `--mediabar`              | media bar                     |
| `-s`, `--speakers`              | left and right speakers       |
| `-l`, `--logo`                  | alien head and Alienware name |

Each zone takes a color, written as:

- `r,g,b`: three values from 0 (off) to 15 (full), e.g. `15,0,8`
- `#rrggbb`: a hex color, rounded to the 16 levels of the hardware, e.g. `#00b4ff`
- a name: `alien`, `red`, `off`… (`--list-colors` shows them all)

### Profiles

Colors can be saved as named profiles in `~/.config/alienfx/profiles/<name>.conf`
(or `$XDG_CONFIG_HOME/alienfx/profiles`). When run with `sudo`, profiles are still
stored in the home of the user who ran `sudo` and stay owned by that user.

| Option                  |                                                        |
|-------------------------|--------------------------------------------------------|
| `-S`, `--save NAME`     | save the resulting colors as a profile                 |
| `-p`, `--load NAME`     | apply a profile, it can be mixed with other zone options |
| `-L`, `--list`          | list the profiles (with their colors in a terminal)    |
| `-D`, `--delete NAME`   | delete a profile                                       |

```
./bin/controller -a off -k '#ff2bd6' -t white --save pink   # apply and save
./bin/controller -n -a alien -l purple --save chill         # save without applying
./bin/controller --load pink --touchpad off                 # load, then override a zone
./bin/controller --list
```

A profile is a plain text file that can be written by hand. Each line is
`zone = color`, applied from top to bottom; zones not listed are left unchanged:

```
# zones: all, keyboard, keyboard-left, keyboard-middle-left,
#        keyboard-middle-right, keyboard-right, touchpad, mediabar, speakers, logo
all = off
keyboard = #00b4ff
touchpad = 15,0,0
```

Other options:

| Option                  |                                                   |
|-------------------------|---------------------------------------------------|
| `-b`, `--brightness N`  | dim every color to N percent                      |
| `-n`, `--dry-run`       | print what would be sent without opening the device |
| `-c`, `--list-colors`   | list the color names                              |
| `-h`, `--help`          | show the help                                     |

Exit codes: `0` success, `1` device or profile error, `2` invalid arguments.
