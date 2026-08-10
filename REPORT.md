# Port report: Telidon JS → ofxNaplps

Port of the two Telidon JavaScript libraries to this openFrameworks addon,
verified against the originals.

| Source | Destination |
| --- | --- |
| `Telidon/js/telidon/naplps.js` | `libs/ofxNaplps/src/Naplps.cpp` + `include/ofxNaplps/Naplps.h` |
| `Telidon/js/telidon/TelidonP5.js` | `libs/ofxNaplps/src/Telidon.cpp` + `include/ofxNaplps/Telidon.h` |

## What was ported

**`Naplps.cpp`** — all of `naplps.js`:

* binary/hex utilities, `NapChar` / `NapOpcode` / `NapData`
* `NapDataArray` / `NapVector` / `NapText`
* `NapCmd`, with the full PDI dispatch: points, lines, arcs, rectangles,
  polygons, incrementals, color, domain, reset, NSR, text
* `NapDecoder` (aliased as `Naplps`)
* the encoder half: `NapInputWrapper`, `NapEncoder`

**`Telidon.cpp`** — all of `TelidonP5.js`: `TelidonDrawCmd` and `TelidonDraw`
(aliased as `Telidon`), including progressive drawing.

**`example/`** — `ofApp` decodes `bin/data/shark.nap` with the `Naplps` object
and draws it with the `Telidon` object. Arrow keys cycle the other sample
files, space redraws, and `.nap` files can be dropped on the window. The
standard openFrameworks `Makefile` was added; the folder had none.

## Verification

**Decoder.** Every command, color, and point was dumped from both
implementations for all six sample `.nap` files and diffed. Byte-identical.
The only deltas are ±1e-5, from `printf` rounding exact `.5` ties differently
than JavaScript's `toFixed`.

| File | Lines compared | Real differences |
| --- | --- | --- |
| shark | 1441 | 0 |
| santa | 1418 | 0 |
| beer | 907 | 0 |
| haunt | 667 | 0 |
| wast | 2905 | 0 |
| email2 | 2130 | 0 |

**Encoder.** The same synthetic strokes were run through both
implementations at 3-byte, 4-byte, and normalized settings. Output identical
byte for byte.

**Renderer.** The example was built against openFrameworks 0.12.1 and used to
render `shark.nap`; the same file was rendered with p5.js in a headless
browser for comparison.

* 98.08% of pixels identical
* 99.22% within 8 levels
* 0.02% differing by more than 64 — edge antialiasing, canvas vs. OpenGL
  tessellation

The orange shape over the shark's head appears in both renders. It is an
artifact of the original decoder, faithfully reproduced.

**Build.** All four sources compile under `-Wall` with no warnings, and the
addon's sources under `libs/ofxNaplps/src` are picked up by the openFrameworks
build system without needing `ADDON_SOURCES` in `addon_config.mk`.

## Deliberate departures from the JavaScript

All of these are commented at the point where they occur.

* The module-level globals in `naplps.js` (`naplps_drawingCursor`,
  `naplps_colorMap`, and the rest) became `NapState`, owned by the decoder
  that is parsing, so two files can be decoded without stepping on each other.
* p5's global fill/stroke state became `TelidonRenderState`, shared by every
  command in a drawing. NAPLPS depends on that state carrying over between
  commands, so it had to stay shared rather than becoming per-command.
* JavaScript exceptions used as control flow became explicit bounds checks
  with the same outcomes. This includes `sendReset`, where the JS version dies
  on an undefined `screenstuff` variable and abandons the rest of the reset.
* A negative shift in `setColor`, reachable with color operands longer than
  four bytes, is clamped. Shifting by a negative amount is undefined behavior
  in C++.
* The scanline fields (`moveScanline`, `scanPos`, `scanDelta`) are gone. They
  were permanently switched off and their pixel loop was commented out.
* `finished` tracks the point index rather than the point count, so a command
  holding an off-screen point can still report itself finished.
* `update()` and `draw()` are separate, the way an `ofApp` expects. In the JS
  version, `run()` did both at once for each command in turn.
* Colors are `ofColor` rather than a 0-255 `Vector3`, and points are
  `glm::vec2`.
