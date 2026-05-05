# CLAUDE.md — Night Park / MPC-MPG

Quick-start context for any new Claude Code session on this project.

---

## Project

Semester project for **MPC-MPG 2025/26** at VUT FEKT.
First-person walk through a night park. Target: **24 points**.
Platform: **macOS 14, CLion, C++17, OpenGL (legacy) + GLUT framework**.

Working file: `xszucm00.cpp` — this is the only source file, never rename it.
Build: `cmake -B build && cmake --build build`, then run from project root: `./build/mpg_projekt`.

---

## Repository layout

```
xszucm00.cpp          main source (all code lives here)
imageLoad.h           BMP/TGA loader — do not modify
CMakeLists.txt        build config
assets/
  textures/
    grass.bmp         512×512 grass texture (converted from JPEG)
TASK.md               phase-by-phase implementation plan with ✅ progress markers
README.md             project design notes and code snippets
CODESTYLE.md          coding and git rules
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
| 6 | Final cleanup, header, checklist | ⬜ |

Check `TASK.md` for detailed per-phase specs. Mark phases `✅` there as they complete.

---

## Rules — read before touching any code

### Language
- **All code, comments, strings, HUD labels, and menu entries must be in English.**
- No Czech anywhere in the source.

### Code style (see CODESTYLE.md for full details)
- C++17, no external libraries beyond OpenGL/GLUT + imageLoad.h
- No `glutSolidSphere`, no GLU quadrics — all geometry via vertex arrays / `glBegin`
- Every drawn object needs correct normals for lighting
- Lighting: `GL_LIGHT0` = moon (directional), `GL_LIGHT1` = torch (spotlight, toggle R)
- Draw order: opaque objects → transparent objects (depth mask off) → HUD
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

## Key globals (phase 1)

| Variable | Purpose |
|----------|---------|
| `camX/Z` | Camera world position (Y driven by camFloorY + bobOffset) |
| `camFloorY` | Base eye height (Page Up/Down) |
| `yaw / pitch` | View angles in radians |
| `bobOffset` | Vertical camera shake added to `camFloorY` |
| `torchOn` | Whether GL_LIGHT1 spotlight is active |
| `animOn` | Whether windmill rotates |
| `windmillAngle` | Current windmill rotation angle |
| `texOn` | Texture enable toggle |
| `texGrass / texChecker` | OpenGL texture IDs |
| `lastAction` | String shown in HUD bottom-left |
| `projectiles[10]` | Active thrown objects |

---

## Missing / blocked

- `grass.bmp` ✅ present at `assets/textures/grass.bmp`
- No blockers for Phase 2
