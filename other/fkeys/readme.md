# fkeys
Minimal X11 hotkey daemon written in C. Binds function keys F1-F10 to executable commands.

Uses Xlib directly, without a runtime configuration parser, shell invocation, or root permissions.
## Requirements
* X11
* C17 compiler
* libX11 development headers

## Build
```
./compile.sh
```
The executable is created at `exe/compiled/fkeys`.
## Configuration
Bindings are defined at compile time. Create an optional `config.h` alongside `fkeys.c`:
```
[1] = "firefox",
[2] = "thunar",
[3] = "start-terminal",
[5] = "dmenu_run",
```
Each index corresponds to a function key. Omitted keys remain unbound.

Without `config.h`, the built-in defaults are used.

Recompile after changing the configuration.
## Usage
```
exe/compiled/fkeys
```
Start the daemon from your X11 session initialization.

Commands must be executable files accessible through `PATH`. Shell aliases, arguments, and shell expressions are not supported.

Caps Lock and Num Lock are ignored when matching bindings.
## Limitations
* X11 only; Wayland is unsupported.
* Keys already grabbed by another application cannot be registered.
* Keyboard mapping changes require restarting the daemon.
* Holding a key may launch its command repeatedly.

Stop other hotkey daemons, such as xbindkeys, before starting fkeys.
