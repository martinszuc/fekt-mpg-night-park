/*
*	MPC-MPG 2023 - Zaverecny projekt
*
*	Autor:		S. Studujici
*	Student ID:	12345
*	Email:		xstude00@vut.cz
*
*	Seznam vypracovanych ukolu:
*		
*   Neco vlastniho co by mohlo byt take hodnoceno:
*
*	Ocekavany pocet bodu:	fakt hodne, ale aspon 12
*
*/
#include "imageLoad.h"
#include <cmath>
#include <GL/glut.h>

// konstanty a makra pridejte zde
// PI/180
#define PI        3.141592653589793f
#define PIover180 0.017453292519943f

// globalni promenne pro transformace
float xnew = 0, ynew = 0;
float xold = 0, yold = 0;
float xx = 0, yy = 0;
bool tocime = false;
float uhel = 0;
float tranz = -100.0f, tranx = 0;
float radius = 150;

// nastaveni projekce
float fov = 60.0;
float nearPlane = 0.1;
float farPlane = 150.0;

// zde vlastnosti svetel materialu, menu, popripade jinych vlastnosti
// ...

// funkce volana pri zmene velikosti sceny
void OnReshape(int w, int h)            // event handler pro zmenu velikosti okna
{
	glViewport(0, 0, w, h);             // OpenGL: nastaveni rozmenu viewportu
	glMatrixMode(GL_PROJECTION);        // OpenGL: matice bude typu projekce
	glLoadIdentity();                   // OpenGL: matice bude identicka (jen jednicky v hlavni diagonale)
	gluPerspective(fov, (double)w/h, nearPlane, farPlane); // perspektivni projekce
}

// funkce pro inicializaci
// do teto funkce vlozit vsechny vlastnosti a inicializace, ktere se nebudou menit pri behu aplikace
void OnInit(void)
{
	// steny
	glFrontFace(GL_CW);					// nastaveni front == clockwise
	glPolygonMode(GL_FRONT, GL_FILL);   // mod predni steny fill
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	// dalsi stavy a vlastnosti (napr. nastaveni svetel, materialu,...)
}

// doporucene vykresleni objektu 
// (1) pouzijte PUSH/POP pro GL_MODELVIEW matice, abyste si nerozhodili transformace cele sceny
// (2) vykreslujte vlastni objekty okolo pocatku (0,0,0) pro snazsi provadeni transformaci a umisteni ve scene
// (3) pro ziskaji aktulanich stavovych promennych v OpenGL muzete pouzit funkce glGetBooleanv(...)
void DrawPlane(int size)
{
	// (1)
	glPushMatrix();
	
	// nastaveni transformace (pokud je treba)
	glTranslatef(0, -15, 0);
	
	// (3) nastaveni vlastnosti OpenGL
	GLboolean isLightingEnabled;
	glGetBooleanv(GL_LIGHTING, &isLightingEnabled);

	if(isLightingEnabled) {
		glDisable(GL_LIGHTING);
	}

	glColor3f(0.0 ,0.7 ,0.0);

	// (2) vykresleni objektu
	glBegin( GL_QUADS);
	glVertex3i(-size, 0, size);
	glVertex3i(-size, 0, -size);
	glVertex3i(size , 0, -size);
	glVertex3i(size , 0, size);
	glEnd();

	// v pripade, ze jste vypnuli/zapnuli nejaky stav, vratte ho do puvodniho
	if(isLightingEnabled) {
		glEnable( GL_LIGHTING);
	}

	glPopMatrix();
}

// funkce volana pri prekresleni sceny
void OnDisplay(void)				   
{
    // nastaveni parametru pro defaultni mazani rovin
    glClearColor(0.8, 0.8, 0.8, 0.0);  
    glClearDepth(1.0f);

    // mazani rovin
    glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);      
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);   

    // inicializace matice MODELVIEW
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	// provedeni transformaci sceny (pro rotaci mysi a pohyb klavesami)
	glRotatef(-180 * ynew / PI, 1.0f, 0.0f, 0.0f);	// rotace podle osy x
	glRotatef(180 * xnew / PI, 0.0f, 1.0f, 0.0f);	// rotace podle osy y
	glTranslatef(tranx, 0.0f, tranz);				// relativni zmena v z

	// souradnicove osy
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
	glColor3f(1, 0, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(50, 0, 0);
	glColor3f(0, 1, 0);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 50, 0);
	glColor3f(0, 0, 1);
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 50);
	glEnd();
	glEnable(GL_LIGHTING);
	
	// zde vykreslovani dalsich objektu...

	// (priklad) vykresli podlahu
	DrawPlane(100);

    glFlush();		
    glutSwapBuffers(); 
}

// funkce volana pri stisku tlacitka
void OnMouseButton(int button, int state, int x, int y)
{ 
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;

	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			tocime = true;
			xx = x;
			yy = y;
		}
		else {
			tocime = false;
			xold = xnew;
			yold = ynew;
		}
	}

	// pripadne test pro prave tlacitko pro vyvolani menu
	// ... GLUT_RIGHT_BUTTON

	glutPostRedisplay();
}

// funkce volana pri pohybu mysi a stistenem tlacitku mysi
void OnMouseMotion(int x, int y)
{ 
	y = glutGet(GLUT_WINDOW_HEIGHT) - y;

	if (tocime) {
		xnew = xold + (x - xx) / radius;
		ynew = yold + (y - yy) / radius;
		uhel = xnew * PIover180;
		glutPostRedisplay();
	}
}

// funkce volana pri stisku specialnich klaves
void OnSpecial(int key, int mx, int my)
{
	switch(key)
	{
		case GLUT_KEY_UP:
			{
				tranz += 2 * cos(uhel);
				tranx -= 2 * sin(uhel);
			}
			break;
		case GLUT_KEY_DOWN:
			{
				tranz -= 2 * cos(uhel);
				tranx += 2 * sin(uhel);
			}
			break;
		// case ... pripadne dalsi klavesy pokud jsou potreba GLUT_KEY_...
		default:
			break;
	}

	glutPostRedisplay();
}

// funkce main
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);					// inicializace knihovny GLUT
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH); // init double buffer

	glutInitWindowSize(640, 480);			// nastaveni pocatecni velikosti dale oteviranych oken
    glutInitWindowPosition(200, 200);		// nastaveni pocatecniho umisteni dale oteviranych oken
    
	glutCreateWindow("Projekt 2023 – Autor: xstude00");	// vytvoreni okna

	// registracni funkce
	glutDisplayFunc(OnDisplay);				// registrace funkce volane pri prekreslovani aktualniho okna
    glutReshapeFunc(OnReshape);				// registrace funkce volane pri zmene velikosti aktualniho okna
    glutMouseFunc(OnMouseButton);			// registrace funkce pro stisk tlacitka mysi
    glutMotionFunc(OnMouseMotion);			// registrace funkce pro pohyb mysi pri stisknutem tlacitku
	glutSpecialFunc(OnSpecial);				// registrace funkce pro zachytavani specialnich klaves

	// pripadne dalsi udalosti...

	// inicializace sceny
	OnInit();
	
	glutMainLoop();

    return 0;
}
