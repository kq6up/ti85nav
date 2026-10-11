SIGHT V8 - OpenCPN refraction update
=====================================

Base: verified V7 SIGHT source/binary pair.
Change: one calculation only -- atmospheric refraction.

OLD (R in arcminutes):
  1.02/tan(H+10.3/(H+5.11))*(P/1010)*(283/(273+T))->R

NEW -- algebraically identical to the OpenCPN Celestial Navigation plugin formula shown in the user's OpenCPN 5.14 calculations window (R in arcminutes):
  .267*P/(tan(H+.04848/(tan(H)+.028))*(T+273.15))->R

The following line remains unchanged:
  H-R/60->H

No almanac, body-routing, parallax, semidiameter, Hc, Zn, intercept, Air/Nautical, or UI logic was changed.

Binary generation:
  SIGHT.85p was minimally patched from the verified V7 ASCII85P-produced binary.
  The old formula token sequence was replaced with the new formula token sequence.
  Cached Goto target offsets after the changed block were shifted by -7 bytes.
  TI-85 data lengths and checksum were recomputed.
  Structural and Goto-target consistency checks passed.

Note: The source remains ASCII85P escape syntax and should be considered the canonical editable source.

Startup banner update:
  SIGHT now displays "SIGHT VERSION8" immediately at launch.
  This replacement is exactly the same byte length as the previous
  "SIGHT FULL RED" banner, so program offsets/Goto targets are unchanged.
