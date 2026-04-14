# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What is frontier

Frontier is a command-line SVG weather chart renderer. It reads WOML (Weather Object Model) files (produced by the SmartMet Mirwa editor) and renders them as styled SVG weather analysis and forecast charts. It is part of the SmartMet Server ecosystem developed by the Finnish Meteorological Institute (FMI).

## Build commands

```bash
make                # Build the `frontier` executable
make clean          # Remove build artifacts
make format         # Run clang-format on all source files
make test           # Run integration tests (cd test && make test)
make install        # Install binary to $(bindir)
make rpm            # Build RPM package
```

The build uses the shared SmartMet build configuration (`makefile.inc`). Dependencies are declared via pkg-config in the `REQUIRES` variable: `configpp geos cairo xerces-c fmt`. Additional SmartMet libraries are linked directly: `smartmet-woml`, `smartmet-newbase`, `smartmet-macgyver`, `smartmet-tron`.

## Testing

Tests are shell-based integration tests in `test/`. `test/TestRunner.sh` runs the `frontier` binary against WOML input files and compares generated SVG output to reference files in `test/output/`. The binary is found via `PATH=..:$PATH`, so you must build before testing.

Test cases render `test/woml/europe-forecast.woml` with two locales (FI-fi, SV-se) and diff against expected SVG output. Tests auto-detect WGS84 mode and use `.wgs84` suffixed reference files when needed.

To run a single test manually:
```bash
cd test
PATH=..:$PATH frontier -w woml/europe-forecast.woml -s tpl/europe-forecast.tpl \
  -p stereographic,5,90,60:-12.24349761,31.83310001,74.66294552,54.86671043:458,-1 \
  -d -t conceptualmodelanalysis -l FI-fi > output.svg
```

## CLI usage

```
frontier -w <womlfile> -s <svgtemplate> -p <projection> [-o <outfile>] \
         -t <type> [-l <locale>] [--verbose|--quiet|--debug]
```

- `-t` type: `conceptualmodelanalysis`, `conceptualmodelforecast`, or `aerodromeforecast`
- `-p` projection: inline spec (e.g. `stereographic,5,90,60:...`) or path to a projection file
- Output goes to stdout if `-o` is not specified

## Architecture

### Source layout

- `main/frontier.cpp` -- Entry point. Reads SVG template (with `#define`/`#include` preprocessing), extracts embedded `<frontier>...</frontier>` libconfig configuration block, parses WOML, drives rendering, writes output.
- `source/SvgRenderer.cpp` (~12k LOC) -- Core rendering engine. Implements `woml::FeatureVisitor` (visitor pattern) with ~24 `visit()` methods for different WOML weather feature types (fronts, pressure centers, cloud areas, precipitation, jet streams, etc.). This is the bulk of the codebase.
- `source/Path.cpp`, `source/BezierModel.cpp`, `source/BezSeg.cpp`, `source/CubicBezier.cpp`, `source/PathFactory.cpp` -- SVG path geometry and Bezier curve operations.
- `source/clipper.cpp` -- Embedded Clipper2 polygon clipping library for overlapping weather feature regions.
- `source/NFmiFillMap.cpp` -- Symbol/label placement to avoid overlaps.
- `source/PreProcessor.cpp` -- Extends `NFmiPreProcessor` to handle `#define`/`#include` in SVG templates.
- `source/Options.cpp` -- CLI argument parsing via `boost::program_options`.
- `include/ConfigTools.h` -- Libconfig utilities (template helpers for reading typed config values).

### Key design patterns

- **Visitor pattern**: `SvgRenderer` visits each WOML feature type to produce SVG elements. Adding a new weather feature type means adding a new `visit()` override.
- **Template preprocessing**: SVG template files embed a `<frontier>...</frontier>` libconfig block and support C-preprocessor-style `#define`/`#include` directives before parsing.
- **Configuration scoping**: Rendering configuration uses libconfig with scoped blocks for different feature types, supporting both global and locale-specific settings.

## Dependencies

Build-time SmartMet libraries (linked via `-l`):
- `smartmet-woml` -- WOML XML file parsing (primary input format)
- `smartmet-newbase` -- Meteorological data structures, projections, QueryData format
- `smartmet-macgyver` -- General C++ utilities
- `smartmet-tron` -- Contouring and interpolation

System libraries: GEOS (geometry), Cairo (2D graphics), Xerces-C + XQilla (XML/XPath), libconfig (configuration), Boost (program_options, iostreams, regex), fmt.
