# Interior camera captures

Frames of the kitchen and the noodle shop at the door end, the middle and the far side, saved by `InteriorCheck -- --capture=<folder>` for [ADR 0048](../../adr/0048-follow-the-actors-through-a-room-from-a-wide-view.md); the file name carries the stop's cell column.

- Run: `godot --path project --display-driver x11 --audio-driver Dummy --disable-vsync --fixed-fps 60 --main-loop InteriorCheck -- --capture=<folder>` inside a private `kwin_wayland --virtual --xwayland --width 1920 --height 1080` session, with `NEOCITY_EXPLORATION_SECONDS=18`, `NEOCITY_BASE_SECONDS=0`, `NEOCITY_COOK_SECONDS=0` and a fresh `XDG_DATA_HOME`, with the one-minute load below 6. The window is not the canvas's 1920 by 1080: the frames are 3264 by 1836.
- The Wayland display driver crashed in `DaySave::encode` (signal 11) in this session, on a build of `origin/goal/rules` as well as on this branch, so the capture ran on the X11 driver of that session.
- A room is 192 by 64 scene pixels, so at the street's zoom it covers 384 of 1280 layout units across the screen.
