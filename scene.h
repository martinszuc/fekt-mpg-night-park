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

struct Projectile { float x, y, z, vx, vy, vz, rot; bool active; };
Projectile projectiles[10] = {};

void SpawnProjectile() {
    for (auto& p : projectiles) {
        if (p.active) continue;
        const float spd = 20.0f;
        p.x   = camX; p.y = camFloorY + bobOffset; p.z = camZ;
        p.vx  = -sinf(yaw) * cosf(pitch) * spd;
        p.vy  =  sinf(pitch) * spd;
        p.vz  = -cosf(yaw) * cosf(pitch) * spd;
        p.rot = 0.0f;
        p.active = true;
        lastAction = "throw object";
        break;
    }
}

// ─── sky ─────────────────────────────────────────────────────────────────────

static float starVerts[200][3];
static float starPhases[200];   // twinkle phase offset per star
static float starSpeeds[200];   // twinkle frequency per star
static float starSizes[200];    // 0 = 1 px, 1 = 2 px
static int   starCount = 0;

// moon billboard quad: same direction as GL_LIGHT0 (0.3, 1, 0.2)
static void DrawMoon() {
    glDisable(GL_LIGHTING);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    const float d    = 70.0f;
    const float norm = sqrtf(0.3f*0.3f + 1.0f*1.0f + 0.2f*0.2f);
    const float mx   = 0.3f * d / norm;
    const float my   = 1.0f * d / norm;
    const float mz   = 0.2f * d / norm;
    const float s    = 3.5f;    // disc half-size
    const float hs   = s * 3.0f; // halo half-size

    // halo: additive, very faint
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.6f, 0.6f, 0.4f, 0.08f);
    glPushMatrix();
        glTranslatef(mx, my, mz);
        glBegin(GL_QUADS);
            glVertex3f(-hs,  hs, 0);
            glVertex3f( hs,  hs, 0);
            glVertex3f( hs, -hs, 0);
            glVertex3f(-hs, -hs, 0);
        glEnd();
    glPopMatrix();

    // disc: opaque
    glBlendFunc(GL_ONE, GL_ZERO);
    glColor3f(0.95f, 0.95f, 0.80f);
    glPushMatrix();
        glTranslatef(mx, my, mz);
        glBegin(GL_QUADS);
            glVertex3f(-s,  s, 0);
            glVertex3f( s,  s, 0);
            glVertex3f( s, -s, 0);
            glVertex3f(-s, -s, 0);
        glEnd();
    glPopMatrix();

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_LIGHTING);
}

void DrawSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_FOG);

    // horizon gradient cylinder — centred on camera so it always fills the sky
    {
        const int   segs = 32;
        const float cR   = 75.0f;
        const float cTop = 40.0f, cBot = -5.0f;
        glPushMatrix();
        glTranslatef(camX, 0.0f, camZ);
        glBegin(GL_QUADS);
        for (int i = 0; i < segs; i++) {
            float a0 = (float)i       / segs * 2.0f * (float)M_PI;
            float a1 = (float)(i + 1) / segs * 2.0f * (float)M_PI;
            float x0 = cosf(a0) * cR, z0 = sinf(a0) * cR;
            float x1 = cosf(a1) * cR, z1 = sinf(a1) * cR;
            glColor3f(0.02f, 0.02f, 0.08f); // dark blue-black at top
            glVertex3f(x0, cTop, z0);
            glVertex3f(x1, cTop, z1);
            glColor3f(0.05f, 0.05f, 0.20f); // deep indigo at horizon
            glVertex3f(x1, cBot, z1);
            glVertex3f(x0, cBot, z0);
        }
        glEnd();
        glPopMatrix();
    }

    // stars with twinkle — two size passes to avoid mid-primitive glPointSize calls
    float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < starCount; i++) {
        if (starSizes[i] >= 0.5f) continue;
        float bright = 0.5f + 0.5f * sinf(t * starSpeeds[i] + starPhases[i]);
        glColor3f(0.88f * bright, 0.90f * bright, bright);
        glVertex3fv(starVerts[i]);
    }
    glEnd();

    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < starCount; i++) {
        if (starSizes[i] < 0.5f) continue;
        float bright = 0.5f + 0.5f * sinf(t * starSpeeds[i] + starPhases[i]);
        glColor3f(0.88f * bright, 0.90f * bright, bright);
        glVertex3fv(starVerts[i]);
    }
    glEnd();

    DrawMoon();

    glEnable(GL_FOG);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
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

// axis-aligned box: bottom at Y=0, centred on X/Z
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

