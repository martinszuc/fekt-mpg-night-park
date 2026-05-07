# TASK.md — v0.1.4 "Big Windmill" Visual Overhaul

This release makes the windmill the best-looking object in the scene.
Every change stays within project constraints: legacy fixed-pipeline OpenGL, glBegin/glEnd only,
no shaders, no GLU quadrics, no glutSolidSphere. Files: `scene.h` + `xszucm00.cpp`.

---

## Current windmill: what's wrong

`DrawWindmill()` — scene.h:582–631, `DrawBlade()` — scene.h:548–580.

| Problem | Visual consequence |
|---------|-------------------|
| Flat `GL_TRIANGLE_FAN` top cap | Tower looks decapitated, zero silhouette recognisability |
| Box hub (DrawBox 0.32³) | Reads as a wooden crate bolted to the tower |
| Blades are flat tapered planks (no spar) | Look like paddles, not windmill sails |
| No door | Building has no entrance — lifeless |
| No window | No interior life, no night atmosphere |
| No stone banding | Tower is a featureless smooth cone |
| No ground ring | Tower floats on grass with a sharp ungrounded edge |
| Constant 120°/s rotation | Mechanical, no weather feel |

---

## Commit plan — ordered by visual impact

---

### Commit 1 — `feat(windmill): add conical slate cap with finial spike`

**File:** `scene.h` — inside `DrawWindmill()`, replace the `GL_TRIANGLE_FAN` top cap.

**What to build:**
- 16-segment cone, base at `y = height = 5.0`, base radius = `topR + 0.08 = 0.28` (slight overhang)
- Apex at `y = height + 1.0 = 6.0`
- Material: dark slate `(0.26, 0.26, 0.30, shin 24)`
- Per-face normals via `triNormal` — all 16 triangles
- Bottom disc ring (underside of overhang): 16-sided `GL_TRIANGLE_STRIP`, radii `topR` → `topR+0.08`, y = height
- Finial spike: `DrawBox(0.04f, 0.38f, 0.04f)` at the apex, iron-black `(0.14, 0.13, 0.15, shin 48)`

```cpp
// at the end of DrawWindmill(), after hub + blades:
const int   capSegs = 16;
const float capBase = topR + 0.08f, capH = 1.0f;
SetMaterial(0.26f, 0.26f, 0.30f, 24.0f);
glBegin(GL_TRIANGLES);
for (int i = 0; i < capSegs; i++) {
    float a0 = (float)i       / capSegs * 2.0f * (float)M_PI;
    float a1 = (float)(i + 1) / capSegs * 2.0f * (float)M_PI;
    float x0 = cosf(a0)*capBase, z0 = sinf(a0)*capBase;
    float x1 = cosf(a1)*capBase, z1 = sinf(a1)*capBase;
    triNormal(x0, height, z0,  x1, height, z1,  0, height+capH, 0);
    glVertex3f(x0, height, z0); glVertex3f(x1, height, z1); glVertex3f(0, height+capH, 0);
}
glEnd();
// underside ring (visible when looking up at overhang)
glBegin(GL_TRIANGLE_STRIP);
glNormal3f(0, -1, 0);
for (int i = 0; i <= capSegs; i++) {
    float a = (float)i / capSegs * 2.0f * (float)M_PI;
    glVertex3f(cosf(a)*topR,    height, sinf(a)*topR);
    glVertex3f(cosf(a)*capBase, height, sinf(a)*capBase);
}
glEnd();
// finial spike
SetMaterial(0.14f, 0.13f, 0.15f, 48.0f);
glPushMatrix();
    glTranslatef(0, height + capH, 0);
    DrawBox(0.04f, 0.38f, 0.04f);
glPopMatrix();
```

**Estimated effort:** ~35 lines. **Impact:** 10/10 — biggest single visual transformation.

---

### Commit 2 — `feat(windmill): replace box hub with octagonal disc and axle shaft`

**File:** `scene.h` — inside `DrawWindmill()`, replace the `DrawBox(0.32f, 0.32f, 0.32f)` hub block.

**What to build:**

