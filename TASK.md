# TASK.md — Implementace projektu MPC-MPG "Noční park"

Tento soubor je určen pro implementaci v Claude Code.
Implementuj přesně v pořadí fází. Každá fáze = jeden git commit.
Cílová platforma: macOS, CLion, C++17, OpenGL + GLUT (framework).

---

## Výchozí bod ✅

Soubor přejmenován na `xszucm00.cpp`. `imageLoad.h` přidán. `CMakeLists.txt` vytvořen.

CMakeLists.txt:
```cmake
cmake_minimum_required(VERSION 3.10)
project(mpg_projekt)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-DGL_SILENCE_DEPRECATION)
add_executable(mpg_projekt projekt.cpp)
target_link_libraries(mpg_projekt "-framework OpenGL" "-framework GLUT")
```

---

## FÁZE 1 — Základ scény ✅ (commit: "phase1: scene base")

### 1A — Globální stav kamery

Nahraď globální proměnné šablony (xnew, ynew, tranz, tranx…) tímto stavem:

```cpp
float camX = 0.0f, camY = 1.7f, camZ = 20.0f;
float yaw   = 0.0f;   // vodorovný úhel (radiány)
float pitch = 0.0f;   // svislý úhel (radiány, clamp ±1.5)
bool  keys[256] = {};
bool  mouseDown = false;
int   mouseLastX = 0, mouseLastY = 0;

// bob
float bobTimer    = 0.0f;
float bobOffset   = 0.0f;
bool  isMoving    = false;
float camFloorY   = 1.7f;  // základní výška očí

// svítilna
bool torchOn = false;

// animace
bool animOn = true;
float windmillAngle = 0.0f;

// textury
bool texOn = true;
unsigned int texGrass   = 0;
unsigned int texChecker = 0;

// text HUD
std::string lastAction = "start";

// projektily
struct Projectile { float x,y,z,vx,vy,vz; bool active; };
Projectile projectiles[10] = {};
```

### 1B — OnReshape a OnInit

```cpp
void OnReshape(int w, int h) {
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w/h, 0.1, 500.0);
}

void OnInit() {
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_SMOOTH);

    // osvětlení GL_LIGHT0 — měsíc (směrové)
    GLfloat amb0[]  = {0.05f,0.05f,0.15f,1.0f};
    GLfloat diff0[] = {0.3f, 0.3f, 0.5f, 1.0f};
    GLfloat spec0[] = {0.1f, 0.1f, 0.2f, 1.0f};
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  diff0);
    glLightfv(GL_LIGHT0, GL_SPECULAR, spec0);
    glEnable(GL_LIGHT0);

    // osvětlení GL_LIGHT1 — svítilna (spotlight, default off)
    GLfloat amb1[]  = {0.0f,0.0f,0.0f,1.0f};
    GLfloat diff1[] = {1.0f,0.9f,0.7f,1.0f};
    GLfloat spec1[] = {0.5f,0.5f,0.4f,1.0f};
    glLightfv(GL_LIGHT1, GL_AMBIENT,  amb1);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,  diff1);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec1);
    glLightf (GL_LIGHT1, GL_SPOT_CUTOFF,   15.0f);
    glLightf (GL_LIGHT1, GL_SPOT_EXPONENT, 8.0f);
    // GL_LIGHT1 zapnout/vypnout dle torchOn

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // procedurální textura (šachovnice 64×64)
    unsigned char checker[64*64*3];
    for(int i=0;i<64;i++) for(int j=0;j<64;j++){
        int idx=(i*64+j)*3;
        unsigned char c=((i/8+j/8)%2)?200:80;
        checker[idx]=checker[idx+1]=checker[idx+2]=c;
    }
    glGenTextures(1, &texChecker);
    glBindTexture(GL_TEXTURE_2D, texChecker);
    gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,64,64,GL_RGB,GL_UNSIGNED_BYTE,checker);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    // externí textura (grass.bmp)
    setTexture("grass.bmp", &texGrass, true);

    // glut timer
    glutTimerFunc(16, OnTimer, 0);
}
```

