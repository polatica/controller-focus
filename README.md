# Controller Focus

Made by polatica.

Geode mod for Geometry Dash (Windows). Stops the game from reading your controller while it's not the active window, so you can play something else without GD reacting in the background.

## Install

Needs Geometry Dash 2.2081 and Geode 5.8.2 (5.x should work).

1. Close GD
2. Put `polatica.controller_focus.geode` in `Geometry Dash/geode/mods/`
3. Remove any previous Controller Focus / Focus Guard package before installing this edition, then start the game. There are no settings.

Everything is blocked while GD is in the background: face buttons, Start/Back, bumpers, triggers, D-pad, both sticks and the mouse movement/clicks GD generates from the controller. Other games still get the controller normally.

When you come back to GD, let go of all buttons and triggers and center the sticks once, then the controller works again. Without this, a button you were holding in the other game would trigger something in GD as soon as you switch back. Slight stick drift is fine (it uses Geode's deadzone). Each controller is handled separately.

It only covers GD's own controller handling. Tools that turn the controller into keyboard/mouse input, or mods that read the controller on their own, are not affected by this mod.

## Build

You need the Geode SDK + CLI, CMake 3.25+ and Visual Studio with C++23 support. `GEODE_SDK` has to point to the SDK folder.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The `.geode` file ends up in `build/`, it isn't installed for you.

To only build the tests (no SDK needed):

```powershell
cmake -S . -B build-tests -DFOCUS_GUARD_TESTS_ONLY=ON
cmake --build build-tests --config Release
ctest --test-dir build-tests -C Release --output-on-failure
```

## Testing in game

The tests only cover the focus/rearm logic (`FocusGate`), not the hooks. To check it for real:

- GD focused: menus, gameplay, Start/Back, sticks all work
- Alt-Tab to another app, press everything: GD shouldn't react and shouldn't move or click the Windows cursor
- Come back while holding a button: nothing should happen until you let go
- Same with GD minimized, with a disconnect/reconnect, and with two controllers
- Check it next to your other input mods
