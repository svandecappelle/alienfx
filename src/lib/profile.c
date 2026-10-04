#include <ctype.h>
#include <stdarg.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "profile.h"
#include "utils/color.h"
#include "utils/vars.h"

#define PROFILE_EXTENSION ".conf"
#define MAX_NAME_LENGTH 64

static char last_error[1024];

// Print an error and keep it for profile_last_error()
static void report(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(last_error, sizeof(last_error), format, args);
    va_end(args);
    fprintf(stderr, "%s\n", last_error);
}

const char * profile_last_error(void) {
    return last_error;
}

const ProfileZone PROFILE_ZONES[PROFILE_ZONE_COUNT] = {
    [PROFILE_KEYBOARD_LEFT] = {"keyboard-left", ZONE_KEYBOARD_LEFT},
    [PROFILE_KEYBOARD_MIDDLE_LEFT] = {"keyboard-middle-left", ZONE_KEYBOARD_MIDDLE_LEFT},
    [PROFILE_KEYBOARD_MIDDLE_RIGHT] = {"keyboard-middle-right", ZONE_KEYBOARD_MIDDLE_RIGHT},
    [PROFILE_KEYBOARD_RIGHT] = {"keyboard-right", ZONE_KEYBOARD_RIGHT},
    [PROFILE_TOUCHPAD] = {"touchpad", ZONE_TOUCHPAD},
    [PROFILE_MEDIABAR] = {"mediabar", ZONE_MEDIABAR},
    [PROFILE_SPEAKERS] = {"speakers", ZONE_SPEAKER_LEFT | ZONE_SPEAKER_RIGHT},
    [PROFILE_LOGO] = {"logo", ZONE_ALIEN_HEAD | ZONE_ALIEN_NAME},
};

unsigned profile_zone_group(const char *name) {
    if (strcmp(name, "all") == 0) {
        return PROFILE_ALL_ZONES;
    }
    if (strcmp(name, "keyboard") == 0) {
        return (1u << PROFILE_KEYBOARD_LEFT) | (1u << PROFILE_KEYBOARD_MIDDLE_LEFT)
            | (1u << PROFILE_KEYBOARD_MIDDLE_RIGHT) | (1u << PROFILE_KEYBOARD_RIGHT);
    }
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        if (strcmp(name, PROFILE_ZONES[i].name) == 0) {
            return 1u << i;
        }
    }
    return 0;
}

void profile_set(Profile *profile, unsigned zones, const int rgb[3]) {
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        if (zones & (1u << i)) {
            profile->set[i] = 1;
            memcpy(profile->rgb[i], rgb, sizeof(profile->rgb[i]));
        }
    }
}

// ---------------------------------------------------------------------------
// Location
// ---------------------------------------------------------------------------

// When run through sudo, profiles belong to the user who invoked sudo, not root
static int sudo_user(uid_t *uid, gid_t *gid) {
    const char *sudo_uid = getenv("SUDO_UID");
    const char *sudo_gid = getenv("SUDO_GID");
    int u, g;
    if (geteuid() != 0 || sudo_uid == NULL || sudo_gid == NULL
            || parse_int(sudo_uid, 0, 0x7fffffff, &u) != 0
            || parse_int(sudo_gid, 0, 0x7fffffff, &g) != 0) {
        return 0;
    }
    *uid = (uid_t) u;
    *gid = (gid_t) g;
    return 1;
}

static void give_to_sudo_user(const char *path) {
    uid_t uid;
    gid_t gid;
    if (sudo_user(&uid, &gid) && chown(path, uid, gid) != 0) {
        report("warning: cannot give %s back to uid %d: %s", path, (int) uid, strerror(errno));
    }
}

static int config_dir(char *buf, size_t size) {
    uid_t uid;
    gid_t gid;
    const char *xdg = getenv("XDG_CONFIG_HOME");
    int written;

    if (sudo_user(&uid, &gid)) {
        struct passwd *pw = getpwuid(uid);
        if (pw == NULL) {
            report("cannot find the home directory of uid %d", (int) uid);
            return -1;
        }
        written = snprintf(buf, size, "%s/.config", pw->pw_dir);
    } else if (xdg != NULL && xdg[0] == '/') {
        written = snprintf(buf, size, "%s", xdg);
    } else {
        const char *home = getenv("HOME");
        if (home == NULL || home[0] == '\0') {
            struct passwd *pw = getpwuid(getuid());
            home = pw != NULL ? pw->pw_dir : NULL;
        }
        if (home == NULL) {
            report("cannot find the home directory");
            return -1;
        }
        written = snprintf(buf, size, "%s/.config", home);
    }
    return written > 0 && (size_t) written < size ? 0 : -1;
}

static int profiles_dir(char *buf, size_t size) {
    char base[4096];
    if (config_dir(base, sizeof(base)) != 0) {
        return -1;
    }
    int written = snprintf(buf, size, "%s/alienfx/profiles", base);
    return written > 0 && (size_t) written < size ? 0 : -1;
}

// mkdir -p, giving the created directories to the sudo user
static int make_dirs(const char *path) {
    char tmp[4096];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; ; p += 1) {
        if (*p == '/' || *p == '\0') {
            char saved = *p;
            *p = '\0';
            if (mkdir(tmp, 0755) == 0) {
                give_to_sudo_user(tmp);
            } else if (errno != EEXIST) {
                report("cannot create %s: %s", tmp, strerror(errno));
                return -1;
            }
            *p = saved;
            if (saved == '\0') {
                break;
            }
        }
    }
    return 0;
}

