# Interior camera captures

Frames of the kitchen and the noodle shop at the door end, the middle and the far side, saved by `InteriorCheck -- --capture=<folder>` for [ADR 0048](../../adr/0048-follow-the-actors-through-a-room-from-a-wide-view.md); the file name carries the stop's cell column, and `kitchen-climb.png` is the frame at the top of the ladder, on arrival.

- Run: `godot --path project --display-driver x11 --audio-driver Dummy --disable-vsync --fixed-fps 60 --main-loop InteriorCheck -- --capture=<folder>` inside a private `kwin_wayland --virtual --xwayland --width 1920 --height 1080` session, with `NEOCITY_EXPLORATION_SECONDS=18`, `NEOCITY_BASE_SECONDS=0`, `NEOCITY_COOK_SECONDS=0` and a fresh `XDG_DATA_HOME`, with the one-minute load below 6. The window is not the canvas's 1920 by 1080: the frames are 3264 by 1836.
- The Wayland display driver crashed in `DaySave::encode` (signal 11) in this session, on a build of `origin/goal/rules` as well as on this branch, so the capture ran on the X11 driver of that session.
- A room is 192 by 64 scene pixels; at the zoom of 8 it fills 1536 by 512 of the 1280 by 720 layout units, so it scrolls, and the actors are drawn at the street's size on the screen.

## Split route

`split-1-chef-in-appraiser-out.png`, `split-2-mid-ease.png` and `split-3-both-in.png` are the kitchen with one human in and the other on the street, the same two a few frames into the second one's entry, and both inside, saved by `InteriorCheck -- --capture-split=<folder>` (the split route alone, a frame every third during the entry) for the camera fix in the pull request that adds them. The street stays while one human is outside; the entry eases for about 0.28 s; inside, the room fills the width with both humans at the street's size on the screen. The frames are 1920 by 1080 through `tools/private-display.sh` on the X11 driver, one-minute load below 6 at the start. No strip of checkered or transparent texture shows in any of them, nor in the other entry frames of that run: the earlier ones came from street frames drawn at zoom 4 and 8 with the actors at street size, which the camera no longer draws.
