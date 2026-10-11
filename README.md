# TI85Nav

Celestial navigation programs for the **TI-85**: sight reduction plus Sun, star,
and Moon almanac calculations. Editable sources use ASCII85P escape syntax;
ready-to-transfer `.85p` files are in [`programs/`](programs).

## Programs

| Program | Purpose | Current evidence |
|---|---|---|
| SETUP | Store position, index correction, eye height, temperature, and pressure | Run in the TI-85 emulator |
| SIGHT V8 | Air / Nautical Almanac sight correction and reduction; outputs Ho, Hc, Zn, and intercept | Ten simulated Moon sights completed |
| MOON 1.0 | Date/time to Moon GHA, declination, HP, and SD | Ten times compared with JPL Horizons |
| SUN | Date/time to Sun GHA, declination, and SD | One supplied example run |
| STAR | Star selection, GHA Aries, SHA, declination, and GHA star | Run; final result page includes GHA star and declination |
| HO229 1.3 | Standalone Hc / Z / Zn reduction | Converted here; runtime not checked |
| SEA | Standalone sea-horizon altitude correction | Converted here; runtime not checked |
| AIR | Standalone Air Almanac altitude correction | Converted here; runtime not checked |

## Use

1. Transfer the required files from `programs/` to your TI-85 or emulator.
2. Run **SETUP** first, with your position, instrument settings, and weather.
3. Run **SIGHT** for a reduction. V8 retains manual Air and Nautical Almanac entry.
4. Run **MOON**, **SUN**, or **STAR** separately to calculate almanac data.

Angles entered as `DD.MMm` are degrees and minutes, not decimal degrees:
`34.020` means 34°02.0′. Dates use `YYYY.MMDD`; observation times use UTC
`HH.MMSS`. SIGHT's elapsed-time prompts use `MM.SS`. Follow each program's
displayed units and signs; SETUP longitude is west positive, east negative.

The native emulator used during development is
[TilEm Portable](https://github.com/kq6up/tilem-portable). It retains the real
calculator GUI and can optionally expose the same calculator through a local
web control panel. Calculator ROMs and emulator RAM saves are not part of this
navigation repository.

See [usage notes](docs/USAGE.md) and the supplied
[celestial navigation primer](docs/original-bundle/CELNAV_PRIMER.pdf).

## Rebuild the TI-85 files

Use Python 3.10 or later; no external Python packages are needed.

```sh
make programs
# Or directly:
python3 tools/ascii85p.py src/*.ASC --output-dir programs
```

The Python converter creates native tokenized TI-85 program files and reserves the
calculator's Goto destination cache bytes. It rejects unsupported syntax and
does not execute BASIC sources on the host.

### C converter

The supplied V8 bundle's C converter is also included as
[`tools/ascii85p.c`](tools/ascii85p.c). It was originally named `ti85asc.c`;
the source is preserved unchanged. Build it with a C99 compiler and make:

```sh
make converter
# Or directly:
cc -std=c99 -O2 -Wall -Wextra -o ascii85p tools/ascii85p.c
```

The make target places the executable at `build/ascii85p`. Its commands are:

```sh
build/ascii85p lint src/SIGHT.ASC
build/ascii85p pack src/SIGHT.ASC build/SIGHT.85p
build/ascii85p check build/SIGHT.85p
build/ascii85p unpack build/SIGHT.85p > build/SIGHT.bas
```

This C converter emits the older plain-text TI-85 program representation and
supports lint, pack, unpack, and structural/checksum checks. Its escape support
differs from the Python tokenizer; it is not assumed to accept every source in
this repository. `unpack` supports plain-text files only. The original
[`converter notes`](tools/README_TI85ASC.txt) are retained; their verification
statements describe the supplied bundle. The C converter has not been built or
run here as part of this repository update. `make programs` continues to use the
Python tokenizer that produced the included, recorded calculator runs.

The supplied V8 bundle's SIGHT binary did not reach its startup banner on the
development ROM. The SIGHT binary distributed here was rebuilt from the
canonical V8 source with the Python tokenizer and completed the recorded scenarios.
See [source and binary provenance](docs/PROVENANCE.md).

## Results and roadmap

- [Ten simulated Moon sights, October 11, 2026](examples/moon-shots-2026-10-11/report.md)
- [Numerical results](examples/moon-shots-2026-10-11/results.csv)
- [Actual calculator result screens](examples/moon-shots-2026-10-11/result-screens.png)
- [SIGHT v9 enhancement todo](docs/SIGHT-V9-TODO.md)

The planned **Internal** almanac branch will retain the existing Air / Nautical
workflows while calculating almanac values internally. After SETUP and session
date selection, repeated sights would need the angle and UTC time rather than
manual almanac entries. **This branch is planned, not implemented.**

## Source notices

The Python converter is GPL-3.0-or-later; its notice and license are preserved
in `tools/`. The supplied navigation sources and original bundle documents
did not include an explicit redistribution license. Their provenance is
recorded in `docs/PROVENANCE.md`; no new license has been assigned to them.
The supplied C converter has no explicit license notice; the Python converter's
GPL notice has not been applied to it.
