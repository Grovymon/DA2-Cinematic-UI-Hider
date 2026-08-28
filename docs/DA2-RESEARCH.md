# Dragon Age II interface research

Research date: 2026-08-27

## Confirmed

| Question | Finding | Evidence |
| --- | --- | --- |
| Does DA2 have a native hide command? | Yes. The PC manual lists `Hide main interface`. | [Official Dragon Age II PC manual](https://eaassets-a.akamaihd.net/eahelp/manuals/dragon-age-ii-manuals_PC.pdf) |
| What is the default key? | `V`. | [Official Dragon Age II PC manual](https://eaassets-a.akamaihd.net/eahelp/manuals/dragon-age-ii-manuals_PC.pdf) |
| Are PC bindings configurable? | Yes; DA2 stores keyboard assignments in `Documents\BioWare\Dragon Age 2\Settings\KeyBindings.ini`. | [EA Forums support reply identifying the DA2 settings file](https://forums.ea.com/discussions/dragon-age-franchise-discussion-en/how-to-mod-dragon-age-2-on-origin/7298058/replies/7298068) |
| How is the mod loaded? | A 32-bit ASI is loaded into `DragonAge2.exe` through Ultimate ASI Loader. The same `bin_ship` pattern is used by DAFix for DA2. | [DAFix source and releases](https://github.com/Lyall/DAFix) |
| Does this project patch game memory? | No. The ASI reads the configured binding and sends one normal scan-code key stroke. It contains no game addresses or memory patches. | Project source code. |

## Not confirmed without the game

The build system did not have Dragon Age II installed. The following are deliberately recorded as unknown, not inferred from the command's name:

| Scenario or element | Status |
| --- | --- |
| Normal gameplay HUD, party panels, health, abilities, minimap | Requires in-game verification |
| Dialogue subtitles and speaker text | Requires in-game verification |
| Dialogue wheel and response options | Requires in-game verification |
| Cinematic UI and letterbox overlays | Requires in-game verification |
| Full cutscenes | Requires in-game verification |
| Persistence across area/dialogue/cutscene transitions | Requires in-game verification |
| Actual UI visibility state readable through a public API | No documented API found; requires reverse engineering before any claim |

## Implementation decision

Version 0.1.0 uses the lowest-risk in-process mod path:

1. Read `HideMainInterface_0` / `HideMainInterface_1` when available.
2. Fall back to the configured `HideInterfaceKey` (`V` by default).
3. Let Ultimate ASI Loader load the x86 mod inside `DragonAge2.exe`.
4. Send one key stroke after a continuous physical hold reaches the threshold.
5. Never send a command on release.
6. Refuse to carry an in-progress hold across focus loss.

No UI-resource override, DirectX hook, or memory address is included because the native action must be tested first. The ASI loader performs normal DLL loading, while the mod itself does not patch the game. If the in-game checklist shows remaining dialogue UI, record exactly which elements remain before designing a second-stage DA2-specific modification.

## State synchronization boundary

The native action is a toggle. The utility can know which toggles it sent, but cannot prove the game's current visual state. The emergency key therefore restores only from the utility's tracked hidden state. This avoids blindly turning a visible UI off, but it cannot repair every possible external desynchronization.
