#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lib/profile.h"
#include "lib/utils/color.h"
#include "lib/utils/functions.h"
#include "lib/utils/vars.h"

#define EXIT_USAGE 2

typedef enum {
    STEP_COLOR,   // give a color to some zones
    STEP_LOAD,    // merge a saved profile
} StepKind;

// What to do, in the order given on the command line
typedef struct {
    StepKind kind;
    unsigned zones;
    int rgb[3];
    const char *profile;
} Step;

static const char *program_name = "controller";

static void usage(FILE *out) {
    fprintf(out,
            "Usage: %s [OPTIONS]\n"
            "\n"
            "Set the AlienFX lighting of an Alienware M14x.\n"
            "\n"
            "Zones (each takes a COLOR, applied in the order given):\n"
            "  -a, --all COLOR                    every zone\n"
            "  -k, --keyboard COLOR               the whole keyboard\n"
            "  -1, --keyboard-left COLOR          keyboard zone 1 (left)\n"
            "  -2, --keyboard-middle-left COLOR   keyboard zone 2\n"
            "  -3, --keyboard-middle-right COLOR  keyboard zone 3\n"
            "  -4, --keyboard-right COLOR         keyboard zone 4 (right)\n"
            "  -t, --touchpad COLOR               touchpad\n"
            "  -m, --mediabar COLOR               media bar\n"
            "  -s, --speakers COLOR               left and right speakers\n"
            "  -l, --logo COLOR                   Alienware name and alien head on the lid\n"
            "\n"
            "Profiles (stored in ~/.config/alienfx/profiles):\n"
            "  -p, --load NAME           apply a saved profile (other zones options can follow)\n"
            "  -S, --save NAME           save the resulting colors as a profile\n"
            "  -L, --list                list the saved profiles\n"
            "  -D, --delete NAME         delete a saved profile\n"
            "\n"
            "Options:\n"
            "  -b, --brightness PERCENT  dim every color (0-100, default 100)\n"
            "  -n, --dry-run             print what would be sent, don't touch the device\n"
            "  -c, --list-colors         list the color names\n"
            "  -h, --help                show this help\n"
            "\n"
            "COLOR is one of:\n"
            "  R,G,B     three values from 0 to 15, e.g. 15,0,8\n"
            "  #RRGGBB   hex color, rounded to the 16 levels of the hardware, e.g. #00b4ff\n"
            "  NAME      a color name (see --list-colors), e.g. alien or off\n"
            "\n"
            "Examples:\n"
            "  %s --all alien\n"
            "  %s -a off -k '#ff2bd6' -t 15,15,15 --save pink\n"
            "  %s --load pink --touchpad off\n"
            "  %s -n -1 red -2 orange -3 yellow -4 green --save warm   (save without applying)\n",
            program_name, program_name, program_name, program_name, program_name);
}

static void print_try_help(void) {
    fprintf(stderr, "Try '%s --help' for more information.\n", program_name);
}

// --list: name, then a block per zone when printing to a terminal
// (colored when lit, "--" when off, "··" when the profile leaves the zone unchanged)
static void print_profile(const char *name, void *data) {
    int color = *(int *) data;
    if (!color) {
        printf("%s\n", name);
        return;
    }
    Profile profile = {0};
    printf("%-20s", name);
    if (profile_load(name, &profile) != 0) {
        printf("  (unreadable)\n");
        return;
    }
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        if (i == PROFILE_TOUCHPAD) {
            printf(" ");
        }
        if (profile.set[i] && profile.rgb[i][0] + profile.rgb[i][1] + profile.rgb[i][2] == 0) {
            printf(" \033[2m--\033[0m");
        } else if (profile.set[i]) {
            printf(" \033[38;2;%d;%d;%dm██\033[0m",
                    profile.rgb[i][0] * 17, profile.rgb[i][1] * 17, profile.rgb[i][2] * 17);
        } else {
            printf(" \033[2m··\033[0m");
        }
    }
    printf("\n");
}

