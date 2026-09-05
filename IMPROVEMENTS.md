# Improvement ideas

This is a short backlog of practical improvements for `superfree`, ordered from
low-risk polish to larger product changes.

## Near-term improvements

1. **Version output**
   - Add `--version` so users can see the installed version.

2. **No-color mode**
   - `--color never` disables ANSI colors for terminals that do not support
     them and for piping output into files.
   - Make `--color auto` detect whether stdout is a TTY.

3. **Better argument validation**
   - `--help` and `--unit kB|KiB|MiB|GiB|TiB|auto` are available now.
   - Add clear errors for unsupported units and unknown options.

4. **Safer parsing layer**
   - Move `/proc/meminfo` parsing into a small standalone class or function.
   - Return clear errors if required fields are missing or malformed.

5. **Tests with fixture data**
   - Add sample `meminfo` fixtures, including a system with no swap configured.
   - Test percentage calculation, parsing, and table formatting separately.

## Medium-term improvements

1. **Refactor source layout**
   - Move reusable code from `main.cpp` into dedicated files, for example
     `MemInfo.h` and `MemInfo.cpp`.
   - Keep `main.cpp` focused on CLI argument parsing and output.

2. **Packaging**
   - Add a GitHub Actions workflow that builds on push and pull request.
   - Add release artifacts for common Linux architectures.
   - Consider packaging for AUR/Homebrew/Linuxbrew once the CLI is stable.

3. **Configurable thresholds**
   - Allow users to tune the warning and critical usage thresholds that control
     bar colors.

4. **Machine-readable output**
   - Add `--json` for scripts, dashboards, and monitoring integrations.

## Code quality notes from the initial audit

- The project already builds and runs with CMake.
- The initial cleanup added a CMake install target and ignored local CMake build
  outputs.
- Human-readable units are now the default. `--unit kB|KiB|MiB|GiB|TiB` can
  force a specific unit.
- Output labels and help text are English-only.
- Memory, swap, and totals now render in a single table for cleaner alignment.
- Colors are enabled by default. `--color auto|always|never` controls colored
  output, and each row is differentiated by usage threshold: green under 60%,
  yellow under 90%, red at 90% or above.
- Percentage calculation now handles zero totals, which matters on Linux systems
  with no swap configured.
- A few table index bounds checks were tightened to avoid out-of-range access.

## Failure points to harden next

- `/proc/meminfo` parsing currently assumes the expected fields exist and are
  numeric. A missing or malformed field can still throw during startup.
- `cleanData` assumes meminfo values end with ` kB`. That is true on normal
  Linux systems, but fixture-based parsing would make this safer and easier to
  test.
- Unknown command-line arguments are currently ignored. Add explicit validation
  alongside `--version`.
- Table width calculations now handle ANSI escape sequences, but do not yet
  handle East Asian wide characters or combining marks.
- ANSI color escape sequences now use standard `\x1b` escapes instead of the
  GNU-only `\e` escape, avoiding pedantic compiler warnings.