### 1C — OnTimer (herní smyčka, 60fps)

```cpp
void OnTimer(int) {
    float dt = 0.016f;
    float speed = 5.0f;
    float dx=0, dz=0;

    if(keys['w']||keys['W']||keys[GLUT_KEY_UP  +200]) { dx+= sinf(yaw)*speed*dt; dz+= cosf(yaw)*speed*dt; }
    if(keys['s']||keys['S']||keys[GLUT_KEY_DOWN +200]) { dx-= sinf(yaw)*speed*dt; dz-= cosf(yaw)*speed*dt; }
    if(keys['a']||keys['A'])                           { dx+= cosf(yaw)*speed*dt; dz-= sinf(yaw)*speed*dt; }
    if(keys['d']||keys['D'])                           { dx-= cosf(yaw)*speed*dt; dz+= sinf(yaw)*speed*dt; }

    camX += dx; camZ += dz;
    isMoving = (dx*dx+dz*dz > 0.0001f);

    // camera bobbing
    if(isMoving) {
        bobTimer += dt;
        bobOffset = sinf(bobTimer * 8.0f) * 0.15f;
    } else {
        bobOffset *= 0.85f;  // útlum
        if(fabsf(bobOffset) < 0.001f) bobOffset = 0.0f;
    }

    // animace větrníku
    if(animOn) windmillAngle += 120.0f * dt;  // 120 deg/s

    // projektily
    for(auto& p : projectiles) {
        if(!p.active) continue;
        p.vy -= 9.8f * dt;
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.z  += p.vz * dt;
        if(p.y < -10.0f) p.active = false;
    }

    glutPostRedisplay();
    glutTimerFunc(16, OnTimer, 0);
}
```

### 1D — Pohyb šipek přes OnSpecial

```cpp
void OnSpecial(int key, int, int) {
    // šipky ukládáme do keys[] s offsetem 200, aby nekolidovaly s ASCII
    if(key==GLUT_KEY_UP)    keys[GLUT_KEY_UP  +200]=true;
    if(key==GLUT_KEY_DOWN)  keys[GLUT_KEY_DOWN+200]=true;
    if(key==GLUT_KEY_PAGE_UP)  { camFloorY+=0.5f; if(camFloorY>30.f) camFloorY=30.f; lastAction="kamera nahoru"; }
    if(key==GLUT_KEY_PAGE_DOWN){ camFloorY-=0.5f; if(camFloorY<-5.f) camFloorY=-5.f; lastAction="kamera dolu";   }
    glutPostRedisplay();
}
void OnSpecialUp(int key, int, int) {
    if(key==GLUT_KEY_UP)   keys[GLUT_KEY_UP  +200]=false;
    if(key==GLUT_KEY_DOWN) keys[GLUT_KEY_DOWN+200]=false;
}
```

Registrovat: `glutSpecialUpFunc(OnSpecialUp);`

### 1E — Myš

```cpp
void OnMouseButton(int button, int state, int x, int y) {
    if(button==GLUT_LEFT_BUTTON) {
        mouseDown = (state==GLUT_DOWN);
        mouseLastX=x; mouseLastY=y;
    }
}
void OnMouseMotion(int x, int y) {
    if(!mouseDown) return;
    float sens = 0.005f;
    yaw   += (x - mouseLastX) * sens;
    pitch -= (y - mouseLastY) * sens;
    if(pitch >  1.5f) pitch= 1.5f;
    if(pitch < -1.5f) pitch=-1.5f;
    mouseLastX=x; mouseLastY=y;
    glutPostRedisplay();
}
void OnPassiveMotion(int x, int y) {}  // registrovat prázdnou (potlačí GLUT warning)
```

### 1F — Klávesnice