// compute and emit unit face normal from three CCW vertices
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
    // trunk
    SetMaterial(0.35f, 0.20f, 0.08f);
    DrawBox(0.4f, 2.5f, 0.4f);

    // undergrowth disc at base — hides the trunk-ground seam
    SetMaterial(0.06f, 0.18f, 0.06f);
    {
        const int disc = 8;
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glVertex3f(0, 0.01f, 0);
        for (int i = 0; i <= disc; i++) {
            float a = (float)i / disc * 2.0f * (float)M_PI;
            glVertex3f(cosf(a) * 1.55f, 0.01f, sinf(a) * 1.55f);
        }
        glEnd();
    }

    // three-layer spruce canopy — darker at base, lighter toward tip
    struct ConeLayer { float baseR, baseY, apexY; float r, g, b; };
    static const ConeLayer layers[] = {
        {1.25f, 2.5f, 4.5f, 0.10f, 0.38f, 0.10f},
        {0.90f, 4.0f, 6.0f, 0.12f, 0.44f, 0.12f},
        {0.55f, 5.5f, 7.0f, 0.14f, 0.50f, 0.14f},
    };
    const int sides = 8;
    for (auto& layer : layers) {
        SetMaterial(layer.r, layer.g, layer.b);
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < sides; i++) {
            float a0 = (float)i       / sides * 2.0f * (float)M_PI;
            float a1 = (float)(i + 1) / sides * 2.0f * (float)M_PI;
            float x0 = cosf(a0) * layer.baseR, z0 = sinf(a0) * layer.baseR;
            float x1 = cosf(a1) * layer.baseR, z1 = sinf(a1) * layer.baseR;
            triNormal(x0, layer.baseY, z0,  x1, layer.baseY, z1,  0, layer.apexY, 0);
            glVertex3f(x0, layer.baseY, z0);
            glVertex3f(x1, layer.baseY, z1);
            glVertex3f(0,  layer.apexY, 0);
        }
        glEnd();
    }
}

void DrawBench() {
    SetMaterial(0.55f, 0.30f, 0.10f);

    const float seatW = 2.0f, seatD = 0.65f, seatH = 0.7f;

    // seat plank
    glPushMatrix();
        glTranslatef(0, seatH, 0);
        DrawBox(seatW, 0.12f, seatD);
    glPopMatrix();

    // four legs
    float lx[2] = {-(seatW * 0.5f - 0.08f),  (seatW * 0.5f - 0.08f)};
    float lz[2] = {-(seatD * 0.5f - 0.08f),   (seatD * 0.5f - 0.08f)};
    for (int i = 0; i < 2; i++) for (int j = 0; j < 2; j++) {
        glPushMatrix();
            glTranslatef(lx[i], 0, lz[j]);
            DrawBox(0.10f, seatH, 0.10f);
        glPopMatrix();
    }

    // backrest
    glPushMatrix();
        glTranslatef(0, seatH, -(seatD * 0.5f));
        glRotatef(-15.0f, 1, 0, 0);
        DrawBox(seatW, 0.60f, 0.09f);
    glPopMatrix();

    // armrests at outer ends
    const float armW = seatW * 0.08f;
    float ax[2] = {-(seatW * 0.5f - armW * 0.5f), (seatW * 0.5f - armW * 0.5f)};
    for (int i = 0; i < 2; i++) {
        glPushMatrix();
            glTranslatef(ax[i], seatH, 0);
            DrawBox(armW, 0.35f, seatD);
        glPopMatrix();
    }
}

struct LanternPos { float x, y, z; };
constexpr LanternPos kLanterns[]   = {{4, 0, 0}, {-4, 0, 3}};
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

