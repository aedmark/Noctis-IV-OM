# M3-W06 Scripted Journey

M3-W06 closes the playable space-flight vertical slice with a headless workflow
fixture. The fixture launches at the inherited Stardrifter position, injects
tick-indexed `InputFrame` values through the production input-provider boundary,
and uses the flight-control keys `r`, `6`, `7`, and `8` to select and start the
remote and local legs of the canonical F02 journey.

## Journey contract

The script selects BALASTRACKONASTREYA, starts the Vimana drive, waits for
calibration, selects FELYSIA P04, starts fine approach, and waits for arrival.
It completes after 793 fixed simulation ticks:

| Checkpoint | Tick | State hash | Indexed-frame hash | Nonzero pixels |
| --- | ---: | --- | --- | ---: |
| Remote target selected | 1 | `13c978f4805771f8` | `1737cdd80c6f921d` | 576 |
| Parent star reached | 398 | `cdf5411db5d9f60d` | `5b8269d030b8f9b3` | 2,253 |
| FELYSIA selected | 399 | `a14749834f71048d` | `1b1bc9d7e0399397` | 3,523 |
| FELYSIA reached | 792 | `0502685c30c189ac` | `30629965a585364d` | 6,844 |

The state hashes include stage, tick, leg step counts, power, lithium charge,
and exact ship coordinates. The final state agrees with M3-W05: 397 remote
steps, 393 local steps, 19,788 kilodyams, and 117 remaining charges. The count
includes each leg's final arrival-check tick, whereas M3-W05 reports the
zero-based loop index as 396 and 392.

The script is replayed while presentation work runs every simulation tick and
again while it runs every fourth tick. State and checkpoint frames must remain
identical. Clang, GCC, and Clang ASan/UBSan agree on all exact values.

## Visual comparison level

Each visual checkpoint is an exact hash of all 64,000 bytes in a 320×200
indexed framebuffer. The views form a canonical native progression rendered by
the production flat/textured polygon paths. They are regression fixtures, not
DOS captures or complete live cockpit frames; their comparison level is EXACT
for this native harness and UNASSESSED against DOS.

## Scope boundary

The smoke journey combines the production input adapter, F02 system generator,
travel guidance, power behavior, and software polygon renderer without opening
a host window. Fixture target resolution is deterministic inside the harness;
GOESnet name lookup and the live point-at-a-star selection UI remain outside
this test. M3 therefore proves the stable native space-flight path and its
cross-frame-rate behavior, not every interactive cockpit route or DOS-pixel
identity. Windows verification remains deferred to M7-W01.
