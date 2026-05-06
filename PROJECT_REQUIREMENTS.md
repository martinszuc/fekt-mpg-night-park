# PROJECT_REQUIREMENTS.md
# Zadání semestrálního projektu MPC-MPG 2025/26

Zdroj: oficiální zadání projektu (dokument ze cvičení).

---

## Povinné (bez bodů, nutné vždy)

- Použít **perspektivní promítání**
- Použít **potlačené vykreslování zadních stěn** (backface culling)
- Vycházet ze zveřejněné šablony `xstude00.cpp`

---

## Základní bodování — až 24 bodů

Cíl: zvolit úkoly tak, aby součet byl přesně 24 b. Za základní projekt nelze získat více.
Lepší splnit méně úkolů správně než všechny s chybami (body se strhávají).

### 1. Modelování objektů — 3 b
- Alespoň **5 libovolných složitějších útvarů**
- Ideálně: ručně zadat souřadnice vertexů jako pole a vykreslovat přes ukazatel
- **Nezapočítávají se:** kvadriky (GLU) ani vestavěné funkce GLUT (glutSolid*)

### 2. Animace — 1 b
- Animace alespoň jednoho objektu

### 3. Osvětlení — 1 b
- Scéna osvětlena alespoň jedním zdrojem světla
- Předměty musí být stínovány
- Nutno zajistit **korektní normálování**

### 4. Volný pohyb v horizontální rovině — 1 b
- Pomocí **myši a klávesnice**
- Myší: rozhlížení
- Šipkami nebo klávesami ASDW: pohyb dopředu, dozadu, úkroky do stran

### 5. Menu — 2 b
- Alespoň **5 položek**, například:
    - reset polohy (kamera na výchozí souřadnice)
    - ovládání animace (zastavení, spuštění)
    - zapnutí/vypnutí textur
    - ovládání světla (barva, jas)
    - ukončení programu

### 6. Výpis textu — 2 b
- Vypisovat do scény **poslední vykonávaný příkaz** uživatelského rozhraní (rotace, posun, pauza…)
- Použít funkci `glutBitmapCharacter` s nastaveným fontem
- Text nesmí podléhat transformacím ani osvětlení
- Doporučený postup:
    1. nastavit matici PROJECTION na ortografické 2D mapování
    2. vykreslit text na příslušné souřadnice
    3. nastavit zpět perspektivní projekci a vykreslit scénu
- Při vykreslování textu vypnout `GL_TEXTURE_2D` a `GL_LIGHTING`

### 7. Ruční svítilna — 2 b
- Na stisk tlačítka **R** se rozsvítí **reflektorové světlo** (spotlight)
- Úzký poloměr
- Střed v pozici kamery

### 8. Blender model — 2 b
- Vymodelovat objekt v Blenderu
- Exportovat souřadnice vertexů a normál, vložit do projektu — **1 b**
- Aplikovat na objekt texturu — **1 b**

### 9. Létání — 2 b
- Rozšíření pohybu: pohyb myší nahoru/dolů způsobí naklonění „horizontální" roviny
- Naklonění platí jak pro pohled, tak pro pohyb (pohyb probíhá po nakloněné rovině)

### 10. Stoupání, klesání — 1 b
- Na stisk **Page Up / Page Down** horizontální rovina pro pohyb kamery stoupá nebo klesá
- Pohyb musí mít **smysluplné meze** (nelze dostat se pod „zem" apod.)

### 11. Hod předmětu — 2 b
- Na stisk tlačítka z pozice kamery vyletí libovolný předmět

### 12. Simulace kroků — 2 b
- Při pohybu v rovině přidat „nadskakování" kamery ve směru ke stropu — **1 b**
- Při zastavení horizontálního pohybu musí být krok dokončen (uživatel nesmí mít pocit, že zůstal viset nad povrchem) — **1 b**

### 13. Tlačítka — 2 b
- Část obrazovky bude obsahovat oblasti, které provedou akci při kliknutí myší

### 14. Průhlednost — 1 b
- K jednomu objektu přidat průhlednost

### 15. Neprůchozí objekt — 2 b
- Některý z objektů bude neprůchozí ze všech stran

### 16. Texturování — 2 b
- Alespoň jedna textura **externě načítaná** (BMP/TGA) — **1 b**
- Alespoň jedna textura **generovaná kódem** — **1 b**
- Textury musí být **odlišné** od těch prezentovaných na cvičení

### 17. Bézierovy pláty — 2 b
- Smysluplné zapojení Bézierových plátů do scény
- (Příklady smysluplného použití: most přes řeku, tvar kopce, střecha budovy — ne jen plocha položená ve scéně)

### 18. Vlastní rozšíření — konzultovat s cvičícím
- Příklad: ukazatel zbývající energie ubývající s každým pohybem

---

## Zvláštní projekty — až 30 bodů

Alternativa k základnímu projektu. Neřídí se seznamem úkolů výše, hodnotí se kvalita provedení.
Nutná textová dokumentace s postupem a popisem ovládání (postup musí být opakovatelný).
**Vždy konzultovat volbu s cvičícím.**

1. **3D tetris** — jednoduchá verze klasické hry ve 3D
2. **Bludiště** — procházení z pohledu první osoby
3. **Sluneční soustava** — Slunce, planety, Měsíc; textury; eliptické dráhy; rotace; ovládání rychlosti; sledování Země; přepínání měřítka
4. **Slunce a Země** — demonstrace pohybu Slunce po obloze v různých částech světa (vstup: místo, datum, čas)

---

## Estetika a smysluplnost

Hodnotitel má právo ocenit **dodatečnými body** pěkné a originální projekty nad rámec splněných bodů.
Nepřistupovat k úkolům formalisticky — scéna má dávat smysl jako celek.

---

## Co odevzdat

### Soubory
- Hlavní `.cpp` soubor
- Maximálně jeden `.h` soubor (mimo externě stažených)
- Případné externí soubory (textury, Blender model)
- **Neodevzdávat:** celé `.sln`, sestavené binárky ani celou složku projektu

### Hlavička `.cpp` souboru (komentář na začátku)
- jméno autora, studentské číslo
- vlastní název projektu
- seznam vypracovaných úloh včetně očekávaného počtu bodů
- seznam ovládacích kláves (pokud jsou specifika)
- komentář k případným vlastním nápadům
- konfigurace, pro kterou je projekt vyladěný

### Video
- Krátké demonstrační video nahrané na **YouTube** (nebo jinou platformu s okamžitým zhlédnutím)
- Demonstruje funkčnost projektu

### Archiv
- Vše zabalit do `.zip` / `.rar` / `.7z`
- Součástí archivu je textový soubor s linkem na video

---

## Deadline

- Odevzdání přes Moodle — položka **"Odevzdání projektu z OpenGL"**
- Nejpozději **2 pracovní dny před zkouškou**
- Doporučeno: **týden před zkouškou** (pro případ vrácení k dopracování)
- Minimum pro zápočet: **alespoň 12 bodů** z projektu (+ splněná minimální účast na cvičeních)