static void print_zone_names(unsigned zones) {
    if (zones == PROFILE_ALL_ZONES) {
        printf("all");
        return;
    }
    const char *separator = "";
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        if (zones & (1u << i)) {
            printf("%s%s", separator, PROFILE_ZONES[i].name);
            separator = ", ";
        }
    }
}

// Send the profile, one USB command per distinct color
static void apply(libusb_device_handle *usbhandle, const Profile *profile) {
    unsigned done = 0;
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        if (!profile->set[i] || (done & (1u << i))) {
            continue;
        }
        const int *rgb = profile->rgb[i];
        unsigned zones = 0;
        int mask = 0;
        for (int j = i; j < PROFILE_ZONE_COUNT; j += 1) {
            if (profile->set[j] && memcmp(profile->rgb[j], rgb, sizeof(profile->rgb[j])) == 0) {
                zones |= 1u << j;
                mask |= PROFILE_ZONES[j].mask;
            }
        }
        done |= zones;

        if (usbhandle == NULL) {
            printf("R %2d  G %2d  B %2d  (mask 0x%04x)  ", rgb[0], rgb[1], rgb[2], mask);
            print_zone_names(zones);
            printf("\n");
        } else {
            set_zone_color(usbhandle, mask, rgb[0], rgb[1], rgb[2]);
        }
    }
}

