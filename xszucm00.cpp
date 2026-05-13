/*
 * MPC-MPG 2025/26 - Semester project
 *
 * Author:      Martin Szüč
 * Student ID:  231284
 * Login:       xszucm00
 * Email:       xszucm00@vut.cz
 *
 * Project name: Night Park
 *
 * Implemented tasks:
 *   1.  Object modelling (5+ custom objects)       3 b
 *         tree, bench, lantern, shed, boulder, fence, windmill,
 *         fountain, bridge, ducks, fireflies
 *   2.  Animation (windmill rotation)              1 b
 *   3.  Lighting + normals                         1 b
 *         GL_LIGHT0 moon (directional), GL_LIGHT1 torch (spotlight),
 *         GL_LIGHT2/3 lantern point lights, GL_LIGHT4 fountain
 *   4.  Free movement (WASD + arrows + mouse look) 1 b
 *   5.  Menu (right-click, 6+ items)               2 b
 *   6.  Text display (HUD — position, FPS, action) 2 b
 *   7.  Handheld torch (GL_LIGHT1 spotlight, key R) 2 b
 *   10. Ascend/descend (Page Up/Down)              1 b
 *   11. Throw object (Space, projectile + gravity) 2 b
 *   12. Step simulation (camera bobbing)           2 b
 *   14. Transparency (lantern window, alpha blend) 1 b
 *   16. Texturing (6 BMP textures)                 2 b
 *         grass.bmp, checker (procedural), treebark.bmp,
 *         woodplank.bmp, rooftiles.bmp, water.bmp
 *   17. Bezier patches (terrain surface)           2 b
 *                                              TOTAL: 24 b
 *
 * Controls:
 *   W/S/A/D or arrows   move forward/back/strafe
 *   mouse drag          look around (pitch + yaw)
 *   R                   torch ON/OFF
 *   Space               throw object (gravity-affected)
 *   Page Up/Down        raise/lower camera height
 *   P                   print camera position to stdout
 *   right mouse button  context menu
 *   Esc                 quit
 *
 * Custom ideas / extensions beyond base requirements:
 *   - Night atmosphere: procedural star field (200 stars, twinkle animation),
 *     moon billboard with glow halo, atmospheric fog (GL_EXP2)
 *   - Fireflies: 8 wandering point glows with randomised blink phases
 *   - Fountain: rippling water surface, animated jet column, basin light
 *   - Lake with textured animated water surface, reed plants, duck models,
 *     wooden bridge with raised deck and staircases at both ends
 *   - Windmill: tapered octagonal tower, 4 canvas sails with lattice frames,
 *     animated rotation, ground shadow + glow pass
 *   - Impact flashes on projectile landing
 *   - Real delta-time physics (glutElapsedTime, 50 ms clamp)
 *   - Lantern point lights (GL_LIGHT2/3) with warm attenuation
 *   - Bark, woodplank, rooftile, water BMP textures on scene objects
 *
 * Platform / configuration:
 *   macOS 15, Apple M4 Pro, CLion 2025, OpenGL 2.1 + GLUT framework
 *   Also tested: Ubuntu 22.04 + Mesa (CI, headless via Xvfb)
 */

#include "scene.h"
#include <cstdarg>
#ifdef _WIN32
#  include <direct.h>
#  define getcwd _getcwd
#else
#  include <unistd.h>
#endif

// ─── logging ─────────────────────────────────────────────────────────────────

// Log(category, fmt, ...) — prints "[CATEGORY] message\n" to stdout, flushed.
static void Log(const char* cat, const char* fmt, ...) {
    printf("[%-5s] ", cat);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    printf("\n");
    fflush(stdout);
}

// CheckGL(where) — prints any pending GL error, or "OK" if none.
static void CheckGL(const char* where) {
    GLenum e = glGetError();
    if (e != GL_NO_ERROR)
        Log("GL", "ERROR after %s: 0x%04X", where, e);
    else
        Log("GL", "%s: OK", where);
}

