# M6-W06 native Moviemaker

Status: complete on Linux under ADR-0013. This closes the final M6
implementation package before Windows-specific work.

## Pinned behavior

The final NIV+ source and `manual/moviemaking.html` define Moviemaker as a BMP
image-sequence recorder available in space and on planetary surfaces.

| Control | Behavior |
| --- | --- |
| F3 | Toggle the Moviemaker setup panel |
| Ctrl-plus / Ctrl-minus | Select numbered deck 001–999 while stopped |
| Plus / minus | Capture every 1–999 simulation frames while stopped |
| `f` | Toggle black-flash versus unobtrusive capture indication |
| Enter | Start from the open panel, or stop an active recording |
| `p` | Pause or resume an active recording |
| `v` / Space | Open the selected recorded deck in the player |
| `x` | Export the selected deck to MP4 on desktop or WebM in the browser |

The F3 panel also displays the player controls: Space or `p` toggles playback,
Left/Right steps one frame, `l` toggles looping, and Escape closes the player.

Recording produces `movies/DDD/FFFFFFFF.BMP`. The frame number restarts at one
for a new deck, recording may continue through landing, and a surface recording
automatically stops after 100 gameplay frames of capsule ascent so recovery can
finish. The displayed recording rate describes captured frames per simulation
second. Moviemaker settings reset on application startup in pinned NIV+ and do
not require a save-schema change.

## Native implementation

1. The tested `movie_capture` component owns bounded mode, deck,
   cadence, frame number, elapsed recording ticks, pause state, deck occupancy,
   and ascent cutoff transitions.
2. Semantic input includes F3 and an explicit Control modifier. It preserves the
   DOS extended-key ordering without inferring modifiers from printable text.
3. Deck paths use `std::filesystem`. Existing deck content is never
   removed or overwritten; an occupied deck blocks recording until the player
   selects a free one.
4. The production indexed-frame BMP writer is shared while movie output stays
   separate from the persisted ordinary-snapshot sequence. Capture cadence is
   driven by simulation frames, not rendering speed or wall time.
5. One shared setup model renders in the cockpit and surface visor, with the
   compact cockpit status messages retained where the full panel cannot appear.
6. Recording stays alive across the normal landing transition. Enter stops it,
   advances to the next deck candidate, and enforces the pinned ascent
   cutoff. Pause neither emits frames nor advances recording elapsed time.
7. Browser recording suppresses the post-capture flash and produces its WebM
   from the completed clean BMP deck rather than recording the viewport canvas.
   Explicit browser deck export uses the same path; it does not ask the player
   to start a second recording.
8. The web shell cancels the browser's Ctrl-plus and Ctrl-minus defaults while
   the game is active, including numpad variants, so those keys reach deck
   selection without changing page zoom.

The game will not invoke or bundle ffmpeg. Converting BMP sequences to a video
remains an optional external workflow.

## Acceptance evidence

- `movie_capture` covers lower and upper bounds, cadence, pause/resume, normal
  stop and deck advance, occupied-deck refusal, write failure, and the exact
  100-frame ascent cutoff.
- `input_mapping` covers F3 and Ctrl-plus/minus without duplicate printable
  events.
- `movie_application` drives the production application without a window. It
  verifies the menu framebuffer, exact BMP names and file hashes, three-frame
  space cadence, frozen pause time, both flash modes, occupied-deck
  preservation, surface continuity, and 34 captured ascent frames before the
  100-tick cutoff.
- The complete 38-test suite passes with Clang, GCC, and sanitized Clang,
  retaining ordinary snapshot/panorama, presentation, journey, and persistence
  coverage.

The historical output frames are behavioral guidance, not a claim of
DOS-pixel identity. Native frame hashes will be exact regression baselines; the
control and lifecycle semantics are the compatibility contract.
