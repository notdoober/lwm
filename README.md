# lwm

`lwm` is a small wlroots compositor with a single-file core, static config, and no built-in bar. The target is simple first boot, sane defaults, and a codebase that stays easy to read.

## Features

- xdg-shell windows
- layer-shell support
- optional Xwayland support
- tile, monocle, and floating layouts
- per-output workspaces
- ALT-drag move and resize
- static rules in `config.h`
- no runtime config parser

## Design

- one main source file: `main.c`
- one static config file: `config.h`
- no IPC layer
- no compositor-side bar
- no plugin system
- no runtime daemon split

If you want dynamic config, scripting, animations, or an integrated panel, this is not that project.

## Dependencies

Build requirements:

- `cc`
- `make`
- `pkg-config`
- `wayland-scanner`
- `wayland-protocols`
- `wlroots-0.19`
- `wayland-server`
- `xkbcommon`

Optional build requirement:

- `xwayland` pkg-config target

Optional runtime tools:

- `dbus-run-session`
- `rofi`
- `kitty`, `foot`, `alacritty`, or `xterm`
- `Xwayland`
- `waybar`

## Build

```sh
make
```

`lwm` builds in two modes:

- if `xwayland` development files are present, Xwayland support is compiled in
- if they are not present, `lwm` still builds as a Wayland-only compositor

The generated protocol headers are created during build and removed by `make clean`.

## Install

Minimal local install:

```sh
make
install -Dm755 lwm ~/.local/bin/lwm
```

System-wide install:

```sh
make
sudo install -Dm755 lwm /usr/local/bin/lwm
```

If you want a display-manager session entry, create one yourself after first boot confirmation. For initial testing, start it directly from a TTY.

## First Launch

From a real TTY:

```sh
make
LWM_DBUS=1 ./lwm
```

If Xwayland support was compiled in and you want X11 clients too:

```sh
LWM_DBUS=1 LWM_XWAYLAND=1 ./lwm
```

Nested testing from an existing Wayland session also works, but the real release check should still be done from a clean TTY.

## Default Controls

- `ALT+Q` spawn terminal
- `ALT+Return` spawn terminal
- `ALT+D` run launcher
- `ALT+1..9` switch workspace
- `ALT+Shift+1..9` move focused window to workspace
- `SUPER+1..9` move focused window to workspace
- `ALT+C` close focused window
- `ALT+Space` cycle layout
- `ALT+F` toggle floating
- `SUPER+F` toggle fullscreen
- `ALT+H/L` shrink or grow master area
- `ALT+J/K` focus next or previous
- `ALT+Shift+Q` quit
- `CTRL+ALT+F1..F12` switch VT on native sessions
- hold `ALT` + `LMB` move
- hold `ALT` + `RMB` resize

## Configuration

All user-facing defaults live in `config.h`.

Relevant parts:

- `term_cmd`: terminal command
- `menu_cmd`: launcher command
- `start[]`: autostart commands
- `envs[]`: exported session environment
- `binds[]`: keybindings
- `rules[]`: static window rules
- `mod_main` and `mod_aux`: modifier layout
- focus policy toggles
- tiling ratio and gap values

The default launcher is:

```sh
rofi -show drun
```

If you prefer `wofi` or `bemenu`, change `menu_cmd` and rebuild.

Cursor theming is disabled by default. If you want the compositor to export a cursor theme, enable `set_cursor_theme` in `config.h`.

## Warnings

- This is early software. Test from a spare TTY first.
- `lwm` expects wlroots `0.19` APIs right now.
- X11 clients require both compile-time Xwayland support and `LWM_XWAYLAND=1` at runtime.
- Many desktop components behave better with a D-Bus session. For normal use, prefer `LWM_DBUS=1 ./lwm`.
- The default launcher and terminal commands are only defaults. They are not guaranteed to exist on every system.
- No login-manager files are shipped in-tree.
- No persistence layer is included. What you build is what you run.

## Smoke Tests

```sh
make smoke
make smoke-dbus
make smoke-rofi
make smoke-waybar
make smoke-xwayland
```

What they cover:

- `smoke`: compositor starts and stays up briefly
- `smoke-dbus`: session bus path works
- `smoke-rofi`: layer-shell launcher path works
- `smoke-waybar`: top layer-shell panel path works
- `smoke-xwayland`: Xwayland startup path works

These are sanity checks, not a substitute for a real TTY boot.

## Release Checklist

Before tagging a public release:

1. `make clean && make`
2. Run all smoke targets that apply to your build.
3. Boot from a clean TTY.
4. Confirm terminal spawn, launcher spawn, close, focus, workspace move, fullscreen, and quit.
5. Confirm behavior with and without D-Bus if you intend to document both.
6. Confirm X11 clients only if you are shipping an Xwayland-enabled build.

## Troubleshooting

### Launcher crashes or does not appear

- confirm your launcher exists in `PATH`
- if using `rofi`, keep D-Bus enabled during testing
- run `make smoke-rofi`

### X11 apps do not start

- confirm the build included Xwayland support
- confirm `Xwayland` exists in `PATH`
- launch with `LWM_XWAYLAND=1`
- run `make smoke-xwayland`

### GTK or portal-backed apps behave strangely

- start with `LWM_DBUS=1 ./lwm`

### Cursor theme is wrong

- leave cursor theming disabled in `config.h`, or
- enable `set_cursor_theme` and set your theme explicitly

### Build fails on another machine

- check that `wlroots-0.19`, `wayland-server`, `xkbcommon`, `wayland-scanner`, and `wayland-protocols` are installed
- if only the `xwayland` pkg-config target is missing, `lwm` should still build in Wayland-only mode

## Tutorials

### Change the terminal

Edit `config.h`:

```c
static const char term_cmd[] = "foot";
```

Then rebuild:

```sh
make clean && make
```

### Change the launcher

Edit `config.h`:

```c
static const char menu_cmd[] = "wofi --show drun";
```

Then rebuild:

```sh
make clean && make
```

### Add autostart

Edit `config.h`:

```c
static const char *start[] = {
    "waybar",
    "mako",
    NULL,
};
```

Then rebuild and boot again.

### Enable cursor theme export

Edit `config.h`:

```c
static const bool set_cursor_theme = true;
static const char cursor_theme[] = "Bibata-Modern-Ice";
static const int cursor_size = 24;
```

Then rebuild.

## Source Layout

- `main.c`: compositor core
- `config.h`: static config
- `Makefile`: build logic
- `LICENSE`: project license
- `tools/`: smoke scripts
- `wlr-layer-shell-unstable-v1.xml`: local protocol source

## Philosophy

`lwm` is meant to stay small enough that you can read the whole compositor, change a binding, add a rule, rebuild, and know exactly what changed.

## License

MIT. See `LICENSE`.
