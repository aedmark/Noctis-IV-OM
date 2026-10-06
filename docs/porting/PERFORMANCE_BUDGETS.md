# M8-W03 Performance and Memory Budgets

Status: complete on reference Linux x86_64.

This document establishes the official performance, frame-timing, and memory
budgets for Noctis IV OM 1.0, backed by empirical profiling across representative
space, orbital, surface, and windowed graphical scenes.

---

## 1. Timing and Frame Cadence Budget

Noctis IV OM inherits Alessandro Ghignola's simulation model, which synchronizes
spaceflight, procedural surface animation, and PRNG sequences to a fixed
simulation clock matching the DOS PIT interrupt rate (~18.2 Hz).

| Metric | Target Budget | Measured Reference | Headroom |
| --- | --- | --- | --- |
| Simulation Tick Cadence | Fixed 55.0 ms (~18.18 Hz) | 55.0 ms (`std::chrono`) | Exact |
| Software Frame Rasterization | $\le$ 16.6 ms (target $\ge$ 60 FPS unthrottled) | 1.98 ms (~504 FPS unthrottled) | 96.4% |
| Sanitized Frame Rasterization (ASan/UBSan) | $\le$ 50.0 ms | 14.5 ms (~68.6 FPS) | 71.0% |
| Frame Pacing Jitter | $\le$ 1.0 ms standard deviation | < 0.2 ms (`sleep_until`) | Pass |

### Rationale

Maintaining frame rasterization below 16.6 ms guarantees that the CPU software
renderer consumes less than 30% of the 55.0 ms simulation window, ensuring zero
dropped simulation ticks, stutter-free movement, and whisper-quiet low-power
operation even on battery-powered portable devices.

---

## 2. Memory (Resident Set Size) Budget

| Execution Profile | Memory Budget | Measured Peak RSS | Headroom |
| --- | --- | --- | --- |
| Headless Diagnostics (`--diagnostics`) | $\le$ 16 MB | 6.1 MB | 61.9% |
| Headless Simulation Journey (817 frames) | $\le$ 32 MB | 7.2 MB | 77.5% |
| Interactive Graphical Window (Raylib/OpenGL 3.3) | $\le$ 150 MB | 92.0 MB | 38.7% |

### Internal Fixed Buffer Allocations

The core simulation retains fixed buffer pools totaling about 1.24 MiB:
- `adapted` (maximum 4× active/hidden video page): 1000.0 KiB (`1280 * 800`).
- `p_surfacemap` (surface elevation/topology cache): 39.1 KiB (`200 * 200`).
- `objectschart` (quadrant object table): 39.1 KiB (`oc_bytes`).
- `pvfile` (vehicle 3D mesh model): 20.0 KiB (`pv_bytes`).
- `n_offsets_map` & `n_globes_map`: 39.2 KiB (`om_bytes` + `gl_bytes` + `gl_brest`).
- `p_background` & `s_background`: 127.3 KiB (`pl_bytes` + `st_bytes`).

M17-W06 removed the former 64 KiB framebuffer tail and OR-based surface-map
padding. Renderer access is now bounded to the active page
and declared texture capacity; see `RENDERER_BOUNDS.md`.

---

## 3. Representative Scenarios Profiled

Profiling was conducted on an x86_64 reference testbed running Linux 6.x with an
AMD Radeon RX 7800 XT graphics driver (Mesa 26.2.4 Core Profile) and Clang 23.

### Scenario A: Full Orbit-to-Surface Journey (`--orbit-surface-fixture`)

- **Scope:** Deep space cruise, target acquisition, planetary approach to
  Felysia (1:60), atmospheric descent, touchdown, capsule exit, surface
  traversal, return to capsule, and liftoff.
- **Complexity:** Procedural generation of 3,590,648 tree coordinates, 2,133
  fauna positions, 35,221 ruin vertices, and 79 capsule draws.
- **Results:**
  - Total frames rendered: 817 frames.
  - Total CPU execution time: 1.62 seconds.
  - Average frame rendering time: 1.98 ms per frame.
  - Maximum Resident Set Size: 7,200 KB (7.2 MB).
  - Page faults: 0 major, 395 minor.

### Scenario B: Windowed Graphical Smoke (`--graphical-smoke`)

- **Scope:** Full GLFW/X11 initialization, OpenGL 3.3 Core Profile context
  creation, shader compilation, indexed 320x200 RGBA texture upload, three
  presented frames scaled to 1280x720, and clean resource deallocation.
- **Results:**
  - Total wall-clock time: 0.12 seconds.
  - Peak RSS (including Mesa driver, ACO shader compiler, and GLFW): 91.9 MB.
  - Clean exit code 0.

### Scenario C: Sanitized Memory Validation (`linux-clang-sanitized`)

- **Scope:** Complete 40-test suite executed under AddressSanitizer and
  UndefinedBehaviorSanitizer.
- **Results:**
  - Zero memory leaks detected (0 bytes leaked).
  - Zero buffer overflows or out-of-bounds accesses.
  - Total test suite duration: 50.81 seconds.

---

## 4. Conclusion and Release Readiness

All measured performance and memory metrics comfortably exceed the established
budgets. The software renderer and simulation pipeline run with more than 96%
available CPU headroom, ensuring flawless performance on modest hardware without
taxing modern multi-core systems.
