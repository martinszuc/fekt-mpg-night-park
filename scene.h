#pragma once

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

// ─── camera state ────────────────────────────────────────────────────────────

float camX = 0.0f, camZ = 20.0f;
float yaw   = 0.0f;
float pitch = 0.0f;
bool  keys[512] = {};
bool  mouseDown  = false;
int   mouseLastX = 0, mouseLastY = 0;

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

// ─── material helper ─────────────────────────────────────────────────────────

void SetMaterial(float r, float g, float b, float shin = 32.0f) {
    GLfloat amb[]  = {r * 0.2f, g * 0.2f, b * 0.2f, 1.0f};
    GLfloat diff[] = {r,        g,        b,        1.0f};
    GLfloat spec[] = {0.3f,     0.3f,     0.3f,     1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT, GL_SHININESS, shin);
}

// ─── geometry primitives ─────────────────────────────────────────────────────

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

// cross product of two edges → unit face normal
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

// ─── scene objects ───────────────────────────────────────────────────────────

void DrawTree() {
    SetMaterial(0.35f, 0.20f, 0.08f);
    DrawBox(0.4f, 3.0f, 0.4f);

    SetMaterial(0.10f, 0.40f, 0.10f);
    const int   sides = 8;
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

    glPushMatrix();
        glTranslatef(0, seatH, 0);
        DrawBox(seatW, 0.12f, seatD);
    glPopMatrix();

    float lx[2] = {-(seatW * 0.5f - 0.08f),  (seatW * 0.5f - 0.08f)};
    float lz[2] = {-(seatD * 0.5f - 0.08f),   (seatD * 0.5f - 0.08f)};
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) {
        glPushMatrix();
            glTranslatef(lx[i], 0, lz[j]);
            DrawBox(0.10f, seatH, 0.10f);
        glPopMatrix();
    }

    glPushMatrix();
        glTranslatef(0, seatH, -(seatD * 0.5f));
        glRotatef(-15.0f, 1, 0, 0);
        DrawBox(seatW, 0.60f, 0.09f);
    glPopMatrix();
}

struct LanternPos { float x, y, z; };
constexpr LanternPos kLanterns[]  = {{4, 0, 0}, {-4, 0, 3}};
constexpr int        kLanternCount = 2;

void DrawLantern() {
    SetMaterial(0.15f, 0.15f, 0.15f, 16.0f);
    DrawBox(0.08f, 4.0f, 0.08f);
    glPushMatrix();
        glTranslatef(0, 4.0f, 0);
        SetMaterial(0.7f, 0.65f, 0.2f, 64.0f);
        DrawBox(0.35f, 0.45f, 0.35f);
        SetMaterial(0.15f, 0.15f, 0.15f, 16.0f);
        glTranslatef(0, 0.45f, 0);
        DrawBox(0.40f, 0.10f, 0.40f);
    glPopMatrix();
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
    SetMaterial(0.60f, 0.45f, 0.25f);
    DrawBox(4.0f, 2.5f, 3.0f);

    SetMaterial(0.45f, 0.20f, 0.10f);
    float bx = 2.3f, by = 2.5f, ridge = 3.5f, rz = 1.5f;
    glBegin(GL_QUADS);
        triNormal(-bx, by, -rz,  -bx, by, rz,  0, ridge, rz);
        glVertex3f(-bx, by, -rz);
        glVertex3f(-bx, by,  rz);
        glVertex3f(  0, ridge,  rz);
        glVertex3f(  0, ridge, -rz);
        triNormal(bx, by, rz,  bx, by, -rz,  0, ridge, -rz);
        glVertex3f( bx, by,  rz);
        glVertex3f( bx, by, -rz);
        glVertex3f(  0, ridge, -rz);
        glVertex3f(  0, ridge,  rz);
    glEnd();
    glBegin(GL_TRIANGLES);
        triNormal(-bx, by, rz,  bx, by, rz,  0, ridge, rz);
        glVertex3f(-bx, by, rz);
        glVertex3f( bx, by, rz);
        glVertex3f(  0, ridge, rz);
        triNormal( bx, by, -rz,  -bx, by, -rz,  0, ridge, -rz);
        glVertex3f( bx, by, -rz);
        glVertex3f(-bx, by, -rz);
        glVertex3f(  0, ridge, -rz);
    glEnd();
}