1. **Axle shaft** (drawn first, sits between tower face and hub back):
   - 8-sided cylinder, radius=0.055, length 0.22 along +Z
   - Start at `(0, height, topR)`, end at `(0, height, topR + 0.22)`
   - Material: iron-dark `(0.22, 0.20, 0.18, shin 32)`
   - GL_QUADS loop for sides, GL_TRIANGLE_FAN caps

2. **Hub disc** (flat wheel face):
   - 8-sided flat cylinder, radius=0.22, depth=0.14
   - Centred at `(0, height, topR + 0.22 + 0.07)` i.e. `(0, height, topR + 0.29)`
   - Material: dark wood `(0.40, 0.30, 0.18, shin 16)`
   - Front cap: GL_TRIANGLE_FAN, back cap: GL_TRIANGLE_FAN, side wall: GL_QUADS loop

3. **Hub spoke cross** (4 flat spokes radiating from centre on the front face):
   - 4× `DrawBox(0.04, 0.44, 0.03)` rotated 0/45/90/135° around Z, drawn in hub-local space
   - Same dark wood material

```cpp
// hub helper (inline in DrawWindmill):
auto DrawDisc = [](float r, int segs, float y) {
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,0,1);
    glVertex3f(0,y,0);
    for(int i=0;i<=segs;i++){
        float a=(float)i/segs*2*(float)M_PI;
        glVertex3f(cosf(a)*r, sinf(a)*r, y);
    }
    glEnd();
};
```

**Estimated effort:** ~60 lines. **Impact:** 8/10 — the crate hub is jarring and everyone notices it.

---

### Commit 3 — `feat(windmill): redesign blades with spine spar and cross-braces`

**File:** `scene.h` — modify `DrawBlade()` and the 4-blade loop in `DrawWindmill()`.

**What to change in `DrawBlade(float len, float rootW, float depth)`:**

1. Keep existing main body geometry unchanged (so blade silhouette stays the same)
2. After the main glBegin/glEnd, add:

   a. **Central spar spine** — raised ridge on front face:
      - A very thin tapered box along the length: base width=0.07 at y=0, tip width=0.025 at y=len
      - Translated so it protrudes +0.07 from the front face (z = hd + 0.05)
      - Material: darker wood `(0.55, 0.40, 0.20, shin 16)` — call before spine draws
      - Draw as GL_QUADS (4 faces: front, back, left taper, right taper)

   b. **Two horizontal cross-braces**:
      - At y = len*0.33 and y = len*0.66
      - Width spans full blade width minus a small margin
      - `DrawBox(rootW * 0.85f - (rootW - tipW) * frac, 0.04f, depth + 0.10f)` where frac = 0.33 or 0.66
      - Same dark wood material as spine

3. **Blade tip darker accent**:
   - After drawing main blade, draw an extra thin quad over just the outer 15% of the blade face
   - Material: `(0.62, 0.48, 0.28, shin 12)` — slightly darker

**Estimated effort:** ~55 lines added to DrawBlade. **Impact:** 8/10 — transforms planks into real sail frames.

---

### Commit 4 — `feat(windmill): add stone banding rings to tower`

**File:** `scene.h` — inside `DrawWindmill()`, after the main tower loop, before the top cap.

**What to build:**
- At y = 1.0, 2.0, 3.0, 4.0: a thin protruding ring wrapping the tower
- At each height `hy`, the tower radius is: `r(hy) = baseR + (topR - baseR) * (hy / height)`
- Ring: 8-sided GL_QUADS loop, radius = `r(hy) + 0.025`, ring height = 0.06
- Material: darker stone `(0.60, 0.52, 0.40, shin 8)` — same family as tower, slightly darker