// ─── CI screenshot mode ───────────────────────────────────────────────────────

static int ciScenario = -1;

static void SaveBMP(const char* path, int w, int h) {
    std::vector<uint8_t> px(w * h * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());

    int stride   = (w * 3 + 3) & ~3;
    int dataSize = stride * h;

    uint8_t hdr[54] = {};
    auto wl = [&](int o, uint32_t v){ memcpy(hdr+o, &v, 4); };
    auto ws = [&](int o, uint16_t v){ memcpy(hdr+o, &v, 2); };
    hdr[0]='B'; hdr[1]='M';
    wl(2,  54 + dataSize); wl(10, 54);
    wl(14, 40); wl(18, (uint32_t)w); wl(22, (uint32_t)h);
    ws(26, 1);  ws(28, 24);
    wl(34, (uint32_t)dataSize);

    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "SaveBMP: cannot open %s\n", path); return; }
    fwrite(hdr, 1, 54, f);
    std::vector<uint8_t> row(stride, 0);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            row[x*3+0] = px[(y*w+x)*3+2];
            row[x*3+1] = px[(y*w+x)*3+1];
            row[x*3+2] = px[(y*w+x)*3+0];
        }
        fwrite(row.data(), 1, stride, f);
    }
    fclose(f);
}

// Returns rel if it exists in cwd, else tries "../rel" (for CLion cmake-build-debug runs).
static std::string findAsset(const char* rel) {
    FILE* f = fopen(rel, "rb");
    if (f) { fclose(f); return rel; }
    std::string up = std::string("../") + rel;
    f = fopen(up.c_str(), "rb");
    if (f) { fclose(f); return up; }
    return rel;
}

// ─── GLUT callbacks ──────────────────────────────────────────────────────────

void OnReshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / h, 0.1, 500.0);
}

