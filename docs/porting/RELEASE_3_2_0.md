# Noctis IV OM 3.2.0 release record

Release: The Browser Moviemaker & WebM Release

Date: 2026-10-06

## Scope

Version 3.2.0 completes the browser Moviemaker workflow. A completed numbered
BMP deck can now be exported directly to WebM from the projector, F3 panel, or
GOESnet without beginning a second live recording. The encoder reads the clean
deck into an offscreen canvas, preserving its native frame dimensions and
excluding browser chrome, unused page area, and the native capture indicator.

The F3 panel now exposes playback and export controls. Ctrl-plus and Ctrl-minus,
including numpad variants, select movie decks while the web game is active
instead of changing browser zoom. Native capture remains compatible, with its
capture indicator correctly positioned at 1x, 2x, and 4x internal resolutions.

## Official artifacts

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Noctis-IV-OM-linux-x86_64.tar.gz` | 3,064,267 | `263ce4addf080b144ef46757f76e936aa7b5b9f8f121f548b5b12b27cc9e6444` |
| `Noctis-IV-OM-windows-x86_64.zip` | 8,603,767 | `e3ef77851615d9b1a461f6df9b84d5b792745dd2b76e89eac232a05f7a13a2e7` |
| `Noctis-IV-OM-web.zip` | 2,360,686 | `1e956ce9756da41dce6ac940a31ae2fb5a48aa6330ce563bdf718db9f97bcb72` |

Each archive has a colocated `.sha256` file containing the same digest and
basename. The release staging directory is `public-builds/3.2.0/`; it remains
ignored by Git so generated binaries do not enter source history.

## Quality gates

- Linux Clang Release: 57/57 tests passed in 7.46 seconds.
- Linux GCC Debug: 57/57 tests passed in 27.84 seconds.
- Clang ASan/UBSan: 57/57 tests passed in 94.28 seconds with leak detection
  disabled only because the desktop runner's ptrace policy prevents
  LeakSanitizer operation.
- MinGW Windows Release and Emscripten Web Release compile and link.
- CPack Linux and Windows archives pass exact content validation, extracted
  diagnostics, isolated player-profile preparation, and checksum validation.
- The extracted Linux archive passes the three-frame graphical smoke test on
  the desktop Wayland/OpenGL/audio stack.
- The Windows archive passes extracted diagnostics and profile preparation
  under Wine.
- The Web ZIP contains one `Noctis-IV-OM-web/` root with the four runtime files,
  README, project licenses, contributors, and third-party notice.

## Moviemaker and browser evidence

- The reported malformed recording was inspected as a 792x985 VP9 stream. Its
  white lower region came from recording the whole viewport while only the
  native presentation region contained movie pixels.
- Browser export now encodes the recorded BMP deck through an offscreen canvas,
  independent of page layout and capture-indicator scaling.
- A browser-generated test export was downloaded and decoded successfully as a
  playable VP9 WebM at the deck's native 640x400 resolution.
- Regression coverage verifies the F3 control legend, browser export routing,
  capture-indicator placement, and Moviemaker input behavior.

## Website and browser build

The root website and dev diary identify v3.2.0 and link all desktop archives,
checksums, the self-host Web ZIP, and the v3.2.0 release page. Desktop and mobile
responsive visual checks cover the release presentation and download cards.
The locally assembled `play/` payload loads `nivlr.js`, `nivlr.wasm`, and
`nivlr.data`, reaches the Noctis launch screen, and reserves Ctrl-plus/minus for
the running game.

The `play/` directory is generated and ignored locally. GitHub Pages rebuilds
the same payload from the release commit through `.github/workflows/web.yml`.