```cpp
const float bandHeights[] = {1.0f, 2.0f, 3.0f, 4.0f};
SetMaterial(0.60f, 0.52f, 0.40f, 8.0f);
for (float bh : bandHeights) {
    float br = baseR + (topR - baseR) * (bh / height) + 0.025f;
    glBegin(GL_QUADS);
    for (int i = 0; i < sides; i++) {
        float a0 = (float)i/sides*2*(float)M_PI, a1=(float)(i+1)/sides*2*(float)M_PI;
        float x0=cosf(a0)*br, z0=sinf(a0)*br, x1=cosf(a1)*br, z1=sinf(a1)*br;
        float nx=(x0+x1)*0.5f, nz=(z0+z1)*0.5f, nl=sqrtf(nx*nx+nz*nz);
        glNormal3f(nx/nl, 0, nz/nl);
        glVertex3f(x0,bh,      z0); glVertex3f(x1,bh,      z1);
        glVertex3f(x1,bh+0.06f,z1); glVertex3f(x0,bh+0.06f,z0);
    }
    glEnd();
}
```

**Estimated effort:** ~25 lines. **Impact:** 6/10 — makes the tower feel like real masonry.

---

### Commit 5 — `feat(windmill): add arched door with planks and iron hinges`

**File:** `scene.h` — inside `DrawWindmill()`, new block after the foundation ring.

**Position:** On the +Z face of the tower base. Tower front radius at y=0 is `baseR = 0.45`, so door sits at z ≈ +0.44.

**What to build:**

1. **Door stone arch frame**:
   - Rectangle part: `DrawBox(0.60f, 0.95f, 0.06f)` — `glTranslatef(0, 0, baseR-0.01f)`, light stone `(0.72, 0.64, 0.48, shin 8)`
   - Semicircular arch top: 8-triangle GL_TRIANGLE_FAN fan centred at `(0, 0.95, 0)` radius=0.30, in XY plane, just upper half

2. **Three door planks** (dark wood, slightly recessed from frame):
   - 3× `DrawBox(0.16f, 0.90f, 0.04f)` at x = -0.18, 0, +0.18, translated 0.04 forward in Z
   - Material: dark wood `(0.30, 0.20, 0.10, shin 8)`

3. **Iron strap hinges** (2, horizontal):
   - 2× `DrawBox(0.46f, 0.035f, 0.045f)` at y = 0.22 and y = 0.72
   - Material: iron `(0.20, 0.18, 0.16, shin 48)`

4. **Threshold stone**:
   - `DrawBox(0.68f, 0.05f, 0.16f)` at y=0, z = baseR + 0.06
   - Light stone `(0.72, 0.65, 0.50, shin 12)`

**Estimated effort:** ~50 lines. **Impact:** 7/10 — makes windmill feel inhabited and purposeful.

---

### Commit 6 — `feat(windmill): add stone-framed window at mid-tower`

**File:** `scene.h` — inside `DrawWindmill()`, new block.

**Position:** +Z face, height y=2.4, centred on x=0.

**What to build:**

1. **Stone surround** (4 slabs projecting slightly from tower face):
   - Top lintel:   `DrawBox(0.64f, 0.10f, 0.08f)` at y=2.98, z = faceZ + 0.02
   - Bottom sill:  `DrawBox(0.72f, 0.09f, 0.11f)` at y=2.38 (slightly wider, like a sill)
   - Left jamb:    `DrawBox(0.08f, 0.60f, 0.08f)` at x=-0.27, y=2.40
   - Right jamb:   `DrawBox(0.08f, 0.60f, 0.08f)` at x=+0.27, y=2.40
   - Material: lighter stone `(0.74, 0.68, 0.54, shin 12)`
   - `faceZ` = tower face Z at height 2.4 = `baseR + (topR-baseR)*(2.4/height) * (something) + tiny offset`
     Approx: `0.45 - 0.25*0.48 = 0.33`, so door sits at z ≈ 0.34.

2. **Glass pane** (inset behind frame):
   - `DrawBox(0.06f, 0.56f, 0.52f)` translated to sit flush with window opening
   - Material: dark glass `(0.08, 0.12, 0.22, shin 80)` — dark blue-grey

3. **Window cross-bar** (a thin iron bar dividing pane into 4):
   - `DrawBox(0.04f, 0.56f, 0.04f)` and `DrawBox(0.04f, 0.04f, 0.52f)` in iron material

