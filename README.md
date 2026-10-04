# AlienFX for Linux

Control the lights of an Alienware M14x: a GTK 4 application (`alienfx`) and a
command line tool (`alienfx-cli`).

# Build and install

Requires CMake, a C compiler, GTK 4 and libusb 1.0 (and `rsvg-convert` from
librsvg to generate the PNG icons, otherwise only the SVG icon is installed).

```
cmake -B build
cmake --build build
sudo cmake --install build      # or: cd build && sudo make install
```

This installs:

- `alienfx` (the UI, in your application menu as *AlienFX*) and `alienfx-cli`
- `alienfx-desktop-icon`, to put an AlienFX icon on your desktop (see below)
- the desktop entry, the logo icon (SVG and PNG from 16 to 512 px) and AppStream metadata
- a udev rule giving the logged in user access to the AlienFX controller, so
  neither program needs `sudo`

The prefix defaults to `/usr/local` (`-DCMAKE_INSTALL_PREFIX=/usr` to change it).
The udev rule always goes to udev's own directory (`/usr/lib/udev/rules.d`).
Remove everything with `sudo make uninstall` from the `build` directory.

## Desktop icon

To also get an AlienFX icon on your desktop, run as your user (not with `sudo`):

```
alienfx-desktop-icon            # or: make desktop-icon, from the build directory
alienfx-desktop-icon --remove   # remove it
```

It copies the launcher to your desktop folder (`xdg-user-dir DESKTOP`), pointing
directly at the installed logo image so it always shows, and marks it as trusted,
so GNOME, KDE, LXQt and Xfce run it without asking first.

Without installing, the programs are in `build/bin`:

```
./build/bin/alienfx
./build/bin/alienfx-cli --all alien
```

## UI

The UI shows the laptop with its lights: click a zone on the drawing (or pick it
in the side panel) then choose a color, a brightness or one of the presets.

- Zones: the 4 keyboard zones, touchpad, media bar, speakers and logo
- The preview shows the colors as the hardware renders them (16 levels per channel)
- Colors are saved in `~/.config/alienfx/zones.ini` and restored on the next launch
- *My profiles*: name the current colors to save them, click a profile to apply it.
  They are the same files as the script mode profiles (see below), so a profile
  saved in the UI can be loaded with `alienfx-cli --load NAME` and vice versa
- Without a device (or without USB permissions) the UI starts in *preview mode*

### Power button

The power button light is driven by the laptop firmware from its power state,
so it can't be given a live color like the other zones. Select *Power button*
to give it a style for each power state (booting, plugged in, charging, on
battery, battery critical and asleep): off, steady, pulse between two colors
or blink. *Write to laptop* stores these styles in the keyboard controller,
together with the current colors of the other zones so they are kept when the
power source changes. *Dell defaults* restores the original styles.

## USB permissions

`make install` installs the udev rule. To use the programs from `build/bin`
without installing, install only the rule, which gives the logged in user
access to the AlienFX controller (and only to it):

```
sudo cp udev/70-alienfx.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Avoid running the UI with `sudo`: GTK can't reach your desktop session as root
and prints "Unable to acquire session bus".

# Supported devices

| Device          | supported |
|-----------------|-----------|
| M14xR1 & M14xR2 | yes       |

---

## Script mode

`alienfx-cli --help` lists every option. Zones are applied in the order
given, so `-a blue -1 red` turns everything blue except the left keys.

| Option                          | Zone                          |
|---------------------------------|-------------------------------|
| `-a`, `--all`                   | every zone                    |
| `-k`, `--keyboard`              | the whole keyboard            |
| `-1` … `-4`, `--keyboard-left` … `--keyboard-right` | one keyboard zone, from left to right |
| `-t`, `--touchpad`              | touchpad                      |
| `-m`, `--mediabar`              | media bar                     |
| `-s`, `--speakers`              | left and right speakers       |
| `-l`, `--logo`                  | Alienware name and alien head on the lid |

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
alienfx-cli -a off -k '#ff2bd6' -t white --save pink   # apply and save
alienfx-cli -n -a alien -l purple --save chill         # save without applying
alienfx-cli --load pink --touchpad off                 # load, then override a zone
alienfx-cli --list
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
