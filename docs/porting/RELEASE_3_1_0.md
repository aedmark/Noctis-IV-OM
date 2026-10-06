# Noctis IV OM 3.1.0 release record

Release: The Deterministic Engine & Reliability Release

Date: 2026-10-06

## Scope

Version 3.1.0 publishes the completed M17 engine-consolidation milestone as a
backward-compatible minor release. It adds deterministic semantic input replay,
explicit application and persistent terminal-state ownership, bounded renderer
memory access, and expanded cross-platform verification. It does not change
native save version 1, accepted legacy imports, universe identity, or accepted
player-visible journey output.

## Official artifacts

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Noctis-IV-OM-linux-x86_64.tar.gz` | 3,064,011 | `92c1e629a6cb44cfbca3d6b529f9672ec8f9d849d62066f0a739219d393282b4` |
| `Noctis-IV-OM-windows-x86_64.zip` | 8,603,584 | `d6c2df0c54bce13f88d10d9688112dbff3dc4dfced89d8dcc0f03218c69233ba` |
| `Noctis-IV-OM-web.zip` | 2,360,157 | `27d9e0fdbfc3801891751b1ddbfa4c3c03116eb7f1ac234708a6358d194b4442` |

Each archive has a colocated `.sha256` file containing the same digest and
basename. The release staging directory is `public-builds/3.1.0/`; it remains
ignored by Git so generated binaries do not enter source history.

## Quality gates

- Linux Clang Release: 57/57 tests passed in 6.91 seconds.
- Linux Clang Debug: 57/57 tests passed in 29.61 seconds.
- Linux GCC Debug: 57/57 tests passed in 27.10 seconds.
- Clang ASan/UBSan: 57/57 tests passed in 93.41 seconds with leak detection
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
  README, project licenses, contributors, and third-party notice; archive and
  checksum validation pass.

## Website and browser build

The root website and dev diary identify v3.1.0 and link all desktop archives,
checksums, the self-host Web ZIP, and the v3.1.0 release page. Desktop and
390×844 responsive visual checks pass. The locally assembled `play/` payload
loads `nivlr.js`, `nivlr.wasm`, and `nivlr.data`, reaches the Noctis launch
screen, and reports no browser console errors.

The `play/` directory is generated and ignored locally. GitHub Pages rebuilds
the same payload from the release commit through `.github/workflows/web.yml`.