```cpp
void OnKeyboard(unsigned char key, int, int) {
    keys[key]=true;
    if(key==27) exit(0);  // Esc
    if(key=='r'||key=='R') {
        torchOn=!torchOn;
        lastAction = torchOn ? "svitilna ON" : "svitilna OFF";
    }
    if(key==' ') SpawnProjectile();
}
void OnKeyboardUp(unsigned char key, int, int) { keys[key]=false; }
```

### 1G — Aplikace kamery v OnDisplay

```cpp
glMatrixMode(GL_MODELVIEW);
glLoadIdentity();

// kamera přes rotace + translace (bez gluLookAt)
glRotatef(-pitch * (180.0f/M_PI), 1,0,0);
glRotatef(-yaw   * (180.0f/M_PI), 0,1,0);
glTranslatef(-camX, -(camFloorY + bobOffset), -camZ);

// směrové světlo GL_LIGHT0 (musí být APÓS rotaci)
GLfloat moonPos[] = {0.3f, 1.0f, 0.2f, 0.0f};
glLightfv(GL_LIGHT0, GL_POSITION, moonPos);

// spotlight GL_LIGHT1 — na pozici kamery, ve směru pohledu
if(torchOn) {
    glEnable(GL_LIGHT1);
    GLfloat lpos[] = {camX, camFloorY+bobOffset, camZ, 1.0f};
    GLfloat ldir[] = {
        sinf(yaw)*cosf(pitch),
        -sinf(pitch),
        cosf(yaw)*cosf(pitch)
    };
    glLightfv(GL_LIGHT1, GL_POSITION, lpos);
    glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, ldir);
} else {
    glDisable(GL_LIGHT1);
}
```

---

## FÁZE 2 — Objekty scény ✅ (commit: "phase2: scene objects")

### 2A — Pomocné funkce materiálů

```cpp
void SetMaterial(float r,float g,float b,float shin=32.0f) {
    GLfloat amb[]  = {r*0.2f, g*0.2f, b*0.2f, 1.0f};
    GLfloat diff[] = {r,      g,      b,      1.0f};
    GLfloat spec[] = {0.3f,   0.3f,   0.3f,   1.0f};
    glMaterialfv(GL_FRONT, GL_AMBIENT,   amb);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT, GL_SHININESS, shin);
}
```

### 2B — DrawBox(w, h, d)

Vykreslí kvádr o rozměrech w×h×d se středem ve (0,h/2,0) — tj. spodní stěna na Y=0.
Každá stěna má vlastní normálu. Použij `GL_QUADS`.

Normály:
- přední (+Z): {0,0,1}
- zadní (-Z):  {0,0,-1}
- pravá (+X):  {1,0,0}
- levá (-X):   {-1,0,0}
- horní (+Y):  {0,1,0}
- spodní (-Y): {0,-1,0} (jen pokud potřeba)

Přidej UV souřadnice pro texturování: `glTexCoord2f` před každým `glVertex3f`.

### 2C — DrawTree()

```
kmen: DrawBox(0.4, 3.0, 0.4) — hnědý materiál
koruna: jehlan — základna 2.5×2.5, výška 3.0, 8 trojúhelníků bočnic
  střed koruny na (0, 3.0, 0) — zelený materiál
```

Normály jehlanu: pro každý trojúhelník `(base_left, base_right, apex)` → křížový součin dvou hran.

### 2D — DrawBench()

```
sedák: DrawBox(2.0, 0.1, 0.5) na Y=0.7
4 nohy: DrawBox(0.1, 0.7, 0.1) na rozích
opěradlo: DrawBox(2.0, 0.6, 0.1) naklonění ~15° (glRotatef)
```

### 2E — DrawLantern()

```
sloup: DrawBox(0.1, 4.0, 0.1) — tmavý kovový materiál
těleso lucerny na Y=4.0: DrawBox(0.4, 0.5, 0.4) — světlý žlutý materiál
```

