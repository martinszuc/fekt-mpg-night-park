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

#include <cstring>
#include "imageLoad.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>
#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

// ─── CI screenshot mode ───────────────────────────────────────────────────────

static int ciScenario = -1; // set via --ci-scenario N; -1 = normal interactive mode

// ─── camera state ────────────────────────────────────────────────────────────

float camX = 0.0f, camZ = 20.0f;
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
        lastAction = "throw object";
        break;
    }
}

// ─── scene drawing ───────────────────────────────────────────────────────────

void SetMaterial(float r, float g, float b, float shin = 32.0f) {
    GLfloat amb[]  = {r * 0.2f, g * 0.2f, b * 0.2f, 1.0f};
    GLfloat diff[] = {r,        g,        b,        1.0f};
    GLfloat spec[] = {0.3f,     0.3f,     0.3f,     1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT, GL_SHININESS, shin);
}

// box with bottom at Y=0, centred on X/Z; UV coords on every vertex
void DrawBox(float w, float h, float d) {
    float hw = w * 0.5f, hd = d * 0.5f;
    glBegin(GL_QUADS);
        // front (+Z)
        glNormal3f(0, 0, 1);
        glTexCoord2f(0, 0); glVertex3f(-hw, 0, hd);
        glTexCoord2f(1, 0); glVertex3f( hw, 0, hd);
        glTexCoord2f(1, 1); glVertex3f( hw, h, hd);
        glTexCoord2f(0, 1); glVertex3f(-hw, h, hd);
        // back (-Z)
        glNormal3f(0, 0, -1);
        glTexCoord2f(0, 0); glVertex3f( hw, 0, -hd);
        glTexCoord2f(1, 0); glVertex3f(-hw, 0, -hd);
        glTexCoord2f(1, 1); glVertex3f(-hw, h, -hd);
        glTexCoord2f(0, 1); glVertex3f( hw, h, -hd);
        // right (+X)
        glNormal3f(1, 0, 0);
        glTexCoord2f(0, 0); glVertex3f(hw, 0,  hd);
        glTexCoord2f(1, 0); glVertex3f(hw, 0, -hd);
        glTexCoord2f(1, 1); glVertex3f(hw, h, -hd);
        glTexCoord2f(0, 1); glVertex3f(hw, h,  hd);
        // left (-X)
        glNormal3f(-1, 0, 0);
        glTexCoord2f(0, 0); glVertex3f(-hw, 0, -hd);
        glTexCoord2f(1, 0); glVertex3f(-hw, 0,  hd);
        glTexCoord2f(1, 1); glVertex3f(-hw, h,  hd);
        glTexCoord2f(0, 1); glVertex3f(-hw, h, -hd);
        // top (+Y)
        glNormal3f(0, 1, 0);
        glTexCoord2f(0, 0); glVertex3f(-hw, h, -hd);
        glTexCoord2f(1, 0); glVertex3f( hw, h, -hd);
        glTexCoord2f(1, 1); glVertex3f( hw, h,  hd);
        glTexCoord2f(0, 1); glVertex3f(-hw, h,  hd);
    glEnd();
}

// cross product of two edges → unit normal
static void triNormal(float ax, float ay, float az,
                      float bx, float by, float bz,
                      float cx, float cy, float cz) {
    float ex = bx-ax, ey = by-ay, ez = bz-az;
    float fx = cx-ax, fy = cy-ay, fz = cz-az;
    float nx = ey*fz - ez*fy;
    float ny = ez*fx - ex*fz;
    float nz = ex*fy - ey*fx;
    float len = sqrtf(nx*nx + ny*ny + nz*nz);
    if (len > 0.0001f) glNormal3f(nx/len, ny/len, nz/len);
}

void DrawTree() {
    // trunk
    SetMaterial(0.35f, 0.20f, 0.08f);
    DrawBox(0.4f, 3.0f, 0.4f);

    // canopy — 8-sided cone, base at y=3, apex at y=6
    SetMaterial(0.10f, 0.40f, 0.10f);
    const int sides = 8;
    const float baseR = 1.25f, baseY = 3.0f, apexY = 6.0f;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < sides; i++) {
        float a0 = (float)i       / sides * 2.0f * (float)M_PI;
        float a1 = (float)(i + 1) / sides * 2.0f * (float)M_PI;
        float x0 = cosf(a0) * baseR, z0 = sinf(a0) * baseR;
        float x1 = cosf(a1) * baseR, z1 = sinf(a1) * baseR;
        triNormal(x0, baseY, z0,  x1, baseY, z1,  0, apexY, 0);
        glVertex3f(x0, baseY, z0);
        glVertex3f(x1, baseY, z1);
        glVertex3f(0,  apexY, 0);
    }
    glEnd();
}