**New function `DrawWindmillGlow(float wx, float wy, float wz)` (called from transparent pass in xszucm00.cpp):**
- Camera-facing quad at world position `(wx, wy+2.65, wz + 0.36)` — in front of window
- Size 0.55 × 0.55, additive blend, warm yellow `(1.0, 0.75, 0.30, 0.22)`
- Disable lighting for this billboard
- Outer soft corona: second quad size 0.90, alpha 0.08, same warm colour

**In `xszucm00.cpp` transparent pass** (after lantern halos, before depthMask restore):
```cpp
// windmill window glow (additive)
glBlendFunc(GL_SRC_ALPHA, GL_ONE);
{
    float mv[16]; glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    float rx=mv[0],ry=mv[4],rz=mv[8], ux=mv[1],uy=mv[5],uz=mv[9];
    glPushMatrix();
        glTranslatef(0, 0, -20);  // windmill world position
        DrawWindmillGlow(rx, ry, rz, ux, uy, uz);
    glPopMatrix();
}
```

**Estimated effort:** ~60 lines in scene.h, ~10 lines in xszucm00.cpp. **Impact:** 9/10 at night — the glowing window is magical.

---

### Commit 7 — `feat(windmill): add stone foundation ring and ground shadow disc`

**File:** `scene.h` — DrawWindmill() for foundation ring; new `DrawWindmillShadow()` function.
**File:** `xszucm00.cpp` — add shadow to transparent pass.

**Foundation ring (opaque, drawn in DrawWindmill at start):**
- 16-sided `GL_TRIANGLE_STRIP` ring at y=0.01
- Inner radius = baseR, outer radius = baseR + 0.32
- Material: dark cobble `(0.38, 0.33, 0.26, shin 4)`

```cpp
const int fSegs = 16;
const float fOuter = baseR + 0.32f;
SetMaterial(0.38f, 0.33f, 0.26f, 4.0f);
glBegin(GL_TRIANGLE_STRIP);
glNormal3f(0, 1, 0);
for (int i = 0; i <= fSegs; i++) {
    float a = (float)i / fSegs * 2.0f * (float)M_PI;
    glVertex3f(cosf(a)*fOuter, 0.01f, sinf(a)*fOuter);
    glVertex3f(cosf(a)*baseR,  0.01f, sinf(a)*baseR);
}
glEnd();
```

**Ground shadow (transparent pass, `DrawWindmillShadow()`):**
- 16-sided GL_TRIANGLE_FAN ellipse at y = 0.005
- Semi-axes: rx = 1.8, rz = 1.5 (slightly elliptical, elongated by sun direction)
- `glDisable(GL_LIGHTING)`, colour `(0, 0, 0, 0.38)`, standard alpha blend `GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA`

```cpp
void DrawWindmillShadow() {
    glDisable(GL_LIGHTING);
    glColor4f(0.0f, 0.0f, 0.0f, 0.38f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,1,0);
    glVertex3f(0, 0.005f, 0);
    for (int i = 0; i <= 16; i++) {
        float a = (float)i / 16 * 2.0f * (float)M_PI;
        glVertex3f(cosf(a)*1.8f, 0.005f, sinf(a)*1.5f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}
```

**In `xszucm00.cpp` transparent pass** — before lantern windows section:
```cpp
// windmill shadow: alpha blend, under everything
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
glPushMatrix();
    glTranslatef(0, 0, -20);
    DrawWindmillShadow();
glPopMatrix();
```

**Estimated effort:** ~35 lines total. **Impact:** 6/10 — grounds the windmill, adds depth.

---

### Commit 8 — `feat(windmill): add variable wind speed to blade rotation`

**File:** `xszucm00.cpp` — inside `OnTimer()`, replace line 212:

**Current:**
```cpp
if (animOn) windmillAngle += 120.0f * dt;
```

**New:**
```cpp
if (animOn) {
    float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    float wind = 0.65f + 0.35f * sinf(t * 0.11f) * (0.7f + 0.3f * sinf(t * 0.07f));
    windmillAngle += 110.0f * dt * wind;
}
```

This produces a gentle, organic speed variation (0.195× to 1.0× baseline) with two overlapping sine waves so it never feels periodic/mechanical.

