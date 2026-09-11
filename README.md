# Computer Graphics Labs

[![CI](https://github.com/PavloShutz/Computer-Graphics-Labs/actions/workflows/ci.yml/badge.svg)](https://github.com/PavloShutz/Computer-Graphics-Labs/actions/workflows/ci.yml)

Small graphics programs built on [SDL3](https://github.com/libsdl-org/SDL), one
directory per lab.

| Lab | What it draws |
| --- | --- |
| `CircleInterpolation` | A circle rasterised with Bresenham's midpoint algorithm, one quadrant stepped and the other three mirrored. |
| `LagrangeInterpolation` | A Lagrange polynomial through seven fixed nodes, sampled once per pixel column. |
| `AffineTransformation` | A triangle rotated about its own centroid, driven by horizontal mouse movement. |

## Building with CMake

Works on Linux and Windows and needs no manual SDL setup — if it can't find an
SDL3 to use, it fetches a pinned one:

```sh
cmake -S . -B build
cmake --build build --parallel
```

The binaries land in `build/` (or `build/<Config>/` with Visual Studio). The
first configure takes a few minutes because SDL is built from source; after
that it is incremental.

CMake looks for SDL in three places, in order:

1. An SDL3 already installed on the machine (vcpkg, apt, Homebrew, or
   `-DCMAKE_PREFIX_PATH=...`).
2. An SDL source tree on disk. Pass `-DLABS_SDL_SOURCE_DIR=/path/to/SDL`, or
   just leave the `SDL-release-3.4.16` checkout sitting next to this repository
   where the Visual Studio projects already expect it.
3. Otherwise, a shallow clone of `release-3.4.16` from GitHub. Override the tag
   with `-DLABS_SDL_TAG=release-3.x.y`.

On Linux, SDL is built from source and its CMake treats a missing backend
dependency as a hard error, so the development headers have to be present. The
list the CI installs is in [`.github/workflows/ci.yml`](.github/workflows/ci.yml).

Other options: `-DLABS_BUILD_TESTS=OFF` skips the smoke tests,
`-DLABS_WERROR=ON` turns compiler warnings into errors (CI uses this).

## Building with Visual Studio

`Labs.slnx` still works and is the most convenient way to debug a lab on
Windows. It expects an SDL checkout unpacked as `SDL-release-3.4.16` next to
this repository, because the projects reference `..\..\SDL-release-3.4.16`.

Visual Studio also opens this folder as a CMake project directly
(*File → Open → Folder*), which needs no SDL setup at all.

## Tests

Each lab gets a headless smoke test. It is not a correctness test — it does not
know what a circle should look like — it answers the question that actually
breaks in practice: does this lab still start up, render frames, handle events
and shut down?

```sh
ctest --test-dir build --output-on-failure
```

The labs use SDL's main-callback API, so none of them has a `main()` that could
be told to stop after N frames. [`tests/smoke_main.c`](tests/smoke_main.c) is
linked against a lab in place of the entry point SDL generates and drives the
same `SDL_AppInit` / `SDL_AppEvent` / `SDL_AppIterate` / `SDL_AppQuit`
callbacks for a bounded number of frames. With `SDL_VIDEODRIVER=dummy` (set by
CTest) that needs no display, so it runs anywhere. It fails on a crash, on any
callback reporting `SDL_APP_FAILURE`, and on a lab that stops honouring
`SDL_EVENT_QUIT`.

To run one by hand:

```sh
SDL_VIDEODRIVER=dummy ./build/CircleInterpolation_smoke --frames 500
```

## CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) builds every lab on
Linux and Windows, in Debug and Release, then runs the smoke tests. Warnings
are errors on the Linux leg; MSVC's `/W4` output is advisory, so a warning only
Microsoft raises reports itself rather than failing the build. That matrix is
the point: the labs are developed against MSVC, and a second toolchain catches
what one compiler alone lets through — it found `AffineTransformation` failing
to link on Linux, because MSVC resolves `sinf` and `cosf` out of its C runtime
while GNU toolchains need an explicit `-lm`.

## Adding a lab

Create `<LabName>/program.c`. CMake discovers any directory containing a
`program.c`, so a new lab is built, warned about and smoke-tested with no
changes to the build files — the point being that a lab nobody has opened in
months can't quietly fall out of CI.

Add a `.vcxproj` too if you want it in `Labs.slnx`.
