# M3 timing model

Status: M3-W01 complete on Linux.

## Clock domains

The native loop now has three explicit timing domains:

| Domain | Source | Purpose |
| --- | --- | --- |
| Universe epoch | System wall clock, synchronized at startup/save restoration | Preserve the legacy calendar and offline elapsed-time behavior |
| Simulation | Monotonic integer tick count at a fixed 55 ms step | Movement, procedural animation, state changes, and PRNG seeds |
| Presentation/input | `std::chrono::steady_clock` | Frame pacing and the real-time double-click window |

`SimulationClock` computes time from a synchronized base plus an integer tick
count. It does not accumulate measured frame durations, so identical tick and
input sequences produce identical simulation times even when rendering work
takes different amounts of time. A focused test covers reset, the first tick,
1,000 ticks, and re-synchronization.

The former implementation reconstructed fractional universe time from frames
observed during the current wall-clock second. Rendering throughput therefore
changed `secs`, `fsecs`, animation phase, and time-seeded procedural effects.
Those FPS counters and the CPU-clock busy waits have been removed. Simulation
and animation sites that used C `clock()` now use the fixed simulation tick.

## Presentation pacing

`swapBuffers()` independently targets the DOS-compatible 55 ms presentation
cadence with `steady_clock::sleep_until`. If a frame misses its deadline, the
next deadline is rebased instead of accumulating an unbounded delay. This
pacer does not write simulation time.

## Limits and follow-on work

- The current loop still performs one simulation step for each completed
  gameplay frame. M3-W06 verifies that the canonical journey has identical
  state and indexed checkpoints when presentation work runs every tick or every
  fourth tick. A real-time catch-up/skip policy under sustained slow rendering
  remains a later performance decision.
- M3-W02 moved live input behind an injectable provider, and M3-W06 uses that
  boundary for its tick-indexed journey.
- DOS-reference timing fixtures do not yet exist. The 55 ms cadence comes from
  the inherited LR/DOS compatibility constant and is protected as a native
  regression contract.