void DrawBench() {
    SetMaterial(0.55f, 0.30f, 0.10f);

    const float seatW = 2.0f, seatD = 0.65f, seatH = 0.7f;

    // seat plank — visible thickness of 0.12
    glPushMatrix();
        glTranslatef(0, seatH, 0);
        DrawBox(seatW, 0.12f, seatD);
    glPopMatrix();

    // 4 legs at seat corners
    float lx[2] = {-(seatW * 0.5f - 0.08f),  (seatW * 0.5f - 0.08f)};
    float lz[2] = {-(seatD * 0.5f - 0.08f),   (seatD * 0.5f - 0.08f)};
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) {
        glPushMatrix();
            glTranslatef(lx[i], 0, lz[j]);
            DrawBox(0.10f, seatH, 0.10f);
        glPopMatrix();
    }

    // backrest — rooted at back edge of seat, tilted 15° back
    glPushMatrix();
        glTranslatef(0, seatH, -(seatD * 0.5f));
        glRotatef(-15.0f, 1, 0, 0);
        DrawBox(seatW, 0.60f, 0.09f);
    glPopMatrix();
}

// lantern positions tracked for transparent window pass in phase 4
struct LanternPos { float x, y, z; };
static LanternPos lanternPositions[8];
static int        lanternCount = 0;

void DrawLantern() {
    // pole
    SetMaterial(0.15f, 0.15f, 0.15f, 16.0f);
    DrawBox(0.08f, 4.0f, 0.08f);
    // head body
    glPushMatrix();
        glTranslatef(0, 4.0f, 0);
        SetMaterial(0.7f, 0.65f, 0.2f, 64.0f);
        DrawBox(0.35f, 0.45f, 0.35f);
        // roof cap
        SetMaterial(0.15f, 0.15f, 0.15f, 16.0f);
        glTranslatef(0, 0.45f, 0);
        DrawBox(0.40f, 0.10f, 0.40f);
    glPopMatrix();
    // transparent window drawn separately in OnDisplay via DrawLanternWindow()
}

void DrawLanternWindow() {
    GLfloat diff[] = {1.0f, 0.9f, 0.5f, 0.4f};
    GLfloat amb[]  = {0.4f, 0.36f, 0.2f, 0.4f};
    GLfloat spec[] = {0.3f, 0.3f, 0.3f, 0.4f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 32.0f);
    glPushMatrix();
        glTranslatef(0, 4.0f, 0);
        DrawBox(0.42f, 0.45f, 0.42f);
    glPopMatrix();
}

void DrawShed() {
    // walls
    SetMaterial(0.60f, 0.45f, 0.25f);
    DrawBox(4.0f, 2.5f, 3.0f);

    // saddle roof — two rectangular slopes + two triangular gables
    SetMaterial(0.45f, 0.20f, 0.10f);
    float bx = 2.3f, by = 2.5f, ridge = 3.5f, rz = 1.5f;
    glBegin(GL_QUADS);
        // left slope
        triNormal(-bx, by, -rz,  -bx, by, rz,  0, ridge, rz);
        glVertex3f(-bx, by, -rz);
        glVertex3f(-bx, by,  rz);
        glVertex3f(  0, ridge,  rz);
        glVertex3f(  0, ridge, -rz);
        // right slope
        triNormal(bx, by, rz,  bx, by, -rz,  0, ridge, -rz);
        glVertex3f( bx, by,  rz);
        glVertex3f( bx, by, -rz);
        glVertex3f(  0, ridge, -rz);
        glVertex3f(  0, ridge,  rz);
    glEnd();
    glBegin(GL_TRIANGLES);
        // front gable
        triNormal(-bx, by, rz,  bx, by, rz,  0, ridge, rz);
        glVertex3f(-bx, by, rz);
        glVertex3f( bx, by, rz);
        glVertex3f(  0, ridge, rz);
        // back gable
        triNormal( bx, by, -rz,  -bx, by, -rz,  0, ridge, -rz);
        glVertex3f( bx, by, -rz);
        glVertex3f(-bx, by, -rz);
        glVertex3f(  0, ridge, -rz);
    glEnd();
}