int profile_valid_name(const char *name) {
    size_t length = strlen(name);
    if (length == 0 || length > MAX_NAME_LENGTH || name[0] == '.' || name[0] == '-') {
        return 0;
    }
    return strspn(name, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") == length;
}

int profile_path(const char *name, char *buf, unsigned long size) {
    char dir[4096];
    if (!profile_valid_name(name)) {
        report("invalid profile name '%s': use letters, digits, '.', '_' or '-' (%d characters max)",
                name, MAX_NAME_LENGTH);
        return -1;
    }
    if (profiles_dir(dir, sizeof(dir)) != 0) {
        return -1;
    }
    int written = snprintf(buf, size, "%s/%s%s", dir, name, PROFILE_EXTENSION);
    return written > 0 && (unsigned long) written < size ? 0 : -1;
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

int profile_save(const char *name, const Profile *profile) {
    char dir[4096], path[4096], tmp[4200];
    if (profile_path(name, path, sizeof(path)) != 0 || profiles_dir(dir, sizeof(dir)) != 0 || make_dirs(dir) != 0) {
        return -1;
    }

    // Write to a temporary file then rename, so a failure never leaves a truncated profile
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    FILE *file = fopen(tmp, "w");
    if (file == NULL) {
        report("cannot write %s: %s", tmp, strerror(errno));
        return -1;
    }
    fprintf(file, "# AlienFX profile \"%s\"\n", name);
    fprintf(file, "# zone = color (r,g,b from 0 to 15, #rrggbb or a color name)\n");
    fprintf(file, "# zones: all, keyboard, ");
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        fprintf(file, "%s%s", PROFILE_ZONES[i].name, i + 1 < PROFILE_ZONE_COUNT ? ", " : "\n\n");
    }
    for (int i = 0; i < PROFILE_ZONE_COUNT; i += 1) {
        if (profile->set[i]) {
            fprintf(file, "%s = %d,%d,%d\n", PROFILE_ZONES[i].name,
                    profile->rgb[i][0], profile->rgb[i][1], profile->rgb[i][2]);
        }
    }

    int failed = ferror(file);
    if (fclose(file) != 0 || failed) {
        report("cannot write %s: %s", tmp, strerror(errno));
        unlink(tmp);
        return -1;
    }
    give_to_sudo_user(tmp);
    if (rename(tmp, path) != 0) {
        report("cannot save %s: %s", path, strerror(errno));
        unlink(tmp);
        return -1;
    }
    return 0;
}

static char * trim(char *text) {
    while (isspace((unsigned char) *text)) {
        text += 1;
    }
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char) end[-1])) {
        end -= 1;
    }
    *end = '\0';
    return text;
}

int profile_load(const char *name, Profile *profile) {
    char path[4096], line[1024];
    if (profile_path(name, path, sizeof(path)) != 0) {
        return -1;
    }
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            report("no profile named '%s' (see --list)", name);
        } else {
            report("cannot read %s: %s", path, strerror(errno));
        }
        return -1;
    }

    // Apply into a copy so a broken file leaves the profile untouched
    Profile loaded = *profile;
    int status = 0;
    for (int number = 1; fgets(line, sizeof(line), file) != NULL; number += 1) {
        char *content = trim(line);
        if (content[0] == '\0' || content[0] == '#' || content[0] == ';') {
            continue;
        }
        char *equal = strchr(content, '=');
        if (equal == NULL) {
            report("%s:%d: expected 'zone = color'", path, number);
            status = -1;
            break;
        }
        *equal = '\0';
        char *key = trim(content);
        char *value = trim(equal + 1);

        unsigned zones = profile_zone_group(key);
        int rgb[3];
        if (zones == 0) {
            report("%s:%d: unknown zone '%s'", path, number, key);
            status = -1;
            break;
        }
        if (parse_color(value, rgb) != 0) {
            report("%s:%d: invalid color '%s'", path, number, value);
            status = -1;
            break;
        }
        profile_set(&loaded, zones, rgb);
    }
    fclose(file);

    if (status == 0) {
        *profile = loaded;
    }
    return status;
}

int profile_delete(const char *name) {
    char path[4096];
    if (profile_path(name, path, sizeof(path)) != 0) {
        return -1;
    }
    if (unlink(path) != 0) {
        if (errno == ENOENT) {
            report("no profile named '%s' (see --list)", name);
        } else {
            report("cannot delete %s: %s", path, strerror(errno));
        }
        return -1;
    }
    return 0;
}

static int is_profile_file(const struct dirent *entry) {
    size_t length = strlen(entry->d_name);
    size_t ext = strlen(PROFILE_EXTENSION);
    return length > ext && strcmp(entry->d_name + length - ext, PROFILE_EXTENSION) == 0;
}

int profile_list(void (*fn)(const char *name, void *data), void *data) {
    char dir[4096];
    struct dirent **entries;
    if (profiles_dir(dir, sizeof(dir)) != 0) {
        return -1;
    }
    int count = scandir(dir, &entries, is_profile_file, alphasort);
    if (count < 0) {
        // No directory yet simply means no profile saved
        return errno == ENOENT ? 0 : -1;
    }

    int listed = 0;
    for (int i = 0; i < count; i += 1) {
        char *name = entries[i]->d_name;
        name[strlen(name) - strlen(PROFILE_EXTENSION)] = '\0';
        if (profile_valid_name(name)) {
            fn(name, data);
            listed += 1;
        }
        free(entries[i]);
    }
    free(entries);
    return listed;
}