void OnInit() {
    // ── startup diagnostics ──────────────────────────────────────────────────
    char cwd[512] = "(unknown)";
    getcwd(cwd, sizeof(cwd));
    Log("INIT", "=== Night Park — OnInit ===");
    Log("INIT", "working directory : %s", cwd);
    Log("INIT", "GL vendor         : %s", glGetString(GL_VENDOR));
    Log("INIT", "GL renderer       : %s", glGetString(GL_RENDERER));
    Log("INIT", "GL version        : %s", glGetString(GL_VERSION));

    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    GLfloat globalAmb[] = {0.25f, 0.25f, 0.38f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    // GL_LIGHT0 — moon (directional, blue-white)
    GLfloat amb0[]  = {0.28f, 0.28f, 0.45f, 1.0f};
    GLfloat diff0[] = {0.85f, 0.85f, 1.00f, 1.0f};
    GLfloat spec0[] = {0.30f, 0.30f, 0.50f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
    glEnable(GL_LIGHT0);
    Log("INIT", "GL_LIGHT0 (moon) enabled — directional blue-white");

    // GL_LIGHT1 — torch spotlight (warm, starts off)
    // Wide 45° cutoff + low attenuation so the ground pool is obvious.
    GLfloat amb1[]  = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat diff1[] = {1.6f, 1.3f, 0.9f, 1.0f};
    GLfloat spec1[] = {0.8f, 0.7f, 0.5f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT,  amb1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  diff1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec1);
    glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,            45.0f);
    glLightf (GL_LIGHT1, GL_SPOT_EXPONENT,            2.0f);
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,    0.4f);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,      0.02f);
    glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION,   0.003f);
    glDisable(GL_LIGHT1);
    Log("INIT", "GL_LIGHT1 (torch) configured — cutoff=45 exp=2, starts OFF");

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // GL_LIGHT2 / GL_LIGHT3 — lantern point lights (warm yellow, tight radius)
    GLfloat lAmb[] = {0.0f,  0.0f,  0.0f,  1.0f};
    GLfloat lDif[] = {1.0f,  0.85f, 0.40f, 1.0f};
    GLfloat lSpc[] = {0.30f, 0.25f, 0.10f, 1.0f};
    for (int li : {GL_LIGHT2, GL_LIGHT3}) {
        glLightfv(li, GL_AMBIENT,               lAmb);
        glLightfv(li, GL_DIFFUSE,               lDif);
        glLightfv(li, GL_SPECULAR,              lSpc);
        glLightf (li, GL_CONSTANT_ATTENUATION,  0.2f);
        glLightf (li, GL_LINEAR_ATTENUATION,    0.10f);
        glLightf (li, GL_QUADRATIC_ATTENUATION, 0.04f);
        glEnable(li);
    }
    Log("INIT", "GL_LIGHT2/3 (lanterns) enabled — warm yellow, attenuation on");

    // GL_LIGHT4 — fountain basin (blue-white point, low attenuation so plaza glows)
    GLfloat fAmb[]  = {0.00f, 0.00f, 0.00f, 1.0f};
    GLfloat fDif[]  = {0.70f, 0.90f, 1.40f, 1.0f};
    GLfloat fSpc[]  = {0.80f, 0.95f, 1.20f, 1.0f};
    glLightfv(GL_LIGHT4, GL_AMBIENT,               fAmb);
    glLightfv(GL_LIGHT4, GL_DIFFUSE,               fDif);
    glLightfv(GL_LIGHT4, GL_SPECULAR,              fSpc);
    glLightf (GL_LIGHT4, GL_CONSTANT_ATTENUATION,  0.10f);
    glLightf (GL_LIGHT4, GL_LINEAR_ATTENUATION,    0.06f);
    glLightf (GL_LIGHT4, GL_QUADRATIC_ATTENUATION, 0.02f);
    glEnable (GL_LIGHT4);
    Log("INIT", "GL_LIGHT4 (fountain) enabled — bright blue-white, low attenuation");

    // atmospheric fog — dark blue-black, fades objects beyond ~40 units
    GLfloat fogColor[] = {0.03f, 0.03f, 0.10f, 1.0f};
    glEnable(GL_FOG);
    glFogfv(GL_FOG_COLOR,   fogColor);
    glFogi (GL_FOG_MODE,    GL_EXP2);
    glFogf (GL_FOG_DENSITY, kFogDensity);
    glHint (GL_FOG_HINT,    GL_NICEST);

    // ── procedural checker texture (64×64) ──────────────────────────────────
    Log("TEX", "generating procedural checker texture (64x64)...");
    unsigned char checker[64 * 64 * 3];
    for (int i = 0; i < 64; i++)
        for (int j = 0; j < 64; j++) {
            int idx = (i * 64 + j) * 3;
            unsigned char c = ((i / 8 + j / 8) % 2) ? 200 : 80;
            checker[idx] = checker[idx+1] = checker[idx+2] = c;
        }
    glGenTextures(1, &texChecker);
    glBindTexture(GL_TEXTURE_2D, texChecker);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, 64, 64, GL_RGB, GL_UNSIGNED_BYTE, checker);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    Log("TEX", "checker texture ID=%u  glIsTexture=%s",
        texChecker, glIsTexture(texChecker) ? "true" : "false");
    CheckGL("checker texture");

    // ── grass BMP texture ────────────────────────────────────────────────────
    {
        std::string grassPath = findAsset("assets/textures/grass.bmp");
        Log("TEX", "loading grass texture: %s", grassPath.c_str());

        bool ok = setTexture(grassPath.c_str(), &texGrass, true);
        Log("TEX", "  setTexture() returned: %s", ok ? "SUCCESS" : "FAILED");
        Log("TEX", "  texGrass ID=%u  glIsTexture=%s",
            texGrass, glIsTexture(texGrass) ? "true" : "false");

        // imageLoad.h does not set filter params — apply them explicitly
        if (ok && texGrass) {
            glBindTexture(GL_TEXTURE_2D, texGrass);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            Log("TEX", "  filter params set (LINEAR_MIPMAP_LINEAR / REPEAT)");
        }
        CheckGL("grass texture");
    }

    // ── additional textures (bark, rooftile, woodplank) ─────────────────────
    {
        struct { const char* rel; unsigned int* id; } extras[] = {
            {"assets/textures/treebark.bmp",  &texBark},
            {"assets/textures/rooftiles.bmp", &texRooftile},
            {"assets/textures/woodplank.bmp", &texWoodplank},
            {"assets/textures/water.bmp",     &texWater},
        };
        for (auto& e : extras) {
            std::string p = findAsset(e.rel);
            bool ok = setTexture(p.c_str(), e.id, true);
            Log("TEX", "  %s -> ID=%u  %s", p.c_str(), *e.id, ok ? "OK" : "FAILED");
            if (ok && *e.id) {
                glBindTexture(GL_TEXTURE_2D, *e.id);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }
        }
        CheckGL("additional textures");
    }

    // star field — seeded once so CI screenshots are pixel-perfect
    srand(42);
    for (int i = 0; i < kStarCount; i++) {
        float theta = ((float)rand() / RAND_MAX) * 2.0f * (float)M_PI;
        float phi   = ((float)rand() / RAND_MAX) * (float)M_PI * 0.5f;
        starVerts[i][0] = kStarRadius * cosf(phi) * cosf(theta);
        starVerts[i][1] = kStarRadius * sinf(phi);
        starVerts[i][2] = kStarRadius * cosf(phi) * sinf(theta);
        starPhases[i] = ((float)rand() / RAND_MAX) * 6.28f;
        starSpeeds[i] = 0.8f + ((float)rand() / RAND_MAX) * 2.5f;
        // 70% small (0), 20% medium (1), 10% large (2)
        int sr = rand() % 10;
        starSizes[i] = (sr < 7) ? 0.0f : (sr < 9) ? 1.0f : 2.0f;
    }

    // firefly positions and blink phases
    for (int i = 0; i < kFireflyCount; i++) {
        fireflies[i].x           = -15.0f + (i % 4) * 8.0f;
        fireflies[i].y           = 0.6f;
        fireflies[i].z           = -10.0f + (i / 4) * 8.0f;
        fireflies[i].phase       = (float)i * 1.57f;  // π/2 apart
        fireflies[i].speed       = 1.2f + (i % 3) * 0.4f;
        fireflies[i].wanderAngle = 0.0f;
    }

    Log("INIT", "stars=%d  fireflies=%d", kStarCount, kFireflyCount);
    Log("INIT", "=== OnInit complete ===");
}

