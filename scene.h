#pragma once

#define _USE_MATH_DEFINES
#include <cstring>
#include "imageLoad.h"
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

// ─── named constants ─────────────────────────────────────────────────────────

constexpr float kCamSpeed        = 5.0f;
constexpr float kBobAmplitude    = 0.28f;
constexpr float kBobFrequency    = 10.0f;
constexpr float kProjectileSpeed = 20.0f;
constexpr float kFogDensity      = 0.018f;
constexpr int   kStarCount       = 200;
constexpr float kStarRadius      = 80.0f;

// ─── projectiles ─────────────────────────────────────────────────────────────

struct Projectile { float x, y, z, vx, vy, vz, rot; bool active; };
Projectile projectiles[10] = {};

void SpawnProjectile() {
    for (auto& p : projectiles) {
        if (p.active) continue;
        constexpr float spd = kProjectileSpeed;
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

static float starVerts[kStarCount][3];
static float starPhases[kStarCount];  // twinkle phase offset per star
static float starSpeeds[kStarCount];  // twinkle frequency per star
static float starSizes[kStarCount];   // 0 = 1 px, 1 = 2 px

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

    // horizon gradient cylinder — centred at origin (drawn in rotation-only pass)
    {
        const int   segs = 32;
        const float cR   = 75.0f;
        const float cTop = 40.0f, cBot = -5.0f;
        glBegin(GL_QUADS);
        for (int i = 0; i < segs; i++) {
            float a0 = (float)i       / segs * 2.0f * (float)M_PI;
            float a1 = (float)(i + 1) / segs * 2.0f * (float)M_PI;
            float x0 = cosf(a0) * cR, z0 = sinf(a0) * cR;
            float x1 = cosf(a1) * cR, z1 = sinf(a1) * cR;
            glColor3f(0.02f, 0.02f, 0.08f);
            glVertex3f(x0, cTop, z0);
            glVertex3f(x1, cTop, z1);
            glColor3f(0.05f, 0.05f, 0.20f);
            glVertex3f(x1, cBot, z1);
            glVertex3f(x0, cBot, z0);
        }
        glEnd();
    }

    // stars with twinkle — two size passes to avoid mid-primitive glPointSize calls
    float t = glutGet(GLUT_ELAPSED_TIME) * 0.001f;

    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < kStarCount; i++) {
        if (starSizes[i] >= 0.5f) continue;
        float bright = 0.5f + 0.5f * sinf(t * starSpeeds[i] + starPhases[i]);
        glColor3f(0.88f * bright, 0.90f * bright, bright);
        glVertex3fv(starVerts[i]);
    }
    glEnd();

    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < kStarCount; i++) {
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
    GLfloat amb[]  = {r * 0.3f, g * 0.3f, b * 0.3f, 1.0f};
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
    // 8-sided polygon trunk (rounder than a box)
    SetMaterial(0.35f, 0.20f, 0.08f);
    {
        const int  sides = 8;
        const float r = 0.22f, h = 2.5f;
        glBegin(GL_QUADS);
        for (int i = 0; i < sides; i++) {
            float a0 = (float)i       / sides * 2.0f * (float)M_PI;
            float a1 = (float)(i + 1) / sides * 2.0f * (float)M_PI;
            float x0 = cosf(a0) * r, z0 = sinf(a0) * r;
            float x1 = cosf(a1) * r, z1 = sinf(a1) * r;
            float nx = (x0 + x1) * 0.5f, nz = (z0 + z1) * 0.5f;
            float nl = sqrtf(nx * nx + nz * nz);
            glNormal3f(nx / nl, 0.0f, nz / nl);
            glVertex3f(x0, 0, z0); glVertex3f(x1, 0, z1);
            glVertex3f(x1, h, z1); glVertex3f(x0, h, z0);
        }
        glEnd();
    }

    // undergrowth disc at base — hides the trunk-ground seam
    SetMaterial(0.06f, 0.18f, 0.06f);
    {
        const int disc = 10;
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 1, 0);
        glVertex3f(0, 0.01f, 0);
        for (int i = 0; i <= disc; i++) {
            float a = (float)i / disc * 2.0f * (float)M_PI;
            glVertex3f(cosf(a) * 1.55f, 0.01f, sinf(a) * 1.55f);
        }
        glEnd();
    }

    // four-layer spruce canopy — darker at base, brighter toward tip
    struct ConeLayer { float baseR, baseY, apexY; float r, g, b; };
    static const ConeLayer layers[] = {
        {1.40f, 1.8f, 4.2f, 0.09f, 0.32f, 0.09f},
        {1.05f, 3.2f, 5.6f, 0.11f, 0.40f, 0.11f},
        {0.70f, 4.7f, 6.5f, 0.13f, 0.46f, 0.13f},
        {0.38f, 5.8f, 7.2f, 0.16f, 0.52f, 0.16f},
    };
    const int sides = 12;
    for (auto& layer : layers) {
        SetMaterial(layer.r, layer.g, layer.b);
        glBegin(GL_TRIANGLES);
        for (int i = 0; i < sides; i++) {
            float a0 = (float)i       / sides * 2.0f * (float)M_PI;
            float a1 = (float)(i + 1) / sides * 2.0f * (float)M_PI;
            float x0 = cosf(a0) * layer.baseR, z0 = sinf(a0) * layer.baseR;
            float x1 = cosf(a1) * layer.baseR, z1 = sinf(a1) * layer.baseR;
            triNormal(x0, layer.baseY, z0, x1, layer.baseY, z1, 0, layer.apexY, 0);
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

    // windows on side walls
    SetMaterial(0.10f, 0.14f, 0.22f, 80.0f); // dark glass
    glPushMatrix();
        glTranslatef(-2.02f, 1.0f, 0.0f); // left wall
        DrawBox(0.08f, 0.60f, 0.70f);
    glPopMatrix();
    glPushMatrix();
        glTranslatef(2.02f, 1.0f, 0.0f);  // right wall
        DrawBox(0.08f, 0.60f, 0.70f);
    glPopMatrix();

    // chimney on left side of roof (pokes through slope)
    SetMaterial(0.55f, 0.35f, 0.28f, 12.0f); // brick red
    glPushMatrix();
        glTranslatef(-0.8f, 2.5f, 0.2f);
        DrawBox(0.36f, 1.40f, 0.36f);
        // chimney cap — slightly wider lip
        glTranslatef(0, 1.40f, 0);
        DrawBox(0.46f, 0.09f, 0.46f);
    glPopMatrix();
}

void DrawBoulder() {
    // apex + 6-vert upper ring + 6-vert lower ring + base centre = 14 verts, 24 faces
    static const float v[][3] = {
        // 0: apex
        { 0.00f, 1.60f,  0.00f},
        // 1-6: upper ring ~y=0.85
        { 0.75f, 0.90f,  0.00f},
        { 0.38f, 0.85f,  0.65f},
        {-0.55f, 0.80f,  0.50f},
        {-0.80f, 0.90f, -0.20f},
        {-0.25f, 0.85f, -0.70f},
        { 0.60f, 0.85f, -0.52f},
        // 7-12: lower ring ~y=0.15 (wider, more irregular)
        { 1.00f, 0.18f,  0.00f},
        { 0.42f, 0.12f,  0.88f},
        {-0.65f, 0.15f,  0.72f},
        {-1.05f, 0.16f, -0.12f},
        {-0.50f, 0.12f, -0.85f},
        { 0.62f, 0.15f, -0.82f},
        // 13: flat base centre
        { 0.00f, 0.00f,  0.00f},
    };
    static const int f[][3] = {
        // top cap
        {0,1,2},{0,2,3},{0,3,4},{0,4,5},{0,5,6},{0,6,1},
        // upper ring to lower ring (6 quads as 12 tris)
        {1,7,2},{7,8,2},
        {2,8,3},{8,9,3},
        {3,9,4},{9,10,4},
        {4,10,5},{10,11,5},
        {5,11,6},{11,12,6},
        {6,12,1},{12,7,1},
        // bottom cap
        {13,8,7},{13,9,8},{13,10,9},{13,11,10},{13,12,11},{13,7,12},
    };
    // base material: middle band (6-17) + bottom cap (18-23)
    SetMaterial(0.46f, 0.42f, 0.36f, 8.0f);
    glBegin(GL_TRIANGLES);
    for (int i = 6; i < 24; i++) {
        triNormal(v[f[i][0]][0], v[f[i][0]][1], v[f[i][0]][2],
                  v[f[i][1]][0], v[f[i][1]][1], v[f[i][1]][2],
                  v[f[i][2]][0], v[f[i][2]][1], v[f[i][2]][2]);
        for (int k = 0; k < 3; k++)
            glVertex3f(v[f[i][k]][0], v[f[i][k]][1], v[f[i][k]][2]);
    }
    glEnd();
    // lighter lichen tone on top cap (faces 0-5) — drawn once, no coplanar overdraw
    SetMaterial(0.56f, 0.54f, 0.46f, 16.0f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 6; i++) {
        triNormal(v[f[i][0]][0], v[f[i][0]][1], v[f[i][0]][2],
                  v[f[i][1]][0], v[f[i][1]][1], v[f[i][1]][2],
                  v[f[i][2]][0], v[f[i][2]][1], v[f[i][2]][2]);
        for (int k = 0; k < 3; k++)
            glVertex3f(v[f[i][k]][0], v[f[i][k]][1], v[f[i][k]][2]);
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

// tapered blade: wider at root (y=0), narrower at tip (y=len)
// spine spar and cross-braces added on the front face (+Z side)
static void DrawBlade(float len, float rootW, float depth) {
    float hr = rootW * 0.5f;
    float ht = rootW * 0.12f;
    float hd = depth * 0.5f;
    float sideN = len, sideNy = hr - ht;
    float sideNl = sqrtf(sideN * sideN + sideNy * sideNy);

    // main blade body
    glBegin(GL_QUADS);
        glNormal3f(0, 0, 1);
        glVertex3f(-hr, 0,   hd); glVertex3f( hr, 0,   hd);
        glVertex3f( ht, len, hd); glVertex3f(-ht, len, hd);
        glNormal3f(0, 0, -1);
        glVertex3f( hr, 0,  -hd); glVertex3f(-hr, 0,  -hd);
        glVertex3f(-ht, len,-hd); glVertex3f( ht, len,-hd);
        glNormal3f( sideN / sideNl, sideNy / sideNl, 0);
        glVertex3f( hr, 0,   hd); glVertex3f( hr, 0,  -hd);
        glVertex3f( ht, len,-hd); glVertex3f( ht, len, hd);
        glNormal3f(-sideN / sideNl, sideNy / sideNl, 0);
        glVertex3f(-hr, 0,  -hd); glVertex3f(-hr, 0,   hd);
        glVertex3f(-ht, len, hd); glVertex3f(-ht, len,-hd);
        glNormal3f(0, 1, 0);
        glVertex3f(-ht, len,-hd); glVertex3f(-ht, len, hd);
        glVertex3f( ht, len, hd); glVertex3f( ht, len,-hd);
        glNormal3f(0, -1, 0);
        glVertex3f( hr, 0,   hd); glVertex3f(-hr, 0,   hd);
        glVertex3f(-hr, 0,  -hd); glVertex3f( hr, 0,  -hd);
    glEnd();

    // central spine spar on front face
    SetMaterial(0.52f, 0.38f, 0.18f, 16.0f);
    glPushMatrix();
        glTranslatef(0, len * 0.5f, hd + 0.04f);
        DrawBox(0.06f, len, 0.08f);
    glPopMatrix();

    // cross-braces at 1/3 and 2/3 span
    for (int ci = 1; ci <= 2; ci++) {
        float frac = ci / 3.0f;
        float hw   = hr + (ht - hr) * frac;
        glPushMatrix();
            glTranslatef(0, len * frac, hd + 0.04f);
            DrawBox(hw * 2.0f * 0.88f, 0.04f, 0.08f);
        glPopMatrix();
    }
}

void DrawWindmill() {
    const int   sides  = 8;
    const float baseR  = 0.45f, topR = 0.20f, height = 5.0f;

    // ── foundation cobble ring ────────────────────────────────────────────────
    const int   fSegs  = 16;
    const float fOuter = baseR + 0.32f;
    SetMaterial(0.38f, 0.33f, 0.26f, 4.0f);
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0, 1, 0);
    for (int i = 0; i <= fSegs; i++) {
        float a = (float)i / fSegs * 2.0f * (float)M_PI;
        glVertex3f(cosf(a) * fOuter, 0.01f, sinf(a) * fOuter);
        glVertex3f(cosf(a) * baseR,  0.01f, sinf(a) * baseR);
    }
    glEnd();

    // ── octagonal tapered tower ───────────────────────────────────────────────
    SetMaterial(0.72f, 0.62f, 0.48f, 8.0f);
    glBegin(GL_QUADS);
    for (int i = 0; i < sides; i++) {
        float a0 = (float)i       / sides * 2.0f * (float)M_PI;
        float a1 = (float)(i + 1) / sides * 2.0f * (float)M_PI;
        float bx0 = cosf(a0)*baseR, bz0 = sinf(a0)*baseR;
        float bx1 = cosf(a1)*baseR, bz1 = sinf(a1)*baseR;
        float tx0 = cosf(a0)*topR,  tz0 = sinf(a0)*topR;
        float tx1 = cosf(a1)*topR,  tz1 = sinf(a1)*topR;
        float nmx = (bx0+bx1)*0.5f, nmz = (bz0+bz1)*0.5f;
        float nml = sqrtf(nmx*nmx + nmz*nmz);
        glNormal3f(nmx/nml, 0, nmz/nml);
        glVertex3f(bx0,0,bz0); glVertex3f(bx1,0,bz1);
        glVertex3f(tx1,height,tz1); glVertex3f(tx0,height,tz0);
    }
    glEnd();

    // ── stone banding rings ───────────────────────────────────────────────────
    const float bandH[] = {1.0f, 2.0f, 3.0f, 4.0f};
    SetMaterial(0.60f, 0.52f, 0.40f, 8.0f);
    for (float bh : bandH) {
        float br = baseR + (topR - baseR) * (bh / height) + 0.025f;
        glBegin(GL_QUADS);
        for (int i = 0; i < sides; i++) {
            float a0 = (float)i     / sides * 2.0f * (float)M_PI;
            float a1 = (float)(i+1) / sides * 2.0f * (float)M_PI;
            float x0=cosf(a0)*br, z0=sinf(a0)*br;
            float x1=cosf(a1)*br, z1=sinf(a1)*br;
            float nx=(x0+x1)*0.5f, nz=(z0+z1)*0.5f, nl=sqrtf(nx*nx+nz*nz);
            glNormal3f(nx/nl, 0, nz/nl);
            glVertex3f(x0,bh,       z0); glVertex3f(x1,bh,       z1);
            glVertex3f(x1,bh+0.06f, z1); glVertex3f(x0,bh+0.06f, z0);
        }
        glEnd();
    }

    // ── arched door on +Z face ────────────────────────────────────────────────
    const float doorZ = 0.41f;
    // stone arch frame
    SetMaterial(0.70f, 0.60f, 0.44f, 8.0f);
    glPushMatrix();
        glTranslatef(0, 0, doorZ);
        DrawBox(0.64f, 1.0f, 0.07f);
        // semicircular arch above rectangle
        glBegin(GL_TRIANGLE_FAN);
        glNormal3f(0, 0, 1);
        glVertex3f(0, 1.0f, 0.035f);
        for (int i = 0; i <= 10; i++) {
            float a = (float)M_PI - (float)i / 10.0f * (float)M_PI;
            glVertex3f(cosf(a)*0.32f, 1.0f + sinf(a)*0.32f, 0.035f);
        }
        glEnd();
    glPopMatrix();
    // door planks (dark wood, slightly forward)
    SetMaterial(0.30f, 0.20f, 0.10f, 8.0f);
    for (int p = -1; p <= 1; p++) {
        glPushMatrix();
            glTranslatef(p * 0.175f, 0, doorZ + 0.025f);
            DrawBox(0.14f, 0.95f, 0.04f);
        glPopMatrix();
    }
    // iron strap hinges
    SetMaterial(0.20f, 0.18f, 0.16f, 48.0f);
    {
        const float hingeY[] = {0.22f, 0.72f};
        for (float hy : hingeY) {
            glPushMatrix();
                glTranslatef(0, hy, doorZ + 0.05f);
                DrawBox(0.46f, 0.035f, 0.04f);
            glPopMatrix();
        }
    }
    // threshold stone
    SetMaterial(0.68f, 0.60f, 0.46f, 12.0f);
    glPushMatrix();
        glTranslatef(0, 0, doorZ + 0.07f);
        DrawBox(0.70f, 0.06f, 0.14f);
    glPopMatrix();

    // ── stone-framed window on +Z face ────────────────────────────────────────
    const float winY = 2.40f;
    const float winZ = baseR + (topR - baseR) * (winY / height) + 0.01f;
    // stone surround
    SetMaterial(0.74f, 0.68f, 0.52f, 12.0f);
    glPushMatrix(); glTranslatef(0, winY+0.56f, winZ); DrawBox(0.64f, 0.10f, 0.08f); glPopMatrix(); // lintel
    glPushMatrix(); glTranslatef(0, winY-0.04f, winZ); DrawBox(0.72f, 0.09f, 0.10f); glPopMatrix(); // sill
    glPushMatrix(); glTranslatef(-0.27f, winY, winZ);  DrawBox(0.08f, 0.56f, 0.08f); glPopMatrix(); // left jamb
    glPushMatrix(); glTranslatef( 0.27f, winY, winZ);  DrawBox(0.08f, 0.56f, 0.08f); glPopMatrix(); // right jamb
    // glass pane
    SetMaterial(0.08f, 0.12f, 0.22f, 80.0f);
    glPushMatrix(); glTranslatef(0, winY, winZ - 0.01f); DrawBox(0.44f, 0.56f, 0.06f); glPopMatrix();
    // iron window bars
    SetMaterial(0.20f, 0.18f, 0.16f, 48.0f);
    glPushMatrix(); glTranslatef(0, winY+0.28f, winZ+0.02f); DrawBox(0.03f, 0.56f, 0.04f); glPopMatrix();
    glPushMatrix(); glTranslatef(0, winY+0.28f, winZ+0.02f); DrawBox(0.44f, 0.03f, 0.04f); glPopMatrix();

    // ── conical slate cap on tower top ────────────────────────────────────────
    const int   capSegs = 16;
    const float capBase = topR + 0.08f, capH = 1.0f;
    SetMaterial(0.26f, 0.26f, 0.30f, 24.0f);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < capSegs; i++) {
        float a0 = (float)i       / capSegs * 2.0f * (float)M_PI;
        float a1 = (float)(i + 1) / capSegs * 2.0f * (float)M_PI;
        float x0=cosf(a0)*capBase, z0=sinf(a0)*capBase;
        float x1=cosf(a1)*capBase, z1=sinf(a1)*capBase;
        triNormal(x0,height,z0, x1,height,z1, 0,height+capH,0);
        glVertex3f(x0,height,z0); glVertex3f(x1,height,z1); glVertex3f(0,height+capH,0);
    }
    glEnd();
    // underside ring (cap overhang)
    glBegin(GL_TRIANGLE_STRIP);
    glNormal3f(0,-1,0);
    for (int i = 0; i <= capSegs; i++) {
        float a = (float)i / capSegs * 2.0f * (float)M_PI;
        glVertex3f(cosf(a)*topR,    height, sinf(a)*topR);
        glVertex3f(cosf(a)*capBase, height, sinf(a)*capBase);
    }
    glEnd();
    // finial spike at apex
    SetMaterial(0.14f, 0.13f, 0.15f, 48.0f);
    glPushMatrix();
        glTranslatef(0, height + capH, 0);
        DrawBox(0.04f, 0.38f, 0.04f);
    glPopMatrix();

    // ── axle shaft (tower top face → hub back) ────────────────────────────────
    const int   shaftSegs = 8;
    const float shaftR    = 0.055f, shaftLen = 0.22f;
    const float shaftZ0   = topR, shaftZ1 = topR + shaftLen;
    SetMaterial(0.22f, 0.20f, 0.18f, 32.0f);
    glBegin(GL_QUADS);
    for (int i = 0; i < shaftSegs; i++) {
        float a0 = (float)i     / shaftSegs * 2.0f * (float)M_PI;
        float a1 = (float)(i+1) / shaftSegs * 2.0f * (float)M_PI;
        float x0=cosf(a0)*shaftR, y0=sinf(a0)*shaftR;
        float x1=cosf(a1)*shaftR, y1=sinf(a1)*shaftR;
        float nx=(x0+x1)*0.5f, ny=(y0+y1)*0.5f, nl=sqrtf(nx*nx+ny*ny);
        glNormal3f(nx/nl, ny/nl, 0);
        glVertex3f(x0, height+y0, shaftZ0); glVertex3f(x1, height+y1, shaftZ0);
        glVertex3f(x1, height+y1, shaftZ1); glVertex3f(x0, height+y0, shaftZ1);
    }
    glEnd();

    // ── hub disc (octagonal cylinder, face toward viewer) ─────────────────────
    const int   hubSegs = 8;
    const float hubR    = 0.22f, hubD = 0.14f;
    const float hubZ    = shaftZ1 + hubD * 0.5f;
    SetMaterial(0.40f, 0.30f, 0.18f, 16.0f);
    // side wall
    glBegin(GL_QUADS);
    for (int i = 0; i < hubSegs; i++) {
        float a0=(float)i    /hubSegs*2.0f*(float)M_PI;
        float a1=(float)(i+1)/hubSegs*2.0f*(float)M_PI;
        float x0=cosf(a0)*hubR, y0=sinf(a0)*hubR;
        float x1=cosf(a1)*hubR, y1=sinf(a1)*hubR;
        float nx=(x0+x1)*0.5f, ny=(y0+y1)*0.5f, nl=sqrtf(nx*nx+ny*ny);
        glNormal3f(nx/nl, ny/nl, 0);
        glVertex3f(x0,height+y0,hubZ-hubD*0.5f); glVertex3f(x1,height+y1,hubZ-hubD*0.5f);
        glVertex3f(x1,height+y1,hubZ+hubD*0.5f); glVertex3f(x0,height+y0,hubZ+hubD*0.5f);
    }
    glEnd();
    // front cap (+Z)
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,0,1);
    glVertex3f(0, height, hubZ+hubD*0.5f);
    for (int i = 0; i <= hubSegs; i++) {
        float a=(float)i/hubSegs*2.0f*(float)M_PI;
        glVertex3f(cosf(a)*hubR, height+sinf(a)*hubR, hubZ+hubD*0.5f);
    }
    glEnd();
    // back cap (-Z)
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0,0,-1);
    glVertex3f(0, height, hubZ-hubD*0.5f);
    for (int i = hubSegs; i >= 0; i--) {
        float a=(float)i/hubSegs*2.0f*(float)M_PI;
        glVertex3f(cosf(a)*hubR, height+sinf(a)*hubR, hubZ-hubD*0.5f);
    }
    glEnd();
    // hub spokes (4 flat bars on front face)
    SetMaterial(0.32f, 0.24f, 0.12f, 12.0f);
    for (int s = 0; s < 4; s++) {
        glPushMatrix();
            glTranslatef(0, height, hubZ + hubD*0.5f + 0.01f);
            glRotatef(s * 45.0f, 0, 0, 1);
            DrawBox(0.04f, hubR*2.0f, 0.03f);
        glPopMatrix();
    }

    // ── four tapered blades ───────────────────────────────────────────────────
    const float bladeStartZ = shaftZ1 + hubD + 0.02f;
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
            glTranslatef(0, height, bladeStartZ);
            glRotatef(windmillAngle + i * 90.0f, 0, 0, 1);
            glTranslatef(0, hubR + 0.02f, 0);
            SetMaterial(0.82f, 0.74f, 0.52f, 16.0f);
            DrawBlade(2.2f, 0.30f, 0.07f);
        glPopMatrix();
    }

    // ── iron ladder on -Z face ────────────────────────────────────────────────
    const float ldrZ = -((baseR + topR) * 0.5f) - 0.01f;
    SetMaterial(0.22f, 0.20f, 0.17f, 32.0f);
    {
        const float lxVals[] = {-0.12f, 0.12f};
        for (float lx : lxVals) {
            glPushMatrix(); glTranslatef(lx, 0, ldrZ); DrawBox(0.02f, 4.0f, 0.04f); glPopMatrix();
        }
    }
    for (int r = 0; r < 12; r++) {
        float ry = 0.30f + r * (3.7f / 11.0f);
        glPushMatrix(); glTranslatef(0, ry, ldrZ); DrawBox(0.28f, 0.025f, 0.04f); glPopMatrix();
    }
}

