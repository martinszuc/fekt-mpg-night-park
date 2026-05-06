# CLAUDE.md — Night Park / MPC-MPG

Quick-start context for any new Claude Code session on this project.

---

## Project

Semester project for **MPC-MPG 2025/26** at VUT FEKT.
First-person walk through a night park. Target: **24 points**.
Platform: **macOS 14, CLion, C++17, OpenGL (legacy) + GLUT framework**.

Working files: `xszucm00.cpp` (GLUT callbacks + main) and `scene.h` (all scene globals, structs, draw functions). `xszucm00.cpp` must never be renamed — it is the submission file. `scene.h` is the one permitted extra header.
Build: `cmake -B build && cmake --build build`, then run from project root: `./build/mpg_projekt`.

---

## Repository layout

```
xszucm00.cpp          GLUT callbacks + main (thin entry point, ~280 lines)
scene.h               all scene globals, structs, and Draw* functions (~560 lines)
imageLoad.h           BMP/TGA loader — do not modify
CMakeLists.txt        build config
VERSION               plain-text version (e.g. 0.1.1) — written by release CI from tag
assets/
  textures/
    grass.bmp         512×512 grass texture (converted from JPEG)
screenshots/
  latest/             scene_0..2.webp — overwritten on every release
  v0.1.0/            archived WebP screenshots per release tag
TASK.md               phase-by-phase implementation plan with ✅ progress markers
README.md             project design notes + auto-updated screenshot sections
CODESTYLE.md          coding and git rules
.github/workflows/
  build.yml           builds on macOS + Windows on every push/PR
  release.yml         triggered by v* tag — renders screenshots, updates README, creates GH release
```

---

## Current progress (update this after each phase)

| Phase | Content | Status |
|-------|---------|--------|
| Setup | xszucm00.cpp from template, CMakeLists, imageLoad.h | ✅ done |
| 1 | Camera state, WASD+arrows, mouse look, bobbing, Page Up/Down, torch, moon, HUD, menu, projectile struct, timer | ✅ done |
| 2 | Scene objects: tree, bench, lantern, shed, boulder, fence, windmill | ✅ done |
| 3 | Bezier terrain + grass texture + checker path | ✅ done |
| 4 | Menu wired, HUD, lantern window transparency | ✅ done |
| 5 | Projectile throw with gravity + octahedron draw | ✅ done |
| 6 | Final cleanup, header, checklist | ✅ done |
| Refactor | Split into xszucm00.cpp + scene.h | ✅ done |
| Post A | Lantern point lights (GL_LIGHT2/3, warm yellow, attenuation) | ✅ done |
| Post B | Atmospheric fog (GL_EXP2, density 0.018, clear color matched) | ✅ done |
| Post C | Night sky: 200 stars + moon billboard quad | ✅ done |
| Post D | Tree variation: per-tree scale and yaw in TreeSpec array | ✅ done |
| Post E | Real delta-time via glutElapsedTime, 50ms clamp | ✅ done |
| Post F | DrawScene side-effect removed, kLanterns constexpr, debug log guarded | ✅ done |

Check `TASK.md` for detailed per-phase specs.

---

## Rules — read before touching any code

### Language
- **All code, comments, strings, HUD labels, and menu entries must be in English.**
- No Czech anywhere in the source.

### Code style (see CODESTYLE.md for full details)
- C++17, no external libraries beyond OpenGL/GLUT + imageLoad.h
- No `glutSolidSphere`, no GLU quadrics — all geometry via vertex arrays / `glBegin`
- Every drawn object needs correct normals for lighting
- Lighting: `GL_LIGHT0` = moon (directional), `GL_LIGHT1` = torch (spotlight, toggle R), `GL_LIGHT2/3` = lantern point lights (toggle via menu)
- Draw order: sky (no depth) → opaque objects → transparent objects (depth mask off) → HUD
- Fog is always on; `DrawSky` disables it for star/moon rendering then re-enables it
- Textures live in `assets/textures/`, referenced by relative path from project root

### Git commits
- Command: `git commit -s -S` — GPG-signed with system credentials, **no AI trailer**
- Format: `type(scope): short imperative description` (conventional commits, ≤72 chars)
- One logical concern per commit
- Types: `feat`, `fix`, `refactor`, `style`, `docs`, `chore`