Průhledné okno: DrawBox(0.42, 0.4, 0.42) — vykreslit zvlášť s alpha=0.4 (viz fáze 4).

### 2F — DrawShed()

```
základ: DrawBox(4.0, 2.5, 3.0)
střecha (sedlová) — 2 trojúhelníkové štíty + 2 obdélníkové plochy
  hřeben střechy: Y=3.5 na středu (X=0), okapy Y=2.5 na X=±2.3
```

### 2G — DrawBoulder()

Nepravidelný polyhedron z ~10 trojúhelníků. Vertexy definovat jako globální pole:

```cpp
float boulderVerts[][3] = {
    { 0,  1.2f, 0}, {-1,0,-0.5f}, { 1,0,-0.5f},
    { 0,  1.0f, 0}, { 1,0,-0.5f}, { 0.8f,0, 1},
    // ... cca 10 trojúhelníků
};
// vykreslit přes GL_TRIANGLES
// normála každého trojúhelníku = křížový součin hran
```

### 2H — DrawFence(int count, float spacing)

Smyčka count-krát: `glTranslatef(spacing,0,0); DrawBox(0.1, 1.2, 0.1);`

### 2I — DrawWindmill()

```
sloup: DrawBox(0.3, 5.0, 0.3)
4 lopatky na Y=5.0: glRotatef(windmillAngle + i*90, 0,0,1); DrawBox(0.15, 2.0, 0.05);
```

### 2J — Sestavení scény v OnDisplay

Rozmístění (přibližné souřadnice):

```
terén (Bézier nebo dočasně rovná plocha) — vykreslí se první
stromy:    (-5,0,-5), (8,0,-3), (-10,0,5), (3,0,10), (-2,0,15)
lavičky:   (0,0,0) rotace 0°, (6,0,-2) rotace 45°
lucerny:   (4,0,0), (-4,0,3)
kůlna:     (12,0,-8)
balvany:   (-8,0,8), (2,0,-10)
plot:      DrawFence(10, 1.2) na z=-15
větrník:   (0,0,-20)
```

---

## FÁZE 3 — Bézierovy pláty + textury ✅ (commit: "phase3: bezier terrain + textures")

### 3A — DrawTerrain()

```cpp
void DrawTerrain() {
    static float cp[4][4][3] = {
        {{-40,0,-40},{-13,4,-40},{ 13,3,-40},{40,0,-40}},
        {{-40,3,-13},{-13,9,-13},{ 13,7,-13},{40,2,-13}},
        {{-40,2, 13},{-13,6, 13},{ 13,8, 13},{40,3, 13}},
        {{-40,0, 40},{-13,3, 40},{ 13,4, 40},{40,0, 40}},
    };

    glMap2f(GL_MAP2_VERTEX_3,
        0,1, 3, 4,   // u: min,max, stride=3, order=4
        0,1, 12, 4,  // v: min,max, stride=12, order=4
        &cp[0][0][0]);
    glEnable(GL_MAP2_VERTEX_3);
    glEnable(GL_AUTO_NORMAL);
    glMapGrid2f(30, 0,1, 30, 0,1);

    SetMaterial(0.2f, 0.6f, 0.2f);

    if(texOn && texGrass) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texGrass);
        glEnable(GL_MAP2_TEXTURE_COORD_2);
        // texturové souřadnice mapovat na u,v terénních parametrů * 5 (repeat)
        float tcp[4][4][2] = {
            {{0,0},{1.67f,0},{3.33f,0},{5,0}},
            {{0,1.67f},{1.67f,1.67f},{3.33f,1.67f},{5,1.67f}},
            {{0,3.33f},{1.67f,3.33f},{3.33f,3.33f},{5,3.33f}},
            {{0,5},{1.67f,5},{3.33f,5},{5,5}},
        };
        glMap2f(GL_MAP2_TEXTURE_COORD_2, 0,1,2,4, 0,1,8,4, &tcp[0][0][0]);
    }

    glEvalMesh2(GL_FILL, 0,30, 0,30);

    glDisable(GL_MAP2_VERTEX_3);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_MAP2_TEXTURE_COORD_2);
    if(texOn) glDisable(GL_TEXTURE_2D);
}
```