void DrawBoulder() {
    SetMaterial(0.45f, 0.42f, 0.38f, 8.0f);
    // 10 triangles forming an irregular rock
    static const float v[][3] = {
        { 0.0f,  1.4f,  0.0f},   // 0 top
        {-1.0f,  0.0f, -0.5f},   // 1
        { 0.8f,  0.0f, -0.9f},   // 2
        { 1.1f,  0.0f,  0.3f},   // 3
        { 0.2f,  0.0f,  1.1f},   // 4
        {-0.9f,  0.0f,  0.6f},   // 5
        {-0.3f,  0.7f, -0.8f},   // 6 mid
        { 0.9f,  0.6f,  0.5f},   // 7 mid
        {-0.5f,  0.6f,  0.7f},   // 8 mid
    };
    static const int f[][3] = {
        {0,1,6}, {0,6,2}, {0,2,7}, {0,7,3},
        {0,3,4}, {0,4,8}, {0,8,5}, {0,5,1},
        {1,2,6}, {3,7,4},
    };
    glBegin(GL_TRIANGLES);
    for (auto& t : f) {
        triNormal(v[t[0]][0], v[t[0]][1], v[t[0]][2],
                  v[t[1]][0], v[t[1]][1], v[t[1]][2],
                  v[t[2]][0], v[t[2]][1], v[t[2]][2]);
        for (int k = 0; k < 3; k++)
            glVertex3f(v[t[k]][0], v[t[k]][1], v[t[k]][2]);
    }
    glEnd();
}

void DrawFence(int count, float spacing) {
    SetMaterial(0.60f, 0.45f, 0.25f);
    for (int i = 0; i < count; i++) {
        glPushMatrix();
            glTranslatef(i * spacing, 0, 0);
            DrawBox(0.10f, 1.2f, 0.10f);
        glPopMatrix();
    }
    // horizontal rails
    glPushMatrix();
        glTranslatef((count - 1) * spacing * 0.5f - 0.05f, 0.9f, 0);
        DrawBox((count - 1) * spacing, 0.06f, 0.06f);
    glPopMatrix();
    glPushMatrix();
        glTranslatef((count - 1) * spacing * 0.5f - 0.05f, 0.45f, 0);
        DrawBox((count - 1) * spacing, 0.06f, 0.06f);
    glPopMatrix();
}

void DrawWindmill() {
    // central pole
    SetMaterial(0.50f, 0.40f, 0.25f);
    DrawBox(0.30f, 5.0f, 0.30f);

    // 4 blades rotating around Z axis at the top
    SetMaterial(0.80f, 0.75f, 0.55f, 16.0f);
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(0, 5.0f, 0.18f);
            glRotatef(windmillAngle + i * 90.0f, 0, 0, 1);
            glTranslatef(0, 1.0f, 0);
            DrawBox(0.15f, 2.0f, 0.05f);
        glPopMatrix();
    }
}

// ─── projectile drawing ──────────────────────────────────────────────────────

static const float octVerts[6][3] = {
    { 1, 0, 0}, {-1, 0, 0},
    { 0, 1, 0}, { 0,-1, 0},
    { 0, 0, 1}, { 0, 0,-1},
};
static const int octFaces[8][3] = {
    {0,2,4},{2,1,4},{1,3,4},{3,0,4},
    {0,3,5},{3,1,5},{1,2,5},{2,0,5},
};

void DrawOctahedron(float r) {
    glBegin(GL_TRIANGLES);
    for (auto& f : octFaces) {
        float nx = (octVerts[f[0]][0] + octVerts[f[1]][0] + octVerts[f[2]][0]) / 3.0f;
        float ny = (octVerts[f[0]][1] + octVerts[f[1]][1] + octVerts[f[2]][1]) / 3.0f;
        float nz = (octVerts[f[0]][2] + octVerts[f[1]][2] + octVerts[f[2]][2]) / 3.0f;
        glNormal3f(nx, ny, nz);
        for (int v : f)
            glVertex3f(octVerts[v][0] * r, octVerts[v][1] * r, octVerts[v][2] * r);
    }
    glEnd();
}

void DrawProjectiles() {
    SetMaterial(0.9f, 0.2f, 0.2f, 64.0f);
    for (auto& p : projectiles) {
        if (!p.active) continue;
        glPushMatrix();
            glTranslatef(p.x, p.y, p.z);
            DrawOctahedron(0.2f);
        glPopMatrix();
    }
}

// ─── scene assembly ──────────────────────────────────────────────────────────