void DrawBoulder() {
    SetMaterial(0.45f, 0.42f, 0.38f, 8.0f);
    static const float v[][3] = {
        { 0.0f,  1.4f,  0.0f},
        {-1.0f,  0.0f, -0.5f},
        { 0.8f,  0.0f, -0.9f},
        { 1.1f,  0.0f,  0.3f},
        { 0.2f,  0.0f,  1.1f},
        {-0.9f,  0.0f,  0.6f},
        {-0.3f,  0.7f, -0.8f},
        { 0.9f,  0.6f,  0.5f},
        {-0.5f,  0.6f,  0.7f},
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
    SetMaterial(0.50f, 0.40f, 0.25f);
    DrawBox(0.30f, 5.0f, 0.30f);

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
    static const float treePos[][2] = {
        {-5, -5}, {8, -3}, {-10, 5}, {3, 10}, {-2, 15}
    };
    for (auto& p : treePos) {
        glPushMatrix();
            glTranslatef(p[0], 0, p[1]);
            DrawTree();
        glPopMatrix();
    }

    glPushMatrix();
        glTranslatef(0, 0, 0);
        DrawBench();
    glPopMatrix();
    glPushMatrix();
        glTranslatef(6, 0, -2);
        glRotatef(45, 0, 1, 0);
        DrawBench();
    glPopMatrix();

    for (auto& l : kLanterns) {
        glPushMatrix();
            glTranslatef(l.x, l.y, l.z);
            DrawLantern();
        glPopMatrix();
    }

    glPushMatrix();
        glTranslatef(12, 0, -8);
        DrawShed();
    glPopMatrix();

    glPushMatrix(); glTranslatef(-8, 0,   8); DrawBoulder(); glPopMatrix();
    glPushMatrix(); glTranslatef( 2, 0, -10); DrawBoulder(); glPopMatrix();

    glPushMatrix();
        glTranslatef(-6, 0, -15);
        DrawFence(10, 1.2f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(0, 0, -20);
        DrawWindmill();
    glPopMatrix();
}

// ─── terrain ─────────────────────────────────────────────────────────────────

void DrawTerrain() {
    // hills only at the four far corners; interior stays near Y=0
    static float cp[4][4][3] = {
        {{-40,2,-40},{-13,0,-40},{ 13,0,-40},{40,2,-40}},
        {{-40,0,-13},{-13,0,-13},{ 13,0,-13},{40,0,-13}},
        {{-40,0, 13},{-13,0, 13},{ 13,0, 13},{40,0, 13}},
        {{-40,2, 40},{-13,0, 40},{ 13,0, 40},{40,2, 40}},
    };
    // texture coordinates scaled ×5 so grass tile repeats
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
        0,1,  3, 4,
        0,1, 12, 4,
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

void DrawPath() {
    if (texOn && texChecker) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texChecker);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    SetMaterial(0.50f, 0.45f, 0.35f, 8.0f);
    glNormal3f(0, 1, 0);
    glBegin(GL_QUADS);
        glTexCoord2f(0, 0);  glVertex3f(-1.0f, 0.02f,  25.0f);
        glTexCoord2f(1, 0);  glVertex3f( 1.0f, 0.02f,  25.0f);
        glTexCoord2f(1, 24); glVertex3f( 1.0f, 0.02f, -22.0f);
        glTexCoord2f(0, 24); glVertex3f(-1.0f, 0.02f, -22.0f);
    glEnd();
    if (texOn) glDisable(GL_TEXTURE_2D);
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

    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2i(10, 20);
    for (const char* c = lastAction.c_str(); *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

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
