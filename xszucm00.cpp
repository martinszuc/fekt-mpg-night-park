/*
 * MPC-MPG 2025/26 - Semester project
 *
 * Author:      Martin Szüč
 * Student ID:  (231284)
 * Email:       xszucm00@vut.cz
 *
 * Project name: Night Park
 *
 * Implemented tasks:
 *   1.  Object modelling (5+ custom objects)       3 b
 *   2.  Animation (windmill)                       1 b
 *   3.  Lighting + normals                         1 b
 *   4.  Free movement (mouse + WASD/arrows)        1 b
 *   5.  Menu (6 items)                             2 b
 *   6.  Text display (HUD)                         2 b
 *   7.  Handheld torch (key R)                     2 b
 *   10. Ascend/descend (Page Up/Down)              1 b
 *   12. Step simulation (camera bobbing)           2 b
 *   14. Transparency (lantern window)              1 b
 *   16. Texturing (grass.bmp + checker)            2 b
 *   17. Bezier patches (terrain)                   2 b
 *   11. Throw object (Space + gravity)             2 b
 *                                              TOTAL: 24 b
 *
 * Controls:
 *   W/S/A/D or arrows   move
 *   mouse drag          look around
 *   R                   torch ON/OFF
 *   Space               throw object
 *   Page Up/Down        camera height
 *   right mouse button  menu
 *   Esc                 quit
 *
 * Platform: macOS 14, CLion, OpenGL/GLUT (framework)
 */

#include "scene.h"

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

// ─── GLUT callbacks ──────────────────────────────────────────────────────────

void OnReshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / h, 0.1, 500.0);
}

void OnInit() {
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    GLfloat globalAmb[] = {0.18f, 0.18f, 0.28f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    // GL_LIGHT0 — moon (directional, blue-white)
    GLfloat amb0[]  = {0.20f, 0.20f, 0.35f, 1.0f};
    GLfloat diff0[] = {0.70f, 0.70f, 0.90f, 1.0f};
    GLfloat spec0[] = {0.25f, 0.25f, 0.40f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
    glEnable(GL_LIGHT0);

    // GL_LIGHT1 — torch spotlight (warm, starts off)
    GLfloat amb1[]  = {0.0f, 0.0f, 0.0f, 1.0f};
    GLfloat diff1[] = {1.0f, 0.9f, 0.7f, 1.0f};
    GLfloat spec1[] = {0.5f, 0.5f, 0.4f, 1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT,  amb1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  diff1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec1);
    glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,   15.0f);
    glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 8.0f);
    glDisable(GL_LIGHT1);

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
        glLightf (li, GL_CONSTANT_ATTENUATION,  0.3f);
        glLightf (li, GL_LINEAR_ATTENUATION,    0.2f);
        glLightf (li, GL_QUADRATIC_ATTENUATION, 0.08f);
        glEnable(li);
    }

    // atmospheric fog — dark blue-black, fades objects beyond ~40 units
    GLfloat fogColor[] = {0.03f, 0.03f, 0.10f, 1.0f};
    glEnable(GL_FOG);
    glFogfv(GL_FOG_COLOR,   fogColor);
    glFogi (GL_FOG_MODE,    GL_EXP2);
    glFogf (GL_FOG_DENSITY, 0.018f);
    glHint (GL_FOG_HINT,    GL_NICEST);

    // procedural checker texture (64×64)
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

    setTexture("assets/textures/grass.bmp", &texGrass, true);
}

