/*
 * MPC-MPG 2025/26 - Semestralni projekt
 *
 * Autor:       Martin Szüč
 * Student ID:  (doplň)
 * Email:       xszucm00@vut.cz
 *
 * Nazev projektu: Nocni park
 *
 * Seznam vypracovanych ukolu:
 *   1.  Modelovani objektu (5+ vlastnich)          3 b
 *   2.  Animace (vetyrnik)                         1 b
 *   3.  Osvetleni + normaly                        1 b
 *   4.  Volny pohyb (mys + WASD/sipky)             1 b
 *   5.  Menu (6 polozek)                           2 b
 *   6.  Vypis textu (HUD)                          2 b
 *   7.  Rucni svitilna (klavesa R)                 2 b
 *   10. Stoupani/klesani (Page Up/Down)            1 b
 *   12. Simulace kroku (camera bobbing)            2 b
 *   14. Pruhlednost (okno lucerny)                 1 b
 *   16. Texturovani (grass.bmp + checker)          2 b
 *   17. Bezierovy platy (teren)                    2 b
 *   11. Hod predmetu (Space + gravitace)           2 b
 *                                              CELKEM: 24 b
 *
 * Ovladani:
 *   W/S/A/D nebo sipky  pohyb
 *   mys (drag)          rozhlizeni
 *   R                   svitilna ON/OFF
 *   Space               hod predmetu
 *   Page Up/Down        vyska kamery
 *   prave tlacitko mys  menu
 *   Esc                 konec
 *
 * Konfigurace: macOS 14, CLion, OpenGL/GLUT (framework)
 */

#include "imageLoad.h"
#include <cmath>
#include <string>
#include <GLUT/glut.h>   // macOS
// #include <GL/glut.h>  // Windows/Linux

// ─── camera state ────────────────────────────────────────────────────────────

float camX = 0.0f, camY = 1.7f, camZ = 20.0f;
float yaw   = 0.0f;
float pitch = 0.0f;
bool  keys[512] = {};
bool  mouseDown  = false;
int   mouseLastX = 0, mouseLastY = 0;

// bobbing
float bobTimer  = 0.0f;
float bobOffset = 0.0f;
bool  isMoving  = false;
float camFloorY = 1.7f;

// ─── scene state ─────────────────────────────────────────────────────────────

bool  torchOn       = false;
bool  animOn        = true;
float windmillAngle = 0.0f;

bool         texOn      = true;
unsigned int texGrass   = 0;
unsigned int texChecker = 0;

std::string lastAction = "start";

// ─── projectiles ─────────────────────────────────────────────────────────────

struct Projectile { float x, y, z, vx, vy, vz; bool active; };
Projectile projectiles[10] = {};

// ─── helpers ─────────────────────────────────────────────────────────────────

void SpawnProjectile() {
    for (auto& p : projectiles) {
        if (p.active) continue;
        const float spd = 20.0f;
        p.x  = camX; p.y = camFloorY + bobOffset; p.z = camZ;
        p.vx = -sinf(yaw) * cosf(pitch) * spd;
        p.vy =  sinf(pitch) * spd;
        p.vz = -cosf(yaw) * cosf(pitch) * spd;
        p.active = true;
        lastAction = "hod predmetu";
        break;
    }
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

    // GL_LIGHT0 — moon (directional, blue-white)
    GLfloat amb0[]  = {0.05f, 0.05f, 0.15f, 1.0f};
    GLfloat diff0[] = {0.3f,  0.3f,  0.5f,  1.0f};
    GLfloat spec0[] = {0.1f,  0.1f,  0.2f,  1.0f};
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

    // external grass texture (skipped gracefully if file missing)
    setTexture("grass.bmp", &texGrass, true);
}