void DrawScene() {
    // trees
    static const float treePos[][2] = {
        {-5, -5}, {8, -3}, {-10, 5}, {3, 10}, {-2, 15}
    };
    for (auto& p : treePos) {
        glPushMatrix();
            glTranslatef(p[0], 0, p[1]);
            DrawTree();
        glPopMatrix();
    }

    // benches
    glPushMatrix();
        glTranslatef(0, 0, 0);
        DrawBench();
    glPopMatrix();
    glPushMatrix();
        glTranslatef(6, 0, -2);
        glRotatef(45, 0, 1, 0);
        DrawBench();
    glPopMatrix();

    // lanterns — register positions for transparent window pass
    lanternCount = 0;
    static const float lanternPos[][2] = {{4, 0}, {-4, 3}};
    for (auto& p : lanternPos) {
        glPushMatrix();
            glTranslatef(p[0], 0, p[1]);
            DrawLantern();
            if (lanternCount < 8) {
                lanternPositions[lanternCount++] = {p[0], 0, p[1]};
            }
        glPopMatrix();
    }

    // shed
    glPushMatrix();
        glTranslatef(12, 0, -8);
        DrawShed();
    glPopMatrix();

    // boulders
    glPushMatrix(); glTranslatef(-8, 0, 8);  DrawBoulder(); glPopMatrix();
    glPushMatrix(); glTranslatef( 2, 0, -10); DrawBoulder(); glPopMatrix();

    // fence row
    glPushMatrix();
        glTranslatef(-6, 0, -15);
        DrawFence(10, 1.2f);
    glPopMatrix();

    // windmill
    glPushMatrix();
        glTranslatef(0, 0, -20);
        DrawWindmill();
    glPopMatrix();
}

// ─── terrain ─────────────────────────────────────────────────────────────────

void DrawTerrain() {
    // 4×4 bicubic Bezier control points — hills across the park
    // Hills only at the four far corners; interior stays near Y=0 so
    // scene objects placed at ground level are not buried by the surface.
    // Bezier center evaluates to ≈0.12 (well below the shortest leg at 0.7).
    static float cp[4][4][3] = {
        {{-40,2,-40},{-13,0,-40},{ 13,0,-40},{40,2,-40}},
        {{-40,0,-13},{-13,0,-13},{ 13,0,-13},{40,0,-13}},
        {{-40,0, 13},{-13,0, 13},{ 13,0, 13},{40,0, 13}},
        {{-40,2, 40},{-13,0, 40},{ 13,0, 40},{40,2, 40}},
    };

    // texture coordinates scaled ×5 so grass tile repeats across terrain
    static float tcp[4][4][2] = {
        {{0,0},{1.67f,0},{3.33f,0},{5,0}},
        {{0,1.67f},{1.67f,1.67f},{3.33f,1.67f},{5,1.67f}},
        {{0,3.33f},{1.67f,3.33f},{3.33f,3.33f},{5,3.33f}},
        {{0,5    },{1.67f,5    },{3.33f,5    },{5,5    }},
    };

    SetMaterial(0.20f, 0.55f, 0.20f, 4.0f);

    if (texOn && texGrass) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glMap2f(GL_MAP2_TEXTURE_COORD_2, 0,1, 2,4, 0,1, 8,4, &tcp[0][0][0]);
        glEnable(GL_MAP2_TEXTURE_COORD_2);
    }

    glMap2f(GL_MAP2_VERTEX_3,
        0,1,  3, 4,   // u: stride=3 floats per point
        0,1, 12, 4,   // v: stride=12 floats per row of 4 points
        &cp[0][0][0]);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_AUTO_NORMAL);
    glMapGrid2f(30, 0,1, 30, 0,1);
    glEvalMesh2(GL_FILL, 0,30, 0,30);

    glDisable(GL_MAP2_VERTEX_3);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_MAP2_TEXTURE_COORD_2);
    if (texOn) glDisable(GL_TEXTURE_2D);
}

// path strip with procedural checker texture running through the park
void DrawPath() {
    if (texOn && texChecker) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texChecker);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    SetMaterial(0.50f, 0.45f, 0.35f, 8.0f);
    glNormal3f(0, 1, 0);
    // path along Z axis, 2 units wide, from z=25 to z=-22
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0);  glVertex3f(-1.0f, 0.02f,  25.0f);
        glTexCoord2f(1, 0);  glVertex3f( 1.0f, 0.02f,  25.0f);
        glTexCoord2f(1, 24); glVertex3f( 1.0f, 0.02f, -22.0f);
        glTexCoord2f(0, 24); glVertex3f(-1.0f, 0.02f, -22.0f);
    glEnd();
    if (texOn) glDisable(GL_TEXTURE_2D);
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

    // scene-wide ambient so shadowed faces aren't pitch black
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
    setTexture("assets/textures/grass.bmp", &texGrass, true);
}