void OnTimer(int) {
    static int lastMs = 0;
    int nowMs   = glutGet(GLUT_ELAPSED_TIME);
    float dt    = (lastMs == 0) ? 0.016f : (nowMs - lastMs) * 0.001f;
    if (dt > 0.05f) dt = 0.05f; // clamp: ignore stalls longer than 50ms
    lastMs = nowMs;

    float dx = 0.0f, dz = 0.0f;

    if (keys['w'] || keys['W'] || keys[GLUT_KEY_UP    + 200]) { dx -= sinf(yaw) * kCamSpeed * dt; dz -= cosf(yaw) * kCamSpeed * dt; }
    if (keys['s'] || keys['S'] || keys[GLUT_KEY_DOWN  + 200]) { dx += sinf(yaw) * kCamSpeed * dt; dz += cosf(yaw) * kCamSpeed * dt; }
    if (keys['a'] || keys['A'] || keys[GLUT_KEY_LEFT  + 200]) { dx -= cosf(yaw) * kCamSpeed * dt; dz += sinf(yaw) * kCamSpeed * dt; }
    if (keys['d'] || keys['D'] || keys[GLUT_KEY_RIGHT + 200]) { dx += cosf(yaw) * kCamSpeed * dt; dz -= sinf(yaw) * kCamSpeed * dt; }

    camX += dx;
    camZ += dz;
    isMoving = (dx * dx + dz * dz > 0.0001f);

    if (isMoving) {
        bobTimer += dt;
        bobOffset = sinf(bobTimer * kBobFrequency) * kBobAmplitude;
    } else {
        bobOffset *= 0.80f;
        if (fabsf(bobOffset) < 0.001f) bobOffset = 0.0f;
    }

    if (animOn) {
        float t    = nowMs * 0.001f;
        float wind = 0.65f + 0.35f * sinf(t * 0.11f) * (0.7f + 0.3f * sinf(t * 0.07f));
        windmillAngle += 45.0f * dt * wind; // believable park-windmill speed
    }

    rippleTime += dt;

    for (auto& p : projectiles) {
        if (!p.active) continue;
        p.vy  -= 9.8f * dt;
        p.x   += p.vx * dt;
        p.y   += p.vy * dt;
        p.z   += p.vz * dt;
        p.rot += 200.0f * dt;
        if (p.y <= 0.0f) {              // hit the ground — spawn flash
            SpawnImpact(p.x, 0.0f, p.z);
            p.active = false;
        }
        if (p.y < -10.0f) p.active = false;
    }

    // age impact flashes
    for (auto& f : impacts) {
        if (!f.active) continue;
        f.life -= dt * 2.2f;            // fades in ~0.45 s
        if (f.life <= 0.0f) f.active = false;
    }

    // torch flicker — dual-sine so it feels organic
    if (torchOn) {
        float flicker = 0.85f + 0.15f * sinf(nowMs * 0.037f) * sinf(nowMs * 0.019f);
        GLfloat fd[] = {1.6f * flicker, 1.3f * flicker, 0.9f * flicker, 1.0f};
        glLightfv(GL_LIGHT1, GL_DIFFUSE, fd);
    }

    // lantern pulse — slow warm breath
    {
        float lb = 1.0f + 0.08f * sinf(nowMs * 0.0013f);
        GLfloat ld[] = {1.0f * lb, 0.85f * lb, 0.40f * lb, 1.0f};
        glLightfv(GL_LIGHT2, GL_DIFFUSE, ld);
        glLightfv(GL_LIGHT3, GL_DIFFUSE, ld);
    }

    // firefly position update
    {
        float tff = nowMs * 0.001f;
        for (int i = 0; i < kFireflyCount; i++) {
            auto& f        = fireflies[i];
            f.wanderAngle  = f.phase + sinf(tff * f.speed) * 0.8f;
            f.x            = -12.0f + 24.0f * (0.5f + 0.5f * sinf(f.wanderAngle));
            f.z            = -18.0f + 28.0f * (0.5f + 0.5f * cosf(f.wanderAngle * 1.3f));
            f.y            = 0.6f + 0.5f * sinf(tff * f.speed * 0.7f + f.phase * 1.3f);
        }
    }

#ifdef DEBUG_CAM
    static int debugTick = 0;
    if (++debugTick >= 312) {
        debugTick = 0;
        printf("[CAM] x=%.2f  y=%.2f  z=%.2f  yaw=%.2f deg  pitch=%.2f deg\n",
               camX, camFloorY, camZ,
               yaw   * (180.0f / (float)M_PI),
               pitch * (180.0f / (float)M_PI));
        fflush(stdout);
    }
#endif

    glutPostRedisplay();
    glutTimerFunc(16, OnTimer, 0);
}