### 3B — Textura na cestu (procedurální checker)

Vykresli pás cesty (DrawBox nebo quad) s texChecker:

```cpp
glEnable(GL_TEXTURE_2D);
glBindTexture(GL_TEXTURE_2D, texChecker);
// ... DrawBox s UV souřadnicemi
glDisable(GL_TEXTURE_2D);
```

---

## FÁZE 4 — UI: menu, HUD text, průhlednost (commit: "phase4: ui + transparency")

### 4A — Menu

```cpp
void OnMenu(int val) {
    switch(val) {
        case 1: camX=0; camY=1.7f; camZ=20; yaw=0; pitch=0; camFloorY=1.7f; lastAction="reset kamery"; break;
        case 2: animOn=!animOn; lastAction=animOn?"animace ON":"animace OFF"; break;
        case 3: texOn=!texOn;   lastAction=texOn?"textury ON":"textury OFF"; break;
        case 4:
            if(glIsEnabled(GL_LIGHT0)) { glDisable(GL_LIGHT0); lastAction="svetlo OFF"; }
            else                       { glEnable (GL_LIGHT0); lastAction="svetlo ON";  }
            break;
        case 5: torchOn=!torchOn; lastAction=torchOn?"svitilna ON":"svitilna OFF"; break;
        case 6: exit(0);
    }
    glutPostRedisplay();
}

// v main():
int menu = glutCreateMenu(OnMenu);
glutAddMenuEntry("Reset kamery",   1);
glutAddMenuEntry("Animace ON/OFF", 2);
glutAddMenuEntry("Textury ON/OFF", 3);
glutAddMenuEntry("Svetlo ON/OFF",  4);
glutAddMenuEntry("Svitilna ON/OFF",5);
glutAddMenuEntry("Konec",          6);
glutAttachMenu(GLUT_RIGHT_BUTTON);
```

### 4B — DrawHUD

Viz README.md sekce "Výpis textu". Volat na konci OnDisplay (po glutSwapBuffers ne — před ním).

### 4C — Průhledné okno lucerny

V DrawLantern: neprůhledné části normálně, pak zvlášť:

```cpp
// průhledné okno — volat z OnDisplay po ostatních objektech
void DrawLanternWindow(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);
    // materiál průhledný
    GLfloat col[] = {1.0f,0.9f,0.5f,0.4f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE, col);
    // DrawBox pro okno
    glPopMatrix();
}
```

OnDisplay pořadí:
```
1. neprůhledné objekty (depth write ON)
2. glEnable(GL_BLEND); glDepthMask(GL_FALSE);
3. průhledné objekty
4. glDepthMask(GL_TRUE); glDisable(GL_BLEND);
5. DrawHUD(lastAction.c_str())
```

---

## FÁZE 5 — Hod předmětu (commit: "phase5: projectile")

### 5A — SpawnProjectile

```cpp
void SpawnProjectile() {
    for(auto& p : projectiles) {
        if(p.active) continue;
        float spd = 20.0f;
        p.x = camX; p.y = camFloorY + bobOffset; p.z = camZ;
        p.vx = -sinf(yaw)*cosf(pitch)*spd;
        p.vy =  sinf(pitch)*spd;
        p.vz = -cosf(yaw)*cosf(pitch)*spd;
        p.active = true;
        lastAction = "hod predmetu";
        break;
    }
}
```

### 5B — DrawProjectiles

Kulička = ikosahedron nebo jednoduše 8stranný polyhedron z ručně zadaných vertexů (ne glutSolidSphere).
Příklad: 8stěnný: vertexy {±r,0,0}, {0,±r,0}, {0,0,±r}, 8 trojúhelníků.

