# M17 engine consolidation exit audit

Status: PASSED

Audit date: 2026-10-06

## Disposition

All seven M17 work items and all six milestone exit criteria are complete. The
milestone changed ownership and test boundaries without changing universe
identity, save schemas, fixed-tick behavior, accepted indexed-frame output, or
player-visible journey results. No M17 item is deferred and no release blocker
was found.

## Exit criteria

| Criterion | Result | Evidence |
| --- | --- | --- |
| New engine state has one explicit owner and no new cross-file mutable globals | PASS | `EngineState` owns travel, application mode, and persistent GOES terminal state. W07 removed the command/cursor/scroll globals; native save fields exist only as boundary values. `engine_state` verifies defaults, mutation, adapters, and reset. |
| Platform filesystem, clock, presentation, audio, and process services are replaceable in headless tests | PASS | Runtime paths and export destinations are caller-selected; universe time has a fixture override and simulation uses fixed ticks; indexed-frame expansion and replay cadence run without a window; audio has an offline renderer and safe uninitialized path; external video process command construction and empty-deck handling are testable without spawning a process. Focused runtime-path, clock, display/framebuffer, audio, gallery, and video tests pass. |
| Application modes transition through a testable state machine | PASS | `ApplicationRuntimeState` rejects illegal transitions and makes shutdown terminal. Gallery/movie lifecycles and the cockpit → descent → surface → ascent → cockpit production journey use the state machine; unit and landing fixtures verify exact transitions. |
| A recorded journey replays identically at multiple presentation cadences | PASS | Versioned semantic input recording rejects missed/out-of-order frames. The scripted journey and 783-frame orbit-to-surface recording replay at 1× and 4× presentation cadence with identical simulation, indexed-frame, and content checkpoints. |
| Renderer overrun padding is replaced by bounded operations with sanitizer and exact-fixture evidence | PASS | Texture sources carry capacities, textured scanlines use active-page bounds, `adapted` is exactly 1280×800, and `p_surfacemap` exactly 200×200. Sentinel tests, exact renderer/surface fixtures, and the full sanitizer lane pass. |
| Linux Clang/GCC, sanitizers, Windows, and Web retain supported gates | PASS | Linux Clang Debug 57/57 in 29.61 s; Linux GCC Debug 57/57 in 27.10 s; Clang ASan/UBSan 57/57 in 93.41 s with LeakSanitizer disabled only for the desktop runner's ptrace policy; MinGW Windows Release and Emscripten Web Release compile and link. |

## Compatibility closure

- Native situation v1 remains exactly 401 bytes and native surface v1 remains
  exactly 65 bytes.
- Every accepted 245/370/377/378/379/380/381/382-byte legacy situation path
  normalizes identically after the first persistent aggregate migration.
- Canonical galaxy, system, star, surface, environment, content, renderer,
  landing, scripted-journey, and orbit/surface hashes and counters are
  unchanged.
- The fixed 55 ms simulation tick remains authoritative; presentation cadence
  does not alter recorded input delivery or simulation results.
- Linux, Windows, and Web continue to share the same simulation and persistence
  implementation.

## Release recommendation

M17 is suitable for release as a compatibility-preserving engine reliability
update. Package release notes should describe deterministic replay, stronger
runtime state ownership, bounded renderer memory, and unchanged native/legacy
save compatibility without presenting internal refactoring as new gameplay.