### Asset pipeline
- Textures must be BMP or TGA (imageLoad.h limitation)
- Convert with: `sips -s format bmp -z 512 512 input.jpg --out assets/textures/name.bmp`
- Keep textures power-of-2, max 1024×1024

---

## Key globals

All globals are defined in `scene.h` (included once by `xszucm00.cpp`). `ciScenario` and `SaveBMP` live in `xszucm00.cpp`.

| Variable | File | Purpose |
|----------|------|---------|
| `camX/Z` | scene.h | Camera world position (Y driven by camFloorY + bobOffset) |
| `camFloorY` | scene.h | Base eye height (Page Up/Down) |
| `yaw / pitch` | scene.h | View angles in radians |
| `bobOffset` | scene.h | Vertical camera shake added to `camFloorY` |
| `torchOn` | scene.h | Whether GL_LIGHT1 spotlight is active |
| `animOn` | scene.h | Whether windmill rotates |
| `windmillAngle` | scene.h | Current windmill rotation angle |
| `texOn` | scene.h | Texture enable toggle |
| `texGrass / texChecker` | scene.h | OpenGL texture IDs |
| `lastAction` | scene.h | String shown in HUD bottom-left |
| `projectiles[10]` | scene.h | Active thrown objects |
| `kLanterns / kLanternCount` | scene.h | `constexpr` lantern world positions (used by DrawScene, OnDisplay, and lantern lights) |
| `starVerts / starCount` | scene.h | Pre-generated star point cloud for DrawSky |
| `ciScenario` | xszucm00.cpp | -1 = normal mode; 0-2 = headless CI render then exit |

---

## CI / release workflow

### How releases work
1. Push a `v*.*.*` tag: `git tag v0.2.0 && git push origin v0.2.0`
2. `release.yml` runs on Ubuntu with Xvfb + Mesa software GL
3. Builds the project, then runs the app **three times** with `--ci-scenario 0/1/2`
4. Each run sets the hardcoded camera, renders one frame, writes `ci_shot_N.bmp` via `glReadPixels`, exits
5. BMPs are converted to WebP with ImageMagick
6. Saved to `screenshots/latest/` (overwritten) and `screenshots/vX.Y.Z/` (archived)
7. README is updated: latest section (3 full-width stacked images) + history table row prepended
8. `VERSION` file is updated from the tag (strip `v` prefix)
9. A GitHub release is created with the three WebP files attached

### CI screenshot scenarios (--ci-scenario)
Hardcoded camera positions in `xszucm00.cpp` (yaw/pitch in radians):

| # | camX | camZ | yaw (°) | pitch (°) | What's visible |
|---|------|------|---------|-----------|----------------|
| 0 | 6.56 | 6.69 | 4.58    | 1.15      | bench + lanterns area |
| 1 | 5.02 | 17.15| 45.26   | 0.29      | entry angle, windmill side |
| 2 | 1.75 | 5.37 | 8.59    | 1.43      | deep park, shed visible |

To add or change scenarios: edit the `scenarios[]` array in `main()` inside `xszucm00.cpp`.

### Debug camera logger
- **Automatic**: prints `[CAM]` every 5 s — only when built with `-DDEBUG_CAM` (guarded by `#ifdef DEBUG_CAM`)
- **On demand**: press `P` for an instant position dump (always available)
- Output format: `[CAM] x=  y=  z=  yaw= deg  pitch= deg`
- Use this to find new viewpoints, then hardcode them as CI scenarios

### Linux build notes (relevant for CI)
- Link `OpenGL::GLU` explicitly — GLU is a separate lib on Linux, not bundled in the framework
- Use `GLUT::GLUT` (uppercase) — that is the correct CMake imported target name
- `#include <cstring>` must come before `imageLoad.h` — Linux GCC does not pull it in implicitly
- CI uses `LIBGL_ALWAYS_SOFTWARE=1` with Mesa for headless rendering on Xvfb

---

## Missing / blocked

- `grass.bmp` ✅ present at `assets/textures/grass.bmp`
- No blockers