void OnSpecial(int key, int, int) {
    if (key == GLUT_KEY_UP)    keys[GLUT_KEY_UP    + 200] = true;
    if (key == GLUT_KEY_DOWN)  keys[GLUT_KEY_DOWN  + 200] = true;
    if (key == GLUT_KEY_LEFT)  keys[GLUT_KEY_LEFT  + 200] = true;
    if (key == GLUT_KEY_RIGHT) keys[GLUT_KEY_RIGHT + 200] = true;
    if (key == GLUT_KEY_PAGE_UP)   { camFloorY += 0.5f; if (camFloorY >  30.0f) camFloorY =  30.0f; lastAction = "camera up";   Log("KEY", "PageUp   -> camFloorY=%.2f", camFloorY); }
    if (key == GLUT_KEY_PAGE_DOWN) { camFloorY -= 0.5f; if (camFloorY < -5.0f)  camFloorY = -5.0f;  lastAction = "camera down"; Log("KEY", "PageDown -> camFloorY=%.2f", camFloorY); }
    glutPostRedisplay();
}

void OnSpecialUp(int key, int, int) {
    if (key == GLUT_KEY_UP)    keys[GLUT_KEY_UP    + 200] = false;
    if (key == GLUT_KEY_DOWN)  keys[GLUT_KEY_DOWN  + 200] = false;
    if (key == GLUT_KEY_LEFT)  keys[GLUT_KEY_LEFT  + 200] = false;
    if (key == GLUT_KEY_RIGHT) keys[GLUT_KEY_RIGHT + 200] = false;
}

void OnMouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseDown  = (state == GLUT_DOWN);
        mouseLastX = x;
        mouseLastY = y;
    }
}

void OnMouseMotion(int x, int y) {
    if (!mouseDown) return;
    const float sens = 0.005f;
    yaw   += (x - mouseLastX) * sens;
    pitch -= (y - mouseLastY) * sens;
    if (pitch >  1.5f) pitch =  1.5f;
    if (pitch < -1.5f) pitch = -1.5f;
    mouseLastX = x;
    mouseLastY = y;
    glutPostRedisplay();
}

void OnPassiveMotion(int, int) {}

void OnKeyboard(unsigned char key, int, int) {
    keys[key] = true;
    if (key == 27) { Log("KEY", "Esc -> exit"); exit(0); }
    if (key == 'r' || key == 'R') {
        torchOn = !torchOn;
        lastAction = torchOn ? "torch ON" : "torch OFF";
        Log("KEY", "R -> torch %s", torchOn ? "ON" : "OFF");
    }
    if (key == ' ') {
        SpawnProjectile();
        Log("KEY", "Space -> projectile thrown from (%.2f, %.2f, %.2f) yaw=%.1f deg",
            camX, camFloorY + bobOffset, camZ,
            yaw * (180.0f / (float)M_PI));
    }
    if (key == 'p' || key == 'P') {
        printf("[CAM] x=%.2f  y=%.2f  z=%.2f  yaw=%.2f deg  pitch=%.2f deg\n",
               camX, camFloorY, camZ,
               yaw   * (180.0f / (float)M_PI),
               pitch * (180.0f / (float)M_PI));
        fflush(stdout);
    }
    // Log non-movement action keys (skip held W/A/S/D/arrows which spam)
    if (key != 'w' && key != 'W' && key != 's' && key != 'S' &&
        key != 'a' && key != 'A' && key != 'd' && key != 'D' &&
        key != ' ' && key != 'r' && key != 'R' &&
        key != 'p' && key != 'P' && key != 27) {
        if (key >= 32 && key < 127)
            Log("KEY", "key '%c' (0x%02X) pressed", key, (unsigned)key);
        else
            Log("KEY", "key 0x%02X pressed", (unsigned)key);
    }
}

void OnKeyboardUp(unsigned char key, int, int) {
    keys[key] = false;
}

// ─── menu ────────────────────────────────────────────────────────────────────

