# SIGHT v9 — Internal almanac enhancement

Recorded October 11, 2026. Status: **planned**.

## Requested feature

- [ ] Preserve SIGHT’s existing Nautical and Air Almanac functions.
- [ ] Add a third almanac source, provisionally **Nav / Air / Internal**.
- [ ] Integrate the existing almanac calculation code into the Internal branch, starting with MOON. Plan how the existing SUN and STAR calculations can use the same branch.
- [ ] After SETUP, allow an Internal sight to be reduced using the sight angle and observation time, without manually entering almanac quantities.

## Intended workflow

1. Run SETUP to store position, index correction, eye height, temperature, and pressure.
2. Select Internal in SIGHT; select the body and applicable horizon/limb options.
3. Enter the sight angle and UTC observation time.
4. Calculate the almanac quantities for that instant internally and feed them into the existing sight correction and reduction code.
5. Display the existing Ho, GHA, declination, LHA, Hc, Zn, and toward/away intercept results, preserving pagination.

## Implementation checklist

- [ ] Establish where the observation date is stored: add it to SETUP or request it once per session. Time alone is insufficient for the internal ephemeris; avoid requesting the date again for every sight.
- [ ] Calculate GHA and declination directly at the observation time, bypassing hourly entries and manual interpolation prompts.
- [ ] For Moon sights, obtain HP and SD internally, bypassing those almanac prompts too.
- [ ] Keep the body, horizon, and limb choices available; consider retaining them between consecutive sights so the repeated workflow needs only angle and time.
- [ ] Reuse the existing refraction, dip, limb, Moon augmentation/parallax, and reduction paths rather than duplicating them.
- [ ] Keep SETUP values intact when integrating the almanac code; check shared TI-85 variables for collisions.
- [ ] Define the initially supported Internal bodies explicitly. Unsupported bodies should remain usable through the existing manual branches.
- [ ] Handle UTC midnight/date rollover explicitly.
- [ ] Check TI-85 program size, available memory, and calculation time.

## Acceptance checks for implementation

- [ ] Existing Nautical and Air workflows still produce their previous results.
- [ ] Internal Moon sights require no GHA, declination, v/d, HP, or SD entry.
- [ ] Internal and manual reductions agree when supplied the same underlying almanac data.
- [ ] Exercise lower limb, upper limb, center, artificial horizon, and sea horizon, including UTC midnight and azimuth quadrant boundaries.
- [ ] Reuse the ten October 11 Moon scenarios as a regression baseline and retain an independent ephemeris comparison.

## Bookmark / current baseline

SIGHT V8, SETUP, and MOON are loaded and working in the TI-85 emulator. Ten simulated artificial-horizon Moon sights were completed with nearby Riverside weather inputs of 17°C and 1002.71 hPa. SIGHT agreed with the separate calculation in all ten cases; substituting JPL Moon almanac values changed the signed intercept by at most 0.524 NM for these cases.

- [Ten-scenario report](../examples/moon-shots-2026-10-11/report.md)
- [Results and exact entered values](../examples/moon-shots-2026-10-11/results.json)
- [SIGHT V8 calculator program](../programs/SIGHT.85p)
- [MOON calculator program](../programs/MOON.85p)

This todo records the enhancement; implementation has not started.
