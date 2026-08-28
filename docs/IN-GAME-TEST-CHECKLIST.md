# In-game verification checklist

Version under test: v0.1.0

Record the DA2 storefront, patch, display mode, resolution, UI language, utility configuration, and whether Steam/EA overlays are enabled. Attach before/after screenshots for each visual test.

## Gameplay

- [ ] Start with the UI visible.
- [ ] Hold the toggle key for at least `HoldDurationMs`; HUD, party panels, health, abilities, minimap and prompts disappear.
- [ ] Release the key; UI remains hidden.
- [ ] Hold again; UI returns.
- [ ] Release again; UI remains visible.

## Dialogue

- [ ] Begin a conversation with subtitles enabled and wait for the dialogue wheel.
- [ ] Hold the toggle key; normal HUD disappears.
- [ ] Subtitles and speaker text disappear.
- [ ] Dialogue wheel, tone icons and every response option disappear.
- [ ] No other visible cinematic/dialogue overlay remains.
- [ ] Release the key; the clean view remains.
- [ ] Hold again; all required interaction UI returns and the conversation can continue.

## Cutscene

- [ ] During a full cutscene, hold the toggle key.
- [ ] Subtitles and every cinematic overlay disappear.
- [ ] Release; the clean view remains.
- [ ] Hold again; expected UI returns.

## Input semantics

- [ ] With `HoldDurationMs=300`, a tap around 50 ms does nothing.
- [ ] Holding for 5 seconds toggles exactly once.
- [ ] A second physical hold toggles exactly once in the opposite direction.
- [ ] Alt+Tab during a partial hold causes no toggle.
- [ ] Returning while the key is still down causes no toggle until release and a new hold.
- [ ] F11 restores the UI when the utility's tracked state is hidden.

## Transition and recovery

- [ ] Hide the UI in gameplay, then enter a conversation; it remains hidden.
- [ ] Hide it during one conversation, then enter another; it remains hidden.
- [ ] Hide it before a cutscene; it remains hidden where technically supported.
- [ ] Manually changing DA2's native UI state demonstrates the documented tracking limitation without crashing or hanging the utility.

If any dialogue or cutscene element remains visible, v0.1.0 has **not** passed the clean-screenshot acceptance criterion. Record the element, scene, screenshot, and whether pressing DA2's native bind directly produces the same result.
