# Persistence Hardening

M5-W07 closes the Linux persistence milestone by testing corruption, upgrades,
failed replacement, and continued play across a real application restart.

## Corruption matrix

`native_save_round_trip` flips one bit in every byte of both valid v1
envelopes: all 401 situation bytes and all 65 surface bytes. Every altered
magic, version, size, checksum, or payload byte is rejected, and the caller's
destination state remains unchanged.

Checksum repair is not enough to make hostile state valid. Separate cases
recompute a correct CRC around an invalid selector, a non-finite number,
unterminated FCS and GOESnet text, an invalid preference, and a non-finite
surface position. Native reads now apply the same semantic boundary that
protected legacy imports before publishing decoded state. Oversized native
files are refused before allocation.

## Replacement and migration durability

Situation and surface writers validate before writing, create a sibling
temporary file, flush and close it, and publish only by replacement. Tests
block the temporary path and make an existing destination read-only. The
committed file remains byte-for-byte unchanged, and a directory occupying the
temporary name is never mistaken for disposable output.

Application migration is tested under the same failure: a valid legacy save
remains present, no partial native file appears, and startup reports the
migration error. Surface migration provides the equivalent focused evidence
and does not partially populate its result object.

## Upgrade and gameplay round trips

Every supported situation layout (245, 370, 377–381, and transitional 382
bytes) is imported, written as native v1, reopened, and compared after its
documented normalization. Both 40- and 45-byte surface layouts follow the same
chain. Unknown future native versions remain explicitly unsupported; v1 is the
only native schema currently available, so no invented native predecessor is
claimed.

The `persistence_journey` fixture starts from the tracked FELYSIA legacy save,
uses production GOESnet and cockpit command paths to select a remote star and
FELYSIA, enable four preferences, select a HUD panel, and change ship state.
It saves, removes the legacy input, starts a second application process from
native v1 alone, verifies the state, continues, and saves again. No host window
or manual input is required.

Every fixture process runs with `--fixture-universe-seconds 1000000000`, which
pins the universe clock that `unfreeze()` uses to replay hidden elapsed time.
Without it, the clean-restart phase was flaky: the internal lamp (on by
default) is charged `elapsed / 84` power for the wall-clock seconds between
the previous save and the next load, and the truncation to `int16_t` turned
any crossed second boundary into 19999 power. The flag is accepted only in
fixture modes; gameplay still uses the wall clock.

## Scope

All evidence is automated on the supported Linux lanes. Windows verification
remains deferred to M7-W01 by ADR-0009. Power-loss durability beyond a
completed userspace flush (for example filesystem or hardware guarantees)
depends on the eventual packaging platform and is not claimed here.