```cpp
float octVerts[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
int octFaces[8][3]   = {{0,2,4},{2,1,4},{1,3,4},{3,0,4},{0,3,5},{3,1,5},{1,2,5},{2,0,5}};

void DrawOctahedron(float r) {
    glBegin(GL_TRIANGLES);
    for(auto& f : octFaces) {
        // normála = průměr 3 vertexů (pro kouli OK)
        float nx=(octVerts[f[0]][0]+octVerts[f[1]][0]+octVerts[f[2]][0])/3;
        float ny=(octVerts[f[0]][1]+octVerts[f[1]][1]+octVerts[f[2]][1])/3;
        float nz=(octVerts[f[0]][2]+octVerts[f[1]][2]+octVerts[f[2]][2])/3;
        glNormal3f(nx,ny,nz);
        for(int v : f)
            glVertex3f(octVerts[v][0]*r, octVerts[v][1]*r, octVerts[v][2]*r);
    }
    glEnd();
}

void DrawProjectiles() {
    SetMaterial(0.9f,0.2f,0.2f,64.0f);
    for(auto& p : projectiles) {
        if(!p.active) continue;
        glPushMatrix();
        glTranslatef(p.x,p.y,p.z);
        DrawOctahedron(0.2f);
        glPopMatrix();
    }
}
```

---

## FÁZE 6 — Finalizace (commit: "phase6: final cleanup")

### 6A — Hlavička souboru

Na začátek `projekt.cpp`:

```cpp
/*
 * MPC-MPG 2025/26 - Semestralni projekt
 *
 * Autor:       Martin Szüč
 * Student ID:  (doplň)
 * Email:       (doplň)@vut.cz
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
 *   7.  Rucni svitilna (klávesa R)                 2 b
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
 *   pravé tlacitko mys  menu
 *   Esc                 konec
 *
 * Konfigurace: macOS 14, CLion, OpenGL/GLUT (framework)
 */
```

### 6B — Kontrolní checklist před odevzdáním

- [ ] Kompiluje bez chyb (`cmake --build .`)
- [ ] Backface culling je zapnutý (`glEnable(GL_CULL_FACE)`)
- [ ] Perspektivní projekce (`gluPerspective`)
- [ ] Všech 5+ vlastních objektů viditelných ve scéně
- [ ] Větrník se točí
- [ ] Osvětlení GL_LIGHT0 aktivní, objekty stínovány
- [ ] Pohyb myší + WASD funguje
- [ ] Menu se otevře right-clickem, všech 6 položek funguje
- [ ] HUD text se mění s každou akcí
- [ ] Svítilna R funguje (viditelný kužel světla)
- [ ] Page Up/Down mění výšku
- [ ] Camera bobbing při chůzi, útlum při zastavení
- [ ] Průhledné okno (alpha blending)
- [ ] Obě textury viditelné (grass + checker)
- [ ] Bézierový terén s vlnami
- [ ] Space hodí kuličku, která padá (gravitace)
- [ ] `imageLoad.h` a `grass.bmp` přítomny

### 6C — Soubory k odevzdání

```
projekt.cpp
imageLoad.h
grass.bmp
README_video.txt   ← link na YouTube demo
```

Zabalit do `xlogin_mpg_projekt.zip`.

---

## Poznámky pro implementaci

- **macOS OpenGL deprecation** — přidej `-DGL_SILENCE_DEPRECATION` do CMakeLists, varování jsou neškodná
- **glMap2f stride** — u parametru stride pro v = `4 * 3 = 12` (počet floatů na celý řádek 4×4 pole), ne 3
- **Průhlednost pořadí** — neprůhledné objekty VŽDY před průhlednými, jinak artefakty
- **Normály** — po `glScalef` nerovnoměrném je třeba `glEnable(GL_NORMALIZE)` (přidej do OnInit)
- **GL_AUTO_NORMAL** — funguje jen pro Bézier, ne pro ručně kreslené objekty
- **Camera bobbing útlum** — `*= 0.85f` každý frame (při 60fps = ~0.5s do zastavení), nezapomeň podmínku `isMoving`