// camera-facing glow halo at the lantern head
// rx/ry/rz = camera right in world space, ux/uy/uz = camera up in world space
// call with additive blend (GL_SRC_ALPHA, GL_ONE) and depth-mask off already active
void DrawLanternGlow(float rx, float ry, float rz,
                     float ux, float uy, float uz) {
    const float s = 0.8f;
    glDisable(GL_LIGHTING);
    glColor4f(1.0f, 0.85f, 0.3f, 0.25f);
    glPushMatrix();
        glTranslatef(0, 4.2f, 0);
        glBegin(GL_QUADS);
            glVertex3f((-rx + ux) * s, (-ry + uy) * s, (-rz + uz) * s); // TL
            glVertex3f(( rx + ux) * s, ( ry + uy) * s, ( rz + uz) * s); // TR
            glVertex3f(( rx - ux) * s, ( ry - uy) * s, ( rz - uz) * s); // BR
            glVertex3f((-rx - ux) * s, (-ry - uy) * s, (-rz - uz) * s); // BL
        glEnd();
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void DrawShed() {
    // walls
    SetMaterial(0.60f, 0.45f, 0.25f);
    DrawBox(4.0f, 2.5f, 3.0f);

    // roof
    SetMaterial(0.45f, 0.20f, 0.10f);
    float bx = 2.3f, by = 2.5f, ridge = 3.5f, rz = 1.5f;
    glBegin(GL_QUADS);
        triNormal(-bx, by, -rz,  -bx, by, rz,  0, ridge, rz);
        glVertex3f(-bx, by, -rz); glVertex3f(-bx, by,  rz);
        glVertex3f(  0, ridge,  rz); glVertex3f(  0, ridge, -rz);
        triNormal(bx, by, rz,  bx, by, -rz,  0, ridge, -rz);
        glVertex3f( bx, by,  rz); glVertex3f( bx, by, -rz);
        glVertex3f(  0, ridge, -rz); glVertex3f(  0, ridge,  rz);
    glEnd();
    glBegin(GL_TRIANGLES);
        triNormal(-bx, by, rz,  bx, by, rz,  0, ridge, rz);
        glVertex3f(-bx, by, rz); glVertex3f( bx, by, rz); glVertex3f(0, ridge, rz);
        triNormal( bx, by, -rz,  -bx, by, -rz,  0, ridge, -rz);
        glVertex3f( bx, by, -rz); glVertex3f(-bx, by, -rz); glVertex3f(0, ridge, -rz);
    glEnd();

    // door on front face (z = 1.5), slightly offset from centre
    SetMaterial(0.35f, 0.18f, 0.08f);
    glPushMatrix();
        glTranslatef(0.4f, 0.0f, 1.53f);
        DrawBox(0.8f, 1.8f, 0.06f);
        // door knob
        SetMaterial(0.65f, 0.55f, 0.20f, 64.0f);
        glPushMatrix();
            glTranslatef(-0.25f, 0.9f, 0.06f);
            DrawBox(0.08f, 0.08f, 0.08f);
        glPopMatrix();
    glPopMatrix();
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
    const float pb = 0.06f, ph = 0.12f; // pyramid cap: half-base, height

    for (int i = 0; i < count; i++) {
        glPushMatrix();
            glTranslatef(i * spacing, 0, 0);
            DrawBox(0.10f, 1.2f, 0.10f);
            // pyramid cap on top of post
            glTranslatef(0, 1.2f, 0);
            glBegin(GL_TRIANGLES);
                triNormal(-pb, 0,  pb,   pb, 0,  pb,  0, ph, 0);
                glVertex3f(-pb, 0,  pb); glVertex3f( pb, 0,  pb); glVertex3f(0, ph, 0);
                triNormal( pb, 0,  pb,   pb, 0, -pb,  0, ph, 0);
                glVertex3f( pb, 0,  pb); glVertex3f( pb, 0, -pb); glVertex3f(0, ph, 0);
                triNormal( pb, 0, -pb,  -pb, 0, -pb,  0, ph, 0);
                glVertex3f( pb, 0, -pb); glVertex3f(-pb, 0, -pb); glVertex3f(0, ph, 0);
                triNormal(-pb, 0, -pb,  -pb, 0,  pb,  0, ph, 0);
                glVertex3f(-pb, 0, -pb); glVertex3f(-pb, 0,  pb); glVertex3f(0, ph, 0);
            glEnd();
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

// ─── fireflies ───────────────────────────────────────────────────────────────

struct Firefly { float x, y, z, phase, speed, wanderAngle; };
static Firefly      fireflies[8];
static const int    kFireflyCount = 8;

void DrawFireflies() {
    glDisable(GL_LIGHTING);
    glPointSize(3.0f);
    float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    glBegin(GL_POINTS);
    for (int i = 0; i < kFireflyCount; i++) {
        float b = 0.5f + 0.5f * sinf(t * fireflies[i].speed + fireflies[i].phase);
        glColor3f(0.3f * b, 0.9f * b, 0.2f * b); // green-yellow blink
        glVertex3f(fireflies[i].x, fireflies[i].y, fireflies[i].z);
    }
    glEnd();
    glPointSize(1.0f);
    glEnable(GL_LIGHTING);
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
            glRotatef(p.rot, 1, 1, 0); // spin in flight
            DrawOctahedron(0.2f);
        glPopMatrix();
    }
}

// ─── scene assembly ──────────────────────────────────────────────────────────

void DrawScene() {
    struct TreeSpec { float x, z, scale, yaw; };
    static const TreeSpec trees[] = {
        {-5,  -5, 1.00f,   0.0f},
        { 8,  -3, 1.30f,  40.0f},
        {-10,  5, 0.85f, 120.0f},
        { 3,  10, 1.15f,  70.0f},
        {-2,  15, 1.00f, 200.0f},
    };
    for (auto& t : trees) {
        glPushMatrix();
            glTranslatef(t.x, 0, t.z);
            glRotatef(t.yaw, 0, 1, 0);
            glScalef(t.scale, t.scale, t.scale);
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

    // lanterns
    for (auto& l : kLanterns) {
        glPushMatrix();
            glTranslatef(l.x, l.y, l.z);
            DrawLantern();
        glPopMatrix();
    }

    // shed
    glPushMatrix();
        glTranslatef(12, 0, -8);
        DrawShed();
    glPopMatrix();

    // boulder cluster near (-8, 0, 8)
    glPushMatrix(); glTranslatef(-8.0f, 0,  8.0f); DrawBoulder(); glPopMatrix();
    glPushMatrix(); glTranslatef(-9.5f, 0,  7.0f); glScalef(0.4f, 0.4f, 0.4f); DrawBoulder(); glPopMatrix();
    glPushMatrix(); glTranslatef(-7.0f, 0,  9.0f); glScalef(0.3f, 0.3f, 0.3f); DrawBoulder(); glPopMatrix();
    glPushMatrix(); glTranslatef(-8.5f, 0,  9.5f); glScalef(0.5f, 0.5f, 0.5f); DrawBoulder(); glPopMatrix();
    // second boulder
    glPushMatrix(); glTranslatef( 2.0f, 0, -10.0f); DrawBoulder(); glPopMatrix();

    // fence
    glPushMatrix();
        glTranslatef(-6, 0, -15);
        DrawFence(10, 1.2f);
    glPopMatrix();

    // windmill
    glPushMatrix();
        glTranslatef(0, 0, -20);
        DrawWindmill();
    glPopMatrix();

    // fireflies
    DrawFireflies();
}

// ─── terrain ─────────────────────────────────────────────────────────────────

void DrawTerrain() {
    static float cp[4][4][3] = {
        {{-40,2,-40},{-13,0,-40},{ 13,0,-40},{40,2,-40}},
        {{-40,0,-13},{-13,0,-13},{ 13,0,-13},{40,0,-13}},
        {{-40,0, 13},{-13,0, 13},{ 13,0, 13},{40,0, 13}},
        {{-40,2, 40},{-13,0, 40},{ 13,0, 40},{40,2, 40}},
    };
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

    glMap2f(GL_MAP2_VERTEX_3, 0,1, 3, 4, 0,1, 12, 4, &cp[0][0][0]);
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
    // wet-stone: high shininess + boosted specular so moonlight glints off path
    SetMaterial(0.50f, 0.45f, 0.35f, 128.0f);
    GLfloat pathSpec[] = {0.6f, 0.6f, 0.6f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, pathSpec);

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

    // FPS counter — update once per second
    static int fps = 0, fpsFrames = 0, fpsLastMs = 0;
    fpsFrames++;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    if (nowMs - fpsLastMs >= 1000) {
        fps       = fpsFrames;
        fpsFrames = 0;
        fpsLastMs = nowMs;
    }

    // bottom-left: last action
    glColor3f(1.0f, 1.0f, 0.0f);
    glRasterPos2i(10, 22);
    for (const char* c = lastAction.c_str(); *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

    // bottom-right: key hints
    const char* hints = "WASD move  R torch  Space throw  RMB menu";
    glColor3f(0.45f, 0.45f, 0.45f);
    glRasterPos2i(w - 298, 8);
    for (const char* c = hints; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // top-left: position / state info
    char info[128];
    snprintf(info, sizeof(info), "pos (%.1f, %.1f, %.1f)  torch: %s  tex: %s",
             camX, camFloorY + bobOffset, camZ,
             torchOn ? "ON" : "OFF",
             texOn   ? "ON" : "OFF");
    glColor3f(0.8f, 0.8f, 1.0f);
    glRasterPos2i(10, h - 24);
    for (const char* c = info; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // top-right: FPS
    char fpsBuf[16];
    snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d", fps);
    glColor3f(0.5f, 0.5f, 0.5f);
    glRasterPos2i(w - 72, h - 24);
    for (const char* c = fpsBuf; *c; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

    // centre: crosshair
    {
        int cx = w / 2, cy = h / 2;
        const int arm = 8;
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINES);
            glVertex2i(cx - arm, cy);
            glVertex2i(cx + arm, cy);
            glVertex2i(cx, cy - arm);
            glVertex2i(cx, cy + arm);
        glEnd();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_LIGHTING);
}
