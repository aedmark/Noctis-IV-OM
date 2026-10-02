# M6-W03 — Noctis IV Plus presentation

M6-W03 ports the remaining first-release HUD and presentation features from the
final pinned NIV+ source. The behavior is implemented in native rendering and
semantic input boundaries rather than copying DOS event or framebuffer code.

## Player-visible behavior

- Remote-star and local-body data sheets include radius in centidyams with four
  decimal places.
- Surface status messages retain up to 41 characters and appear near the center
  of the visor. The compact cockpit status remains ten characters.
- Surface objects retain their full population through depth 255 and are halved
  beginning at depth 256, replacing the LR depth-16 cutoff.
- F1 toggles a context-specific shortcut page in space or on the surface.
  The port's earlier cursor-capture shortcut moves to F10.
- F2 toggles the visual-effects panel. While it is open, `t` toggles HUD text,
  `f` cycles visor-only/always-on/always-off lens flare behavior, and `b` or
  Delete toggles the seamless border. These three choices use the fields already
  reserved in native save v1 and migrate from supported NIV+ saves.

## Acceptance evidence

| Ledger row | Evidence |
| --- | --- |
| P10 | `plus_presentation` pins the exact radius string for DOS-confirmed FELYSIA; both production target-sheet branches call the same formatter. |
| P11 | `ship_interface_application` submits a long status and verifies distinct compact and extended buffers; the production surface loop displays and expires the extended form. |
| P13 | The focused boundary verifies full counts at depths 16 and 255 and halving at 256. Existing surface-content and orbit-to-surface fixtures now record the increased production tree/rock draw counts and complete frame hashes. |
| P20 | Semantic input coverage pins F1/F2 DOS scan-code ordering. Context-specific help content has a focused inventory, and the application fixture proves the help overlay changes the logical framebuffer. |
| P25 | Focused tests cover the three-state flare cycle/override and exact menu labels. The application fixture renders a distinct F2 overlay and round-trips HUD text, flare mode, and border choice through native persistence. Environment and journey hashes cover the corrected default border geometry. |

The presentation fixtures remain exact native baselines, not claims that every
pixel matches the DOS executable. Moviemaker is restored as M6-W06 by ADR-0013,
and Windows rendering verification remains assigned to M7-W01 by ADR-0009.