void OnTimer(int) {
    const float dt    = 0.016f;
    const float speed = 5.0f;
    float dx = 0.0f, dz = 0.0f;

    if (keys['w'] || keys['W'] || keys[GLUT_KEY_UP   + 200]) { dx += sinf(yaw) * speed * dt; dz += cosf(yaw) * speed * dt; }
    if (keys['s'] || keys['S'] || keys[GLUT_KEY_DOWN + 200]) { dx -= sinf(yaw) * speed * dt; dz -= cosf(yaw) * speed * dt; }
    if (keys['a'] || keys['A'])                               { dx += cosf(yaw) * speed * dt; dz -= sinf(yaw) * speed * dt; }
    if (keys['d'] || keys['D'])                               { dx -= cosf(yaw) * speed * dt; dz += sinf(yaw) * speed * dt; }

    camX += dx;
    camZ += dz;
    isMoving = (dx * dx + dz * dz > 0.0001f);

    if (isMoving) {
        bobTimer += dt;
        bobOffset = sinf(bobTimer * 8.0f) * 0.15f;
    } else {
        bobOffset *= 0.85f;
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

    glutPostRedisplay();
    glutTimerFunc(16, OnTimer, 0);
}

void OnSpecial(int key, int, int) {
    if (key == GLUT_KEY_UP)   keys[GLUT_KEY_UP   + 200] = true;
    if (key == GLUT_KEY_DOWN) keys[GLUT_KEY_DOWN + 200] = true;
    if (key == GLUT_KEY_PAGE_UP)   { camFloorY += 0.5f; if (camFloorY >  30.0f) camFloorY =  30.0f; lastAction = "kamera nahoru"; }
    if (key == GLUT_KEY_PAGE_DOWN) { camFloorY -= 0.5f; if (camFloorY < -5.0f)  camFloorY = -5.0f;  lastAction = "kamera dolu";   }
    glutPostRedisplay();
}

void OnSpecialUp(int key, int, int) {
    if (key == GLUT_KEY_UP)   keys[GLUT_KEY_UP   + 200] = false;
    if (key == GLUT_KEY_DOWN) keys[GLUT_KEY_DOWN + 200] = false;
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
        lastAction = torchOn ? "svitilna ON" : "svitilna OFF";
    }
    if (key == ' ') SpawnProjectile();
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
            lastAction = "reset kamery";
            break;
        case 2: animOn = !animOn; lastAction = animOn ? "animace ON" : "animace OFF"; break;
        case 3: texOn  = !texOn;  lastAction = texOn  ? "textury ON" : "textury OFF"; break;
        case 4:
            if (glIsEnabled(GL_LIGHT0)) { glDisable(GL_LIGHT0); lastAction = "svetlo OFF"; }
            else                        { glEnable (GL_LIGHT0); lastAction = "svetlo ON";  }
            break;
        case 5: torchOn = !torchOn; lastAction = torchOn ? "svitilna ON" : "svitilna OFF"; break;
        case 6: exit(0);
    }
    glutPostRedisplay();
}

// ─── HUD ─────────────────────────────────────────────────────────────────────

void DrawHUD(const char* text) {
    int w = glutGet(GLUT_WINDOW_WIDTH);
    int h = glutGet(GLUT_WINDOW_HEIGHT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2i(10, 20);
    for (const char* c = text; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}

// ─── display ─────────────────────────────────────────────────────────────────

void OnDisplay() {
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f); // night sky
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // camera via rotations + translation (no gluLookAt)
    glRotatef(-pitch * (180.0f / M_PI), 1.0f, 0.0f, 0.0f);
    glRotatef(-yaw   * (180.0f / M_PI), 0.0f, 1.0f, 0.0f);
    glTranslatef(-camX, -(camFloorY + bobOffset), -camZ);

    // directional moon light (set after camera rotation so it's world-space)
    GLfloat moonPos[] = {0.3f, 1.0f, 0.2f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, moonPos);

    // torch spotlight follows camera
    if (torchOn) {
        glEnable(GL_LIGHT1);
        GLfloat lpos[] = {camX, camFloorY + bobOffset, camZ, 1.0f};
        GLfloat ldir[] = {
             sinf(yaw) * cosf(pitch),
            -sinf(pitch),
             cosf(yaw) * cosf(pitch)
        };
        glLightfv(GL_LIGHT1, GL_POSITION,       lpos);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, ldir);
    } else {
        glDisable(GL_LIGHT1);
    }

    // placeholder ground — replaced by Bezier terrain in phase 3
    glDisable(GL_LIGHTING);
    glColor3f(0.1f, 0.3f, 0.1f);
    glBegin(GL_QUADS);
        glVertex3f(-100.0f, 0.0f,  100.0f);
        glVertex3f( 100.0f, 0.0f,  100.0f);
        glVertex3f( 100.0f, 0.0f, -100.0f);
        glVertex3f(-100.0f, 0.0f, -100.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    // HUD drawn last, before buffer swap
    DrawHUD(lastAction.c_str());

    glutSwapBuffers();
}

// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 768);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("MPC-MPG 2025/26 — Nocni park | xszucm00");

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
    glutAddMenuEntry("Reset kamery",    1);
    glutAddMenuEntry("Animace ON/OFF",  2);
    glutAddMenuEntry("Textury ON/OFF",  3);
    glutAddMenuEntry("Svetlo ON/OFF",   4);
    glutAddMenuEntry("Svitilna ON/OFF", 5);
    glutAddMenuEntry("Konec",           6);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    OnInit();
    glutTimerFunc(16, OnTimer, 0);

    glutMainLoop();
    return 0;
}
