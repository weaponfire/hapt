/**
 *
 *    OpenGL Iso-Surfaces Utilities
 *
 * Marroquim, Ricardo -- April, 2009
 *
 **/

/**
 *   isoGLut functions : handle an auxiliary window for iso-surfaces editing.
 *
 * C++ code.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#include "isoGLut.h"

extern "C" {
#include <GL/glut.h> // gl-utility library
}

using std::cerr;
using std::cout;
using std::endl;
using std::flush;

/// Global Variables

static const char titleWin[] = "Iso-Surfaces Editing"; ///< Window title

static int winWidth = 400, winHeight = 300; ///< Window size

extern int ptWinId; ///< PT Window id

int isoWinId; ///< Window id

static int buttonPressed = -1; ///< button state

static bool showHelp = false; ///< show help flag
static bool showISO = true; ///< show ISO flag

static bool animating = false; ///< Animating iso-surface
static int frame = 0; ///< Animation frame

static bool useLight = true; ///< Use illumination flag


/// ----------------------------------   Functions   -------------------------------------

/// glISO Show Information boxes

void glISOShowInfo(void) {

	glColor3f(BLACK);

	if (showHelp) {

		glWrite(1.00, 1.2, "(?) close help");
		glWrite(0.12, 1.0, "(left-button) select/move control point");
		glWrite(0.12, 0.8, "(right-button) open menu");
		glWrite(0.12, 0.7, "(0-9) select color code");
		glWrite(0.12, 0.6, "(r) read ISO file");
		glWrite(0.12, 0.5, "(w) write ISO file");
		glWrite(0.12, 0.4, "(s) show/hide ISO chart");
		glWrite(0.12, 0.2, "(h|?) open/close this help");
		glWrite(0.12, 0.1, "(q|t|esc) close this window");

	} else {

		glWrite(1.0, 1.2, "(?) open help");

	}

}


/// glRefresh Iso-Surfaces values

void glRefreshISO(void) {

	glutSetWindow( ptWinId );
	app.refreshISO( );
	glutPostRedisplay();
	glutSetWindow( isoWinId );
	glutPostRedisplay();
	
}

/// glISO Display

void glISODisplay(void) {

  static int max_frames = 100;

	glClear(GL_COLOR_BUFFER_BIT);

	if (showISO)
		iso->draw();

	// animation traverses Iso-Surface 0 through the scalars range
	if (animating) {
	  iso->animate(frame/(GLfloat)max_frames);
	  frame++;
	  if (frame == max_frames) {
		frame = 0;
		animating = false;
	  }
	  glRefreshISO();
	  glutPostRedisplay();
	}

	glISOShowInfo();

	glutSwapBuffers();

}

/// glISO Reshape

void glISOReshape(int w, int h) {

	iso->setWindow(w, h);

	glViewport(0, 0, winWidth=w, winHeight=h);

}


/// glISO Keyboard

void glISOKeyboard( unsigned char key, int x, int y ) {

	switch(key) {
	case 'h': case 'H': case '?': // show help
		showHelp = !showHelp;
		if (showHelp) showISO = false;
		else showISO = true;
		break;
	case 'r': case 'R': // read ISO
	  if( app.volume.readISO(iso->getISOName().c_str()) )
			cout << "Iso-surfaces: " << iso->getISOName() << " read!" << endl;
		break;
	case 'w': case 'W': // write ISO
		if( app.volume.writeISO(iso->getISOName().c_str()) )
			cout << "Iso-surfaces: " << iso->getISOName() << " written!" << endl;
		break;
	case 's': case 'S': // show ISO
		showISO = !showISO;
		break;
	case 'l': case 'L': // change illumination usage flag
		useLight = !useLight;
		app.useIllumination( useLight );
		break;
	case 'a': case 'A': // animate iso-surface
	  animating = !animating;
	  break;
	case 'q': case 'Q': case 27: case 'i': case 'I': // hide window
		glutHideWindow();
		return;
	default: // any other key
		cerr << "No key bind for " << key
		     << " in (" << x << ", " << y << ")" << endl;
		return;
	}

	glRefreshISO();
}

/// glISO Command

void glISOCommand(int value) {

	glISOKeyboard( (unsigned char)value, 0, 0 );

}

/// glISO Mouse

void glISOMouse(int button, int state, int x, int y) {

	buttonPressed = button;

	if (!showISO) return;

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {

		iso->pick(x, y);
		glRefreshISO();
		glutPostRedisplay();

	}

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP)) {

		iso->releasePickedPoint(x, y);
		glRefreshISO();
		glutPostRedisplay();

	}

}

/// glISO Motion

void glISOMotion(int x, int y) {

	if (!showISO) return;

	if (buttonPressed == GLUT_LEFT_BUTTON) {

		iso->updatePickedPoint(x, y);
		glRefreshISO();
		glutPostRedisplay();

	}

}


/// glISO Application Setup

void glISOSetup(void) {

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(winWidth, winHeight);
	glutInitWindowPosition(520, 300);
	isoWinId = glutCreateWindow(titleWin);	

	/// Event handles
	glutReshapeFunc(glISOReshape);
	glutDisplayFunc(glISODisplay);
	glutKeyboardFunc(glISOKeyboard);
	glutMouseFunc(glISOMouse);
	glutMotionFunc(glISOMotion);

	/// Command Menu
	glutCreateMenu(glISOCommand);
	glutAddMenuEntry("[h] Open/close help", 'h');
	glutAddMenuEntry("[r] Read ISO file", 'r');
	glutAddMenuEntry("[w] Write ISO file", 'w');
	glutAddMenuEntry("[s] Show/hide ISO chart", 's');
	glutAddMenuEntry("[t] Close ISO window", 't');
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/// ModelviewProjection setup (<-)
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	gluOrtho2D(-0.2, 1.3, -0.2, 1.3);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();

	glutHideWindow();

	iso = new isoSurfaces< GLfloat, GLuint >(app.volume.iso);

	iso->glSetup();
	iso->setISOName( app.volName + app.isoExt );

	iso->setWindow(winWidth, winHeight);
	iso->setOrtho(-0.2, 1.3);

}