void OnMenu(int val) {
    switch (val) {
        case 1:
            camX = 0; camFloorY = 1.7f; camZ = 20.0f;
            yaw = 0; pitch = 0;
            lastAction = "camera reset";
            Log("MENU", "Reset camera -> pos (0, 1.7, 20)");
            break;
        case 2:
            animOn = !animOn;
            lastAction = animOn ? "animation ON" : "animation OFF";
            Log("MENU", "Animation -> %s", animOn ? "ON" : "OFF");
            break;
        case 3:
            texOn = !texOn;
            lastAction = texOn ? "textures ON" : "textures OFF";
            Log("MENU", "Textures -> %s  (texGrass=%u texChecker=%u)",
                texOn ? "ON" : "OFF", texGrass, texChecker);
            break;
        case 4:
            if (glIsEnabled(GL_LIGHT0)) { glDisable(GL_LIGHT0); lastAction = "light OFF"; Log("MENU", "Moon light -> OFF"); }
            else                        { glEnable (GL_LIGHT0); lastAction = "light ON";  Log("MENU", "Moon light -> ON");  }
            break;
        case 5:
            torchOn = !torchOn;
            lastAction = torchOn ? "torch ON" : "torch OFF";
            Log("MENU", "Torch -> %s", torchOn ? "ON" : "OFF");
            break;
        case 6: {
            bool on = glIsEnabled(GL_LIGHT2);
            on ? glDisable(GL_LIGHT2) : glEnable(GL_LIGHT2);
            on ? glDisable(GL_LIGHT3) : glEnable(GL_LIGHT3);
            lastAction = on ? "lanterns OFF" : "lanterns ON";
            Log("MENU", "Lanterns -> %s", on ? "OFF" : "ON");
            break;
        }
        case 8: {
            bool on = glIsEnabled(GL_LIGHT4);
            on ? glDisable(GL_LIGHT4) : glEnable(GL_LIGHT4);
            lastAction = on ? "fountain light OFF" : "fountain light ON";
            Log("MENU", "Fountain light -> %s", on ? "OFF" : "ON");
            break;
        }
        case 7:
            Log("MENU", "Exit selected -> quit");
            exit(0);
    }
    glutPostRedisplay();
}

// ─── display ─────────────────────────────────────────────────────────────────

