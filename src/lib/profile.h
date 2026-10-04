#ifndef PROFILE_H
#define PROFILE_H

// Individual lighting zones a profile can hold
enum {
    PROFILE_KEYBOARD_LEFT,
    PROFILE_KEYBOARD_MIDDLE_LEFT,
    PROFILE_KEYBOARD_MIDDLE_RIGHT,
    PROFILE_KEYBOARD_RIGHT,
    PROFILE_TOUCHPAD,
    PROFILE_MEDIABAR,
    PROFILE_SPEAKERS,
    PROFILE_LOGO,
    PROFILE_ZONE_COUNT
};

#define PROFILE_ALL_ZONES ((1u << PROFILE_ZONE_COUNT) - 1)

typedef struct {
    const char *name;   // name used on the command line and in profile files
    int mask;           // hardware zone bitmask
} ProfileZone;

extern const ProfileZone PROFILE_ZONES[PROFILE_ZONE_COUNT];

// Colors of the zones, as hardware values (0-15 per channel)
typedef struct {
    int set[PROFILE_ZONE_COUNT];
    int rgb[PROFILE_ZONE_COUNT][3];
} Profile;

// Zones (bitmask of zone indexes) named by a zone name, "keyboard" or "all". 0 when unknown.
unsigned profile_zone_group(const char *name);

// Give a color to every zone of the bitmask
void profile_set(Profile *profile, unsigned zones, const int rgb[3]);

// Profiles are stored as <config dir>/alienfx/profiles/<name>.conf
// These functions print their errors on stderr and return -1 on failure,
// profile_last_error() then returns the message.
int profile_valid_name(const char *name);
int profile_path(const char *name, char *buf, unsigned long size);
int profile_save(const char *name, const Profile *profile);
// Merge the profile file into the given profile (zones it sets override existing ones)
int profile_load(const char *name, Profile *profile);
int profile_delete(const char *name);
// Call fn for each saved profile, in alphabetical order. Returns the number of profiles.
int profile_list(void (*fn)(const char *name, void *data), void *data);
const char * profile_last_error(void);

#endif