**Estimated effort:** ~5 lines. **Impact:** 4/10 but zero risk — purely an animation polish.

---

### Commit 9 — `feat(windmill): add iron ladder on tower rear face`

**File:** `scene.h` — inside `DrawWindmill()`, new block after foundation ring.

**What to build:**
- Two vertical side rails: `DrawBox(0.02f, 4.0f, 0.04f)` at x = ±0.14, z = -(baseR lerp to topR at 2.0) - 0.02
  Since rails run the full height, use the mid-tower Z offset ≈ -(baseR + topR)/2 = -0.325
- 12 horizontal rungs evenly from y=0.3 to y=4.0:
  - `DrawBox(0.28f, 0.025f, 0.04f)` at each rung y, same z as rails
- Material: iron `(0.22, 0.20, 0.17, shin 32)` — wrought iron look

```cpp
SetMaterial(0.22f, 0.20f, 0.17f, 32.0f);
const float ldrZ = -(baseR + topR) * 0.5f - 0.015f; // average wall face
// rails
for (float rx : {-0.14f, 0.14f}) {
    glPushMatrix(); glTranslatef(rx, 0, ldrZ); DrawBox(0.02f,4.0f,0.04f); glPopMatrix();
}
// rungs
for (int r = 0; r < 12; r++) {
    float ry = 0.30f + r * (3.7f / 11.0f);
    glPushMatrix(); glTranslatef(0, ry, ldrZ); DrawBox(0.28f, 0.025f, 0.04f); glPopMatrix();
}
```

**Estimated effort:** ~20 lines. **Impact:** 5/10 — functional detail that adds narrative (someone works up there).

---

## Execution checklist

| # | Commit | Scene.h | xszucm00.cpp | Est. lines | Done |
|---|--------|---------|--------------|------------|------|
| 1 | Conical slate cap + finial | DrawWindmill | — | ~35 | [x] |
| 2 | Disc hub + axle shaft | DrawWindmill | — | ~60 | [x] |
| 3 | Blade spine spar + cross-braces | DrawBlade | — | ~55 | [x] |
| 4 | Stone banding rings | DrawWindmill | — | ~25 | [x] |
| 5 | Arched door with planks + hinges | DrawWindmill | — | ~50 | [x] |
| 6 | Stone window + interior glow | DrawWindmill + new DrawWindmillGlow | transparent pass | ~70 | [x] |
| 7 | Foundation ring + shadow disc | DrawWindmill + new DrawWindmillShadow | transparent pass | ~35 | [x] |
| 8 | Variable wind speed | — | OnTimer | ~5 | [x] |
| 9 | Iron ladder on rear face | DrawWindmill | — | ~20 | [x] |

---

## Architecture notes

**Transparent pass order in OnDisplay (xszucm00.cpp):**
```
glEnable(GL_BLEND);
glDepthMask(GL_FALSE);
  1. alpha blend  → windmill shadow disc           GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
  2. alpha blend  → lantern windows               (same)
  3. additive     → lantern glow halos            GL_SRC_ALPHA, GL_ONE
  4. additive     → windmill window glow          (same)
glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
```

**`DrawWindmillGlow` signature (scene.h):**
```cpp
void DrawWindmillGlow(float rx, float ry, float rz, float ux, float uy, float uz);
// rx/ry/rz = camera right world vector, ux/uy/uz = camera up world vector
// call inside windmill-local transform (already translated to windmill world pos)
```

**`DrawWindmillShadow` signature (scene.h):**
```cpp
void DrawWindmillShadow();
// call inside windmill-local transform
```

---

## Constraints reminder

- No external textures unless sips-converted BMP ≤ 1024×1024
- No glutSolidSphere / GLU quadrics — all geometry via glBegin
- Every lit face needs a normal (glNormal before vertex)
- Draw order: sky → opaque → transparent (depthMask off) → HUD
- Fog is always on; disable only inside DrawSky
- Commit format: `feat(windmill): short imperative description` — one concern per commit
- No AI trailer in commits — use `git commit -s -S`