void OnTimer(int) {
    static int lastMs = 0;
    int nowMs   = glutGet(GLUT_ELAPSED_TIME);
    float dt    = (lastMs == 0) ? 0.016f : (nowMs - lastMs) * 0.001f;
    if (dt > 0.05f) dt = 0.05f; // clamp: ignore stalls longer than 50ms
    lastMs = nowMs;

    const float speed = 5.0f;
    float dx = 0.0f, dz = 0.0f;

    if (keys['w'] || keys['W'] || keys[GLUT_KEY_UP    + 200]) { dx -= sinf(yaw) * speed * dt; dz -= cosf(yaw) * speed * dt; }
    if (keys['s'] || keys['S'] || keys[GLUT_KEY_DOWN  + 200]) { dx += sinf(yaw) * speed * dt; dz += cosf(yaw) * speed * dt; }
    if (keys['a'] || keys['A'] || keys[GLUT_KEY_LEFT  + 200]) { dx -= cosf(yaw) * speed * dt; dz += sinf(yaw) * speed * dt; }
    if (keys['d'] || keys['D'] || keys[GLUT_KEY_RIGHT + 200]) { dx += cosf(yaw) * speed * dt; dz -= sinf(yaw) * speed * dt; }

    camX += dx;
    camZ += dz;
    isMoving = (dx * dx + dz * dz > 0.0001f);

    if (isMoving) {
        bobTimer += dt;
        bobOffset = sinf(bobTimer * 10.0f) * 0.28f;
    } else {
        bobOffset *= 0.80f;
        if (fabsf(bobOffset) < 0.001f) bobOffset = 0.0f;
    }

    if (animOn) windmillAngle += 120.0f * dt;

    for (auto& p : projectiles) {
        if (!p.active) continue;
        p.vy -= 9.8f * dt;
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.z  += p.vz * dt;
        if (p.y < -10.0f) p.active = false;
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
    if (key == GLUT_KEY_PAGE_UP)   { camFloorY += 0.5f; if (camFloorY >  30.0f) camFloorY =  30.0f; lastAction = "camera up"; }
    if (key == GLUT_KEY_PAGE_DOWN) { camFloorY -= 0.5f; if (camFloorY < -5.0f)  camFloorY = -5.0f;  lastAction = "camera down"; }
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
    if (key == 27) exit(0);
    if (key == 'r' || key == 'R') {
        torchOn = !torchOn;
        lastAction = torchOn ? "torch ON" : "torch OFF";
    }
    if (key == ' ') SpawnProjectile();
    if (key == 'p' || key == 'P') {
        printf("[CAM] x=%.2f  y=%.2f  z=%.2f  yaw=%.2f deg  pitch=%.2f deg\n",
               camX, camFloorY, camZ,
               yaw   * (180.0f / (float)M_PI),
               pitch * (180.0f / (float)M_PI));
        fflush(stdout);
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
            break;
        case 2: animOn = !animOn; lastAction = animOn ? "animation ON" : "animation OFF"; break;
        case 3: texOn  = !texOn;  lastAction = texOn  ? "textures ON"  : "textures OFF";  break;
        case 4:
            if (glIsEnabled(GL_LIGHT0)) { glDisable(GL_LIGHT0); lastAction = "light OFF"; }
            else                        { glEnable (GL_LIGHT0); lastAction = "light ON";  }
            break;
        case 5: torchOn = !torchOn; lastAction = torchOn ? "torch ON" : "torch OFF"; break;
        case 6: {
            bool on = glIsEnabled(GL_LIGHT2);
            on ? glDisable(GL_LIGHT2) : glEnable(GL_LIGHT2);
            on ? glDisable(GL_LIGHT3) : glEnable(GL_LIGHT3);
            lastAction = on ? "lanterns OFF" : "lanterns ON";
            break;
        }
        case 7: exit(0);
    }
    glutPostRedisplay();
}

// ─── display ─────────────────────────────────────────────────────────────────

void OnDisplay() {
    glClearColor(0.03f, 0.03f, 0.10f, 1.0f); // matches fog color to avoid horizon seam
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
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

    if (torchOn) {
        glEnable(GL_LIGHT1);
        GLfloat lpos[] = {camX, camFloorY + bobOffset, camZ, 1.0f};
        GLfloat ldir[] = {
            -sinf(yaw) * cosf(pitch),
             sinf(pitch),
            -cosf(yaw) * cosf(pitch)
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

    // transparent pass — lantern windows
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (auto& l : kLanterns) {
        glPushMatrix();
            glTranslatef(l.x, l.y, l.z);
            DrawLanternWindow();
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    DrawHUD();

    if (ciScenario >= 0) {
        char path[64];
        snprintf(path, sizeof(path), "ci_shot_%d.bmp", ciScenario);
        SaveBMP(path, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        glutSwapBuffers();
        exit(0);
    }

    glutSwapBuffers();
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
    glutAddMenuEntry("Lanterns ON/OFF",  6);
    glutAddMenuEntry("Exit",             7);
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