---

## Additional improvement phases (post-submission)

These phases are not required for grading. They address real visual and code-quality gaps found during review.

---

### PHASE A — Lantern point lights (commit: "feat(lighting): add point lights at lantern positions")

**Problem:** lanterns have transparent glowing windows but cast no light on surrounding geometry. At night this is very noticeable — the grass and bench next to a lantern are identically dark as everything else.

**What to do:**

Use `GL_LIGHT2` and `GL_LIGHT3` (one per lantern). Set them as positional (w=1), warm yellow, with quadratic attenuation so the glow fades quickly.

```cpp
// in OnInit, after GL_LIGHT1 setup:
GLfloat lAmb[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat lDif[] = {1.0f, 0.85f, 0.4f, 1.0f};
GLfloat lSpc[] = {0.3f, 0.25f, 0.1f, 1.0f};
for (int li : {GL_LIGHT2, GL_LIGHT3}) {
    glLightfv(li, GL_AMBIENT,  lAmb);
    glLightfv(li, GL_DIFFUSE,  lDif);
    glLightfv(li, GL_SPECULAR, lSpc);
    glLightf (li, GL_CONSTANT_ATTENUATION,  0.3f);
    glLightf (li, GL_LINEAR_ATTENUATION,    0.2f);
    glLightf (li, GL_QUADRATIC_ATTENUATION, 0.08f);
    glEnable(li);
}
```

In `OnDisplay`, after the camera transform, set their positions:
```cpp
// lantern world positions match DrawScene hardcoded values
float lanternWorldPos[][4] = {{4,4.2f,0,1}, {-4,4.2f,3,1}};
glLightfv(GL_LIGHT2, GL_POSITION, lanternWorldPos[0]);
glLightfv(GL_LIGHT3, GL_POSITION, lanternWorldPos[1]);
```

The Y=4.2 places the light at the lantern head height.

Add a menu entry to toggle lantern lights (extend `OnMenu` case range and `glutAddMenuEntry`).

---

### PHASE B — Atmospheric fog (commit: "feat(atmosphere): add GL_FOG night fog")

**Problem:** the terrain edge and object pop-in are hard and unconvincing. Night scenes in real life always have reduced visibility at distance.

**What to do:**

```cpp
// in OnInit:
glEnable(GL_FOG);
GLfloat fogColor[] = {0.03f, 0.03f, 0.10f, 1.0f}; // dark blue-black
glFogfv(GL_FOG_COLOR,   fogColor);
glFogi (GL_FOG_MODE,    GL_EXP2);
glFogf (GL_FOG_DENSITY, 0.018f);  // tune: higher = thicker
glHint (GL_FOG_HINT,    GL_NICEST);
```

The clear color must match the fog color exactly or the horizon will show a seam:
```cpp
// in OnDisplay:
glClearColor(0.03f, 0.03f, 0.10f, 1.0f); // same as fogColor
```

Tune `GL_FOG_DENSITY` so objects at ~40 units (terrain edge) are nearly invisible but objects at ~10 units are clear.

---

### PHASE C — Night sky: stars + moon (commit: "feat(sky): star field and moon billboard")

**Problem:** background is a featureless black void. A night park should have a visible sky.

**What to do in `scene.h`:**

Generate a fixed set of star points once in `OnInit`, draw them as `GL_POINTS` in a pre-camera coordinate system (disable depth write, draw before everything else).