void OnTimer(int) {
    const float dt    = 0.016f;
    const float speed = 5.0f;
    float dx = 0.0f, dz = 0.0f;

    if (keys['w'] || keys['W'] || keys[GLUT_KEY_UP   + 200]) { dx -= sinf(yaw) * speed * dt; dz -= cosf(yaw) * speed * dt; }
    if (keys['s'] || keys['S'] || keys[GLUT_KEY_DOWN + 200]) { dx += sinf(yaw) * speed * dt; dz += cosf(yaw) * speed * dt; }
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

    static int debugTick = 0;
    if (++debugTick >= 312) {
        debugTick = 0;
        printf("[CAM] x=%.2f  y=%.2f  z=%.2f  yaw=%.2f deg  pitch=%.2f deg\n",
               camX, camFloorY, camZ,
               yaw   * (180.0f / (float)M_PI),
               pitch * (180.0f / (float)M_PI));
        fflush(stdout);
    }

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
        case 6: exit(0);
    }
    glutPostRedisplay();
}

// ─── HUD ─────────────────────────────────────────────────────────────────────

void DrawHUD() {
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

    // bottom-left: last action
    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2i(10, 20);
    for (const char* c = lastAction.c_str(); *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // top-left: position, torch, texture state
    char info[128];
    snprintf(info, sizeof(info), "pos (%.1f, %.1f, %.1f)  torch: %s  tex: %s",
             camX, camFloorY + bobOffset, camZ,
             torchOn ? "ON" : "OFF",
             texOn   ? "ON" : "OFF");
    glColor3f(0.8f, 0.8f, 1.0f);
    glRasterPos2i(10, h - 24);
    for (const char* c = info; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}

// ─── CI screenshot helpers ────────────────────────────────────────────────────

static void SaveBMP(const char* path, int w, int h) {
    std::vector<uint8_t> px(w * h * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());

    int stride = (w * 3 + 3) & ~3;
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
            row[x*3+0] = px[(y*w+x)*3+2]; // R→B
            row[x*3+1] = px[(y*w+x)*3+1]; // G
            row[x*3+2] = px[(y*w+x)*3+0]; // B→R
        }
        fwrite(row.data(), 1, stride, f);
    }
    fclose(f);
}

// ─── display ─────────────────────────────────────────────────────────────────

void OnDisplay() {
    glClearColor(0.02f, 0.02f, 0.08f, 1.0f); // night sky
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // camera via rotations + translation (no gluLookAt)
    glRotatef(-pitch * (180.0f / (float)M_PI), 1.0f, 0.0f, 0.0f);
    glRotatef(-yaw   * (180.0f / (float)M_PI), 0.0f, 1.0f, 0.0f);
    glTranslatef(-camX, -(camFloorY + bobOffset), -camZ);

    // directional moon light (set after camera rotation so it's world-space)
    GLfloat moonPos[] = {0.3f, 1.0f, 0.2f, 0.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, moonPos);

    // torch spotlight follows camera
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

    // Bezier terrain with grass texture
    DrawTerrain();

    // checker-textured path strip
    DrawPath();

    // all scene objects
    DrawScene();

    // projectiles
    DrawProjectiles();

    // transparent pass — lantern windows (depth write off to avoid z-fighting)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int i = 0; i < lanternCount; i++) {
        glPushMatrix();
            glTranslatef(lanternPositions[i].x, lanternPositions[i].y, lanternPositions[i].z);
            DrawLanternWindow();
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // HUD drawn last, before buffer swap
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
    glutAddMenuEntry("Exit",             6);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    OnInit();

    // Parse --ci-scenario N and apply the hardcoded camera for that viewpoint
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--ci-scenario") == 0) {
            ciScenario = atoi(argv[i + 1]);
            struct { float x, floorY, z, yaw, pitch; } scenarios[] = {
                { 6.56f, 1.70f,  6.69f, 0.07994f, 0.02007f }, // scene 0: bench + lanterns area
                { 5.02f, 1.70f, 17.15f, 0.79013f, 0.00506f }, // scene 1: entry angle, windmill side
                { 1.75f, 1.70f,  5.37f, 0.14993f, 0.02496f }, // scene 2: deep park, shed visible
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