void OnDisplay() {
    glClearColor(0.03f, 0.03f, 0.10f, 1.0f); // matches fog color to avoid horizon seam
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);

    // sky pass — rotation only, no translation → stars/moon at true infinity
    glLoadIdentity();
    glRotatef(-pitch * (180.0f / (float)M_PI), 1.0f, 0.0f, 0.0f);
    glRotatef(-yaw   * (180.0f / (float)M_PI), 0.0f, 1.0f, 0.0f);
    DrawSky();

    // world pass — full camera transform
    glLoadIdentity();
    glRotatef(-pitch * (180.0f / (float)M_PI), 1.0f, 0.0f, 0.0f);
    glRotatef(-yaw   * (180.0f / (float)M_PI), 0.0f, 1.0f, 0.0f);
    glTranslatef(-camX, -(camFloorY + bobOffset), -camZ);

    GLfloat moonPos[] = {0.3f, 1.0f, 0.2f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, moonPos);

    // lantern point lights — positional (w=1), placed at lantern head height
    const GLenum lanternLights[] = {GL_LIGHT2, GL_LIGHT3};
    for (int i = 0; i < kLanternCount; i++) {
        GLfloat lp[] = {kLanterns[i].x, 4.2f, kLanterns[i].z, 1.0f};
        glLightfv(lanternLights[i], GL_POSITION, lp);
    }

    // fountain light — above basin rim so it casts outward onto the stone plaza
    {
        GLfloat fp[] = {0.0f, 1.80f, -2.0f, 1.0f};
        glLightfv(GL_LIGHT4, GL_POSITION, fp);
    }

    if (torchOn) {
        glEnable(GL_LIGHT1);
        GLfloat lpos[] = {camX, camFloorY + bobOffset, camZ, 1.0f};
        // Tilt the beam 15° below the camera look direction so it always
        // illuminates the ground in front even when looking straight ahead.
        // Without this bias, pitch≈0 → direction horizontal → 90° from ground
        // normal → zero diffuse contribution on the terrain.
        const float kDownTilt = 0.26f;  // ~15 degrees in radians
        float dp = pitch - kDownTilt;
        GLfloat ldir[] = {
            -sinf(yaw) * cosf(dp),
             sinf(dp),
            -cosf(yaw) * cosf(dp)
        };
        glLightfv(GL_LIGHT1, GL_POSITION,       lpos);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, ldir);
    } else {
        glDisable(GL_LIGHT1);
    }

    DrawTerrain();
    DrawPath();
    DrawScene();
    DrawProjectiles();

    // transparent pass — lantern windows + glow halos + windmill shadow/glow
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);

    // 1) windmill ground shadow + lantern windows: alpha blend
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix();
        glTranslatef(-4, 0, -24);
        DrawWindmillShadow();
    glPopMatrix();
    for (auto& l : kLanterns) {
        glPushMatrix();
            glTranslatef(l.x, l.y, l.z);
            DrawLanternWindow();
        glPopMatrix();
    }

    // 2) additive: lantern glow halos + windmill window glow
    {
        float mv[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mv);
        float rx = mv[0], ry = mv[4], rz = mv[8];
        float ux = mv[1], uy = mv[5], uz = mv[9];
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        for (auto& l : kLanterns) {
            glPushMatrix();
                glTranslatef(l.x, l.y, l.z);
                DrawLanternGlow(rx, ry, rz, ux, uy, uz);
            glPopMatrix();
        }
        glPushMatrix();
            glTranslatef(-4, 0, -24);
            DrawWindmillGlow(rx, ry, rz, ux, uy, uz);
        glPopMatrix();

        // fountain water jet (additive glow) — matches fountain Y offset
        glPushMatrix();
            glTranslatef(0, 0.14f, -2);
            DrawFountainWater();
        glPopMatrix();

        // lake moon shimmer (additive — soft reflection on water)
        DrawLakeMoonShimmer();

        // projectile impact flashes
        DrawImpacts();
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    DrawHUD();

    if (ciScenario >= 0) {
        char path[64];
        snprintf(path, sizeof(path), "ci_shot_%d.bmp", ciScenario);
        SaveBMP(path, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
    }
    glutSwapBuffers();
    if (ciScenario >= 0) exit(0);
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("MPC-MPG 2025/26 — Night Park | xszucm00");

    glutDisplayFunc(OnDisplay);
    glutReshapeFunc(OnReshape);
    glutMouseFunc(OnMouseButton);
    glutMotionFunc(OnMouseMotion);
    glutPassiveMotionFunc(OnPassiveMotion);
    glutKeyboardFunc(OnKeyboard);
    glutKeyboardUpFunc(OnKeyboardUp);
    glutSpecialFunc(OnSpecial);
    glutSpecialUpFunc(OnSpecialUp);

    int menu = glutCreateMenu(OnMenu);
    glutAddMenuEntry("Reset camera",     1);
    glutAddMenuEntry("Animation ON/OFF", 2);
    glutAddMenuEntry("Textures ON/OFF",  3);
    glutAddMenuEntry("Light ON/OFF",     4);
    glutAddMenuEntry("Torch ON/OFF",     5);
    glutAddMenuEntry("Lanterns ON/OFF",       6);
    glutAddMenuEntry("Fountain light ON/OFF", 8);
    glutAddMenuEntry("Exit",                  7);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    OnInit();

    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--ci-scenario") == 0) {
            ciScenario = atoi(argv[i + 1]);
            struct { float x, floorY, z, yaw, pitch; } scenarios[] = {
                { 6.56f, 1.70f,  6.69f, 0.07994f, 0.02007f },
                { 5.02f, 1.70f, 17.15f, 0.79013f, 0.00506f },
                { 1.75f, 1.70f,  5.37f, 0.14993f, 0.02496f },
            };
            if (ciScenario >= 0 && ciScenario < 3) {
                camX      = scenarios[ciScenario].x;
                camFloorY = scenarios[ciScenario].floorY;
                camZ      = scenarios[ciScenario].z;
                yaw       = scenarios[ciScenario].yaw;
                pitch     = scenarios[ciScenario].pitch;
            }
        }
    }

    glutTimerFunc(16, OnTimer, 0);
    glutMainLoop();
    return 0;
}