```cpp
// globals in scene.h:
static float starVerts[200][3]; // filled in OnInit
static int   starCount = 0;

// in OnInit:
srand(42); // deterministic
starCount = 200;
for (int i = 0; i < starCount; i++) {
    float theta = ((float)rand()/RAND_MAX) * 2.0f * (float)M_PI;
    float phi   = ((float)rand()/RAND_MAX) * (float)M_PI * 0.5f; // upper hemisphere only
    float r     = 80.0f;
    starVerts[i][0] = r * cosf(phi) * cosf(theta);
    starVerts[i][1] = r * sinf(phi);
    starVerts[i][2] = r * cosf(phi) * sinf(theta);
}

// DrawSky() — call before DrawTerrain in OnDisplay:
void DrawSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glPointSize(1.5f);
    glColor3f(0.9f, 0.9f, 1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < starCount; i++)
        glVertex3fv(starVerts[i]);
    glEnd();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}
```

Moon: a bright quad billboard at a fixed world direction (e.g. high on the +X side), drawn with `GL_QUADS` and a white emissive material, no lighting.

---

### PHASE D — Tree variation (commit: "feat(scene): randomised tree scale and yaw")

**Problem:** 5 trees are pixel-identical clones. The park looks like a copy-paste grid.

**What to do in `DrawScene`:**

Give each tree entry a scale factor and Y-rotation:

```cpp
struct TreeSpec { float x, z, scale, yaw; };
static const TreeSpec trees[] = {
    {-5,  -5, 1.0f,  0.0f},
    { 8,  -3, 1.3f, 40.0f},
    {-10,  5, 0.85f,120.0f},
    { 3,  10, 1.15f, 70.0f},
    {-2,  15, 1.0f, 200.0f},
};
for (auto& t : trees) {
    glPushMatrix();
        glTranslatef(t.x, 0, t.z);
        glRotatef(t.yaw, 0, 1, 0);
        glScalef(t.scale, t.scale, t.scale);
        DrawTree();
    glPopMatrix();
}
```

Because `glScalef` is non-uniform only along one axis here (uniform scale), `GL_NORMALIZE` (already enabled) handles the normal re-normalisation correctly.

---

### PHASE E — Real delta-time (commit: "fix(timer): replace hardcoded dt with glutElapsedTime")

**Problem:** `dt` is hardcoded to `0.016f`. If the system is under load and the timer fires late, physics and movement skip instead of catching up.

**What to do in `OnTimer`:**

```cpp
void OnTimer(int) {
    static int lastMs = 0;
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt  = (lastMs == 0) ? 0.016f : (nowMs - lastMs) * 0.001f;
    dt = (dt > 0.05f) ? 0.05f : dt; // clamp: max 50ms step
    lastMs = nowMs;
    // ... rest unchanged, replace literal 0.016f references with dt
}
```

The clamp prevents a single huge step if the window is unfocused for several seconds.

---

### PHASE F — Code cleanup: DrawScene side effect + debug output (commit: "refactor(scene): remove side effects from DrawScene, guard debug log")

**Problem 1:** `DrawScene()` resets and writes to `lanternCount`/`lanternPositions` every frame. A function named `Draw*` should only draw — the position registration is a hidden mutation that makes the transparent pass depend on draw order.

**Fix:** Remove the registration from `DrawScene`. Instead, define the lantern positions as a `constexpr` array in `scene.h` and use it both in `DrawScene` (for drawing) and in `OnDisplay` (for the transparent pass):

```cpp
// in scene.h — single source of truth:
struct LanternPos { float x, y, z; };
constexpr LanternPos kLanterns[] = {{4, 0, 0}, {-4, 0, 3}};
constexpr int kLanternCount = 2;
```

`DrawScene` iterates `kLanterns` to place lanterns. `OnDisplay` iterates `kLanterns` for the transparent pass. Delete `lanternPositions[]` and `lanternCount`.

**Problem 2:** `OnTimer` prints `[CAM]` to stdout every ~5 seconds unconditionally. Guard it:

```cpp
#ifdef DEBUG_CAM
    static int debugTick = 0;
    if (++debugTick >= 312) { debugTick = 0; printf("[CAM] ..."); fflush(stdout); }
#endif
```

Add `-DDEBUG_CAM` to the CMake debug config only. The `P` key dump can stay since it's explicit.
