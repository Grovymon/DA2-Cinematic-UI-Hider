# DA2 Cinematic UI Hider

[Русская инструкция](README_RU.md)

Toggle the Dragon Age II interface by holding a configurable hotkey, allowing clean screenshots during conversations, cutscenes and gameplay.

> [!IMPORTANT]
> v0.1.0 is a native-toggle verification release. The ASI loading layout and hold state machine are built and automatically tested, but Dragon Age II was not installed on the development machine. The exact visual coverage of DA2's built-in **Hide main interface** action—especially subtitles, dialogue text, response wheel and cinematic overlays—**requires in-game verification**. See [In-game test checklist](docs/IN-GAME-TEST-CHECKLIST.md).

## Features

- Native 32-bit ASI mod loaded inside `DragonAge2.exe`; no separate helper program.
- Configurable toggle key and hold duration.
- Exactly one toggle per physical hold; keyboard autorepeat cannot retrigger it.
- Releasing the key never sends a restore command.
- Automatically reads `HideMainInterface_0` / `_1` from DA2's `KeyBindings.ini` when possible.
- Configurable fallback for DA2's native interface key (`V` by default).
- F11 emergency restore based on the mod's tracked state.
- Alt+Tab-safe: focus loss resets the physical hold without changing UI state.
- Optional transition-only debug log; no per-frame spam.
- No game memory addresses, DirectX hooks, game assets or executable patches.

## How It Works

