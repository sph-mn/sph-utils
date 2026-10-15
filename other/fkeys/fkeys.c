#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>

#define fkeys_key_count 10
#define fkeys_modifier_count 8
#define fkeys_grab_count 4

char *fkeys_commands[] = {
#if __has_include("config.h")
#include "config.h"
#else
#endif
};

int main(void)
{
    Display *display;
    Window root;
    XModifierKeymap *mapping;
    KeyCode numlock;
    KeyCode key;
    unsigned int mask;
    int32_t modifier;
    int32_t index;
    int32_t key_number;
    XEvent event;

    display = XOpenDisplay(0);
    root = DefaultRootWindow(display);
    mapping = XGetModifierMapping(display);
    numlock = XKeysymToKeycode(display, XK_Num_Lock);
    mask = 0;

    for (modifier = 0; modifier < fkeys_modifier_count; modifier += 1) {
        for (index = 0; index < mapping->max_keypermod; index += 1) {
            if (*(mapping->modifiermap + modifier * mapping->max_keypermod + index) == numlock) {
                mask |= 1U << modifier;
            }
        }
    }

    XFreeModifiermap(mapping);

    for (key_number = 1; key_number <= fkeys_key_count; key_number += 1) {
        if (fkeys_commands[key_number]) {
            key = XKeysymToKeycode(display, XK_F1 + key_number - 1);

            for (index = 0; index < fkeys_grab_count; index += 1) {
                XGrabKey(
                    display,
                    key,
                    (index & 1 ? LockMask : 0) | (index & 2 ? mask : 0),
                    root,
                    False,
                    GrabModeAsync,
                    GrabModeAsync
                );
            }
        }
    }

    signal(SIGCHLD, SIG_IGN);

    while (1) {
        XNextEvent(display, &event);

        if (event.type == KeyPress) {
            key_number = XLookupKeysym(&event.xkey, 0) - XK_F1 + 1;

            if (key_number >= 1 && key_number <= fkeys_key_count) {
                if (fkeys_commands[key_number]) {
                    if (!fork()) {
                        close(ConnectionNumber(display));
                        signal(SIGCHLD, SIG_DFL);
                        execlp(fkeys_commands[key_number], fkeys_commands[key_number], (char *)0);
                        _exit(127);
                    }
                }
            }
        }
    }
}