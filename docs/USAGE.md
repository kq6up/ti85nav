# Using the navigation programs

## SETUP and SIGHT

Run SETUP before using SIGHT's stored position. SETUP stores latitude,
longitude, index correction, eye height in metres, temperature in Celsius,
and pressure in hPa. Latitude is north positive / south negative; longitude
is west positive / east negative. Position inputs use degrees/minutes format.

SIGHT V8 asks for Air or Nautical Almanac mode, the body, sea or artificial
horizon, and the sextant angle. Subsequent prompts depend on those choices.
Moon sights also select lower limb, upper limb, or center.

Artificial-horizon angles are the doubled angle between the direct and
reflected images: SIGHT halves that entered angle and skips sea-horizon dip.
The program pauses between output pages; press ENTER to advance.

## Standalone almanacs

- MOON: enter date `YYYY.MMDD` and UTC `HH.MMSS`. The first page shows GHA,
  declination, and horizontal parallax in arcminutes. ENTER advances to
  semidiameter in arcminutes.
- SUN: enter date and UTC time using the same formats; outputs Sun almanac data.
- STAR: select a star number; zero displays the list. The first output page
  contains GHA Aries, SHA, and declination. The final page contains GHA star
  and declination.

GHA star is computed in decimal degrees as GHA Aries + SHA, modulo 360, then
converted for display. Do not add the displayed degrees/minutes numbers as
ordinary decimal numbers.

V8 does not call these almanacs automatically. Use their outputs with the
appropriate SIGHT prompts. Nautical interpolation requires hourly data and
the applicable signed corrections; a calculated instantaneous GHA is not an
hourly GHA unless the observation is on the hour. The worked Moon examples
document exactly how their interpolation inputs were derived.

## Transfer and interruption

Transfer `.85p` files using a compatible TI link utility or the emulator's
Send File function. If the TI-85 asks about an existing program, **F2 selects
OVERW**. Run programs from PRGM → NAMES; function-key assignments depend on
the installed program list.

Hold **ON** to interrupt a running program, then press **CLEAR**.

The supplied [transfer guide](original-bundle/TRANSFER.txt) describes its
original toolchain. This repository also supplies the portable Python converter.

## Additional programs

AIR, SEA, and HO229 are included from the supplied V8 / HO229 1.3 bundle and
have been converted with the current Python tool. They have not been run in
the current emulator session. See the original primer for their input workflow.