The [official Dragon Age II PC manual](https://eaassets-a.akamaihd.net/eahelp/manuals/dragon-age-ii-manuals_PC.pdf) lists **Hide main interface**, bound to `V` by default. This mod uses that existing game action instead of inventing an undocumented DA2 API.

Ultimate ASI Loader loads `DA2CinematicUIHider.asi` into the 32-bit game process. The mod observes the configured physical key only while DA2 is focused. After the continuous hold reaches `HoldDurationMs`, it sends one scan-code key stroke for DA2's currently configured `HideMainInterface` action.

```text
UI visible
Hold F12 for 300 ms  -> one native DA2 toggle -> UI hidden
Release F12          -> no action             -> UI remains hidden
Hold F12 again       -> one native DA2 toggle -> UI visible
Release F12          -> no action             -> UI remains visible
```

Keeping F12 down for five seconds still produces only one toggle. A release is required before the next one.

## Installation

1. Download `DA2-Cinematic-UI-Hider-v0.1.0.zip` from the [v0.1.0 Release](https://github.com/Grovymon/DA2-Cinematic-UI-Hider/releases/tag/v0.1.0).
2. Find the Dragon Age II installation folder—the folder that contains `bin_ship`.
3. Extract the archive into that Dragon Age II folder and merge `bin_ship` when Windows asks.
4. Confirm the final files are next to `DragonAge2.exe`:

   ```text
   Dragon Age II\
   └── bin_ship\
       ├── DragonAge2.exe
       ├── dinput8.dll
       ├── dinput8.ini
       ├── DA2CinematicUIHider.asi
       └── DA2CinematicUIHider.ini
   ```

5. Start Dragon Age II normally through Steam or EA App.

If `bin_ship` already contains `dinput8.dll`, it may be another ASI Loader. Do not overwrite it blindly. A current Ultimate ASI Loader can load multiple `.asi` files; copy only this mod's `.asi` and `.ini` beside the existing loader, or compare the loader first. The bundled loader is Ultimate ASI Loader v9.7.4 and its MIT license is included.

## Usage

Default controls:

```text
Hold F12        -> Hide UI
Release F12     -> UI remains hidden
Hold F12 again  -> Restore UI
Release F12     -> UI remains visible
Press F11       -> Emergency restore attempt if tracked state is hidden
```

F12 is Steam's default screenshot key. The mod does not suppress it, so Steam may capture a screenshot at the start of the hold. Set `ToggleKey=F10` or remap Steam's screenshot key if that is unwanted.

## Configuration

Edit `bin_ship\DA2CinematicUIHider.ini` while the game is closed:

```ini
[General]
ToggleKey=F12
HoldDurationMs=300
AutoDetectHideInterfaceKey=true
HideInterfaceKey=V
EmergencyRestoreKey=F11
KeyBindingsPath=Auto
OnlyWhenGameFocused=true
PollIntervalMs=10
Debug=false
```

| Setting | Meaning |
| --- | --- |
| `ToggleKey` | Physical key to hold. `F10` is a useful alternative to F12. |
| `HoldDurationMs` | Required continuous hold, from 50 to 10000 ms. |
| `AutoDetectHideInterfaceKey` | Reads DA2's actual native binding when available. |
| `HideInterfaceKey` | Fallback native DA2 key or chord. Default: `V`. |
| `EmergencyRestoreKey` | Edge-triggered recovery key. Default: `F11`. |
| `KeyBindingsPath` | `Auto` uses Windows' Documents folder; an explicit path is also accepted. |
| `OnlyWhenGameFocused` | Prevents input from being sent after Alt+Tab. Keep this `true`. |
| `PollIntervalMs` | Physical state polling interval, from 5 to 100 ms. |
| `Debug` | Writes `bin_ship\DA2CinematicUIHider.log`. |

Letters, digits, `F1`–`F24`, navigation keys, `PrintScreen`, common numpad keys and `Ctrl`/`Alt`/`Shift` chords are parsed. Prefer a toggle key without modifiers because physically held modifiers can affect the native DA2 key being sent.

## Compatibility

- Dragon Age II on Windows PC; `DragonAge2.exe` is a 32-bit process.
- Steam and EA App releases are the intended targets.
- The mod is compiled as PE32 x86 and uses the same Ultimate ASI Loader installation pattern as existing DA1/DA2 ASI fixes.
- The default binding file is resolved through Windows Documents, then `BioWare\Dragon Age 2\Settings\KeyBindings.ini`.
- No game files or saves are modified.

## Known Issues

- **Requires in-game verification:** normal HUD, party panels, subtitles, dialogue text, dialogue wheel, response options, letterboxes and other cinematic overlays have not been visually tested with v0.1.0.
- DA2 exposes a toggle action, not a documented `ShowUI` command or readable public UI-state API. The mod tracks only commands that it successfully sends.
- Manually pressing DA2's native hide key, loading the mod while UI is already hidden, or a game transition that changes visibility can desynchronize tracked state.
- F11 therefore avoids a blind toggle when the tracked state is already visible. It cannot mathematically guarantee `ON` when the real state is unreadable.
- The mod does not suppress Steam's F12 screenshot action.
- If DA2's built-in command leaves dialogue or cinematic elements visible, v0.1.0 does **not** pass the project's clean-conversation acceptance criterion. Record the failed item before adding a version-specific Scaleform/GFx or engine hook.

## Troubleshooting

- **No log and no response:** set `Debug=true`. If the log still does not appear, verify that `dinput8.dll`, `.asi` and `.ini` are next to `DragonAge2.exe` in `bin_ship`.
- **The log says configuration is invalid:** restore the release INI and change one value at a time.
- **The wrong DA2 key is sent:** keep auto-detection enabled or set `HideInterfaceKey` to the same binding shown in DA2 controls.
- **Steam takes an unwanted screenshot:** use `ToggleKey=F10` or remap Steam.
- **Returning from Alt+Tab does not toggle while F12 is still held:** release it and start a new hold. This is intentional.
- **Tracked state is wrong:** make the real DA2 UI visible with its native binding, restart the game, then use only the mod's toggle key.
- **Another ASI mod is installed:** use one compatible `dinput8.dll` loader and keep all `.asi` files beside it. Do not install two proxy DLLs with the same name.

## Uninstallation

Delete these two files from `bin_ship`:

```text
DA2CinematicUIHider.asi
DA2CinematicUIHider.ini
```

Delete `dinput8.dll` and `dinput8.ini` only if no other ASI mod needs that loader.

## Building

### Visual Studio / CMake

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Portable Zig build

With Zig 0.16+ on `PATH`:

```powershell
.\scripts\test.ps1
.\scripts\build.ps1
.\scripts\smoke-test.ps1
.\scripts\package.ps1
```

The packaging script downloads the official Ultimate ASI Loader v9.7.4 archive and refuses to use it unless its SHA-256 is `952cebfc30d525afc2bdbaca954329d405ded3aa688a83027354dae14dfd5c5f`.

## Credits

- BioWare and Electronic Arts for Dragon Age II and its native **Hide main interface** action.
- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) by ThirteenAG, bundled under the MIT License.
- [DAFix](https://github.com/Lyall/DAFix) by Lyall as evidence of the working ASI loading layout for Dragon Age II. No DAFix source code is included.
- Inspired by the workflow of *Cleaner Screenshots from Conversations* for Dragon Age: Origins. No code or game assets from it are included.

This project is not affiliated with or endorsed by BioWare or Electronic Arts.

## License

The mod source is MIT licensed. See [LICENSE](LICENSE). Third-party notices are in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

---

## Community / Сообщество

💬 **[Join the Discord server / Присоединиться к Discord-серверу](https://discord.gg/gUPsQCnTeN)**
