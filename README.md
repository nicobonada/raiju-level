# raiju-level

Print the battery reading SDL reports for a connected gamepad.

Working personal tool, not a template. The Razer Raiju V3 Pro does not
expose a charge percentage to the kernel. SDL's PlayStation 5 HID driver
does, as a coarse step (about 5%, 15%, ... 100%). This prints that number.

## Run

```fish
nix run ~/src/raiju-level
nix run ~/src/raiju-level -- --icon
```

A Raiju on the HyperSpeed dongle looks like:

```text
Razer Raiju V3 Pro(PS/Wireless) (1532:1026): 100% on-battery
```

`--icon` adds a Material Design battery glyph from Symbols Nerd Font
(the range Kitty already maps). It follows the same coarse steps, so
85% is the 90% glyph. While charging, the glyph is the charging one.

`on-battery`, `charging`, and `charged` are SDL's power states. If SDL
never gets a percentage, the line omits it and the command exits 1.

The pad has to be on. The dongle alone, with the controller off, is not
a joystick.