// window glow billboard — call from transparent pass inside windmill-local transform
// rx/ry/rz = camera right world vec, ux/uy/uz = camera up world vec
void DrawWindmillGlow(float rx, float ry, float rz,
                      float ux, float uy, float uz) {
    const float baseR = 0.45f, topR = 0.20f, height = 5.0f;
    const float wcy   = 2.40f + 0.28f;   // window centre Y
    const float wcz   = baseR + (topR - baseR) * (wcy / height) + 0.09f; // in front of window
    glDisable(GL_LIGHTING);
    glPushMatrix();
        glTranslatef(0, wcy, wcz);
        // outer corona
        glColor4f(1.0f, 0.72f, 0.28f, 0.07f);
        {
            const float s = 0.90f;
            glBegin(GL_QUADS);
                glVertex3f((-rx+ux)*s,(-ry+uy)*s,(-rz+uz)*s);
                glVertex3f(( rx+ux)*s,( ry+uy)*s,( rz+uz)*s);
                glVertex3f(( rx-ux)*s,( ry-uy)*s,( rz-uz)*s);
                glVertex3f((-rx-ux)*s,(-ry-uy)*s,(-rz-uz)*s);
            glEnd();
        }
        // core glow
        glColor4f(1.0f, 0.80f, 0.35f, 0.22f);
        {
            const float s = 0.50f;
            glBegin(GL_QUADS);
                glVertex3f((-rx+ux)*s,(-ry+uy)*s,(-rz+uz)*s);
                glVertex3f(( rx+ux)*s,( ry+uy)*s,( rz+uz)*s);
                glVertex3f(( rx-ux)*s,( ry-uy)*s,( rz-uz)*s);
                glVertex3f((-rx-ux)*s,(-ry-uy)*s,(-rz-uz)*s);
            glEnd();
        }
    glPopMatrix();
    glEnable(GL_LIGHTING);
}

void DrawWindmillShadow() {
    glDisable(GL_LIGHTING);
    glColor4f(0.0f, 0.0f, 0.0f, 0.38f);
    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(0, 1, 0);
    glVertex3f(0, 0.005f, 0);
    for (int i = 0; i <= 16; i++) {
        float a = (float)i / 16.0f * 2.0f * (float)M_PI;
        glVertex3f(cosf(a)*1.8f, 0.005f, sinf(a)*1.5f);
    }
    glEnd();
    glEnable(GL_LIGHTING);
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
    // corners rise to 2; all inner control points at 0 — centre y≈0.125
    static float cp[4][4][3] = {
        {{-40,2.0f,-40},{-13,0.0f,-40},{ 13,0.0f,-40},{40,2.0f,-40}},
        {{-40,0.0f,-13},{-13,0.0f,-13},{ 13,0.0f,-13},{40,0.0f,-13}},
        {{-40,0.0f, 13},{-13,0.0f, 13},{ 13,0.0f, 13},{40,0.0f, 13}},
        {{-40,2.0f, 40},{-13,0.0f, 40},{ 13,0.0f, 40},{40,2.0f, 40}},
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