int main(int argc, char *argv[]) {
    static const struct option long_options[] = {
        {"all", required_argument, NULL, 'a'},
        {"keyboard", required_argument, NULL, 'k'},
        {"keyboard-left", required_argument, NULL, '1'},
        {"keyboard-middle-left", required_argument, NULL, '2'},
        {"keyboard-middle-right", required_argument, NULL, '3'},
        {"keyboard-right", required_argument, NULL, '4'},
        {"touchpad", required_argument, NULL, 't'},
        {"mediabar", required_argument, NULL, 'm'},
        {"speakers", required_argument, NULL, 's'},
        {"logo", required_argument, NULL, 'l'},
        {"load", required_argument, NULL, 'p'},
        {"save", required_argument, NULL, 'S'},
        {"list", no_argument, NULL, 'L'},
        {"delete", required_argument, NULL, 'D'},
        {"brightness", required_argument, NULL, 'b'},
        {"dry-run", no_argument, NULL, 'n'},
        {"list-colors", no_argument, NULL, 'c'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0},
    };
    // Zone each short option sets, as named in profiles
    static const char *ZONE_OPTIONS[128] = {
        ['a'] = "all",
        ['k'] = "keyboard",
        ['1'] = "keyboard-left",
        ['2'] = "keyboard-middle-left",
        ['3'] = "keyboard-middle-right",
        ['4'] = "keyboard-right",
        ['t'] = "touchpad",
        ['m'] = "mediabar",
        ['s'] = "speakers",
        ['l'] = "logo",
    };

    if (argc > 0 && argv[0] != NULL) {
        const char *slash = strrchr(argv[0], '/');
        program_name = slash != NULL ? slash + 1 : argv[0];
    }

    Step *steps = calloc(argc, sizeof(Step));
    int count = 0;
    int brightness = 100;
    int dry_run = 0;
    int list = 0;
    const char *save_name = NULL;
    const char *delete_name = NULL;
    int opt;

    while ((opt = getopt_long(argc, argv, "a:k:1:2:3:4:t:m:s:l:p:S:LD:b:nch", long_options, NULL)) != -1) {
        switch (opt) {
            case 'a': case 'k': case '1': case '2': case '3': case '4':
            case 't': case 'm': case 's': case 'l':
                steps[count].kind = STEP_COLOR;
                steps[count].zones = profile_zone_group(ZONE_OPTIONS[opt]);
                if (parse_color(optarg, steps[count].rgb) != 0) {
                    fprintf(stderr, "%s: invalid color '%s' for --%s\n", program_name, optarg, ZONE_OPTIONS[opt]);
                    fprintf(stderr, "Use R,G,B (0-15 each), #RRGGBB or a name from --list-colors.\n");
                    return EXIT_USAGE;
                }
                count += 1;
                break;
            case 'p':
            case 'S':
            case 'D':
                if (!profile_valid_name(optarg)) {
                    fprintf(stderr, "%s: invalid profile name '%s': use letters, digits, '.', '_' or '-'\n",
                            program_name, optarg);
                    return EXIT_USAGE;
                }
                if (opt == 'p') {
                    steps[count].kind = STEP_LOAD;
                    steps[count].profile = optarg;
                    count += 1;
                } else if (opt == 'S') {
                    save_name = optarg;
                } else {
                    delete_name = optarg;
                }
                break;
            case 'L':
                list = 1;
                break;
            case 'b':
                if (parse_int(optarg, 0, 100, &brightness) != 0) {
                    fprintf(stderr, "%s: invalid brightness '%s', expected 0-100\n", program_name, optarg);
                    return EXIT_USAGE;
                }
                break;
            case 'n':
                dry_run = 1;
                break;
            case 'c':
                list_named_colors(stdout);
                return EXIT_SUCCESS;
            case 'h':
                usage(stdout);
                return EXIT_SUCCESS;
            default:
                print_try_help();
                return EXIT_USAGE;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "%s: unexpected argument '%s'\n", program_name, argv[optind]);
        print_try_help();
        return EXIT_USAGE;
    }
    if (count == 0 && !list && delete_name == NULL) {
        if (save_name != NULL) {
            fprintf(stderr, "%s: nothing to save, give some zone colors or --load a profile\n", program_name);
            return EXIT_USAGE;
        }
        usage(stderr);
        return EXIT_USAGE;
    }

    if (delete_name != NULL) {
        if (profile_delete(delete_name) != 0) {
            return EXIT_FAILURE;
        }
        printf("Deleted profile '%s'\n", delete_name);
    }
    if (list) {
        int color = isatty(STDOUT_FILENO);
        int listed = profile_list(print_profile, &color);
        if (listed < 0) {
            return EXIT_FAILURE;
        }
        if (listed == 0 && color) {
            printf("No saved profile yet, create one with --save NAME\n");
        }
    }
    if (count == 0) {
        return EXIT_SUCCESS;
    }

    // Resolve every step into the final color of each zone
    Profile profile = {0};
    for (int i = 0; i < count; i += 1) {
        if (steps[i].kind == STEP_LOAD) {
            if (profile_load(steps[i].profile, &profile) != 0) {
                return EXIT_FAILURE;
            }
        } else {
            profile_set(&profile, steps[i].zones, steps[i].rgb);
        }
    }
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        for (int c = 0; c < 3; c += 1) {
            profile.rgb[i][c] = (profile.rgb[i][c] * brightness + 50) / 100;
        }
    }

    if (dry_run) {
        apply(NULL, &profile);
    } else {
        libusb_device_handle *usbhandle = try_connect_usb();
        if (usbhandle == NULL) {
            fprintf(stderr, "%s: cannot open the AlienFX device (%04x:%04x).\n",
                    program_name, ALIENWARE_VENDORID, ALIENWARE_PRODUCTID_M14XR2);
            fprintf(stderr, "Run with sudo or add the udev rule described in the README.\n");
            return EXIT_FAILURE;
        }
        apply(usbhandle, &profile);
    }

    if (save_name != NULL) {
        char path[4096];
        if (profile_save(save_name, &profile) != 0 || profile_path(save_name, path, sizeof(path)) != 0) {
            return EXIT_FAILURE;
        }
        printf("Saved profile '%s' to %s\n", save_name, path);
    }

    free(steps);
    return EXIT_SUCCESS;
}
