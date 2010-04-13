/**
 *
 *    OpenGL Transfer Function Utilities
 *
 * Maximo, Andre -- Mar, 2008
 *
 **/

/**
 *   tfGLut functions : handle an auxiliary window for transfer function editing.
 *
 * C++ code.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#include "tfGLut.h"

extern "C" {
#include <GL/glut.h> // gl-utility library
}

using std::cerr;
using std::cout;
using std::endl;
using std::flush;

/// Global Variables

static const char titleWin[] = "Transfer Function Editing"; ///< Window title

static int winWidth = 400, winHeight = 300; ///< Window size

extern int ptWinId; ///< PT Window id

int tfWinId; ///< Window id

static int buttonPressed = -1; ///< button state

static bool showHelp = false; ///< show help flag
static bool showTF = true; ///< show TF flag

/// ----------------------------------   Functions   -------------------------------------

/// glTF Show Information boxes

void glTFShowInfo(void) {

	glColor3f(BLACK);

	if (showHelp) {

		glWrite(1.00, 1.2, "(?) close help");
		glWrite(0.12, 1.0, "(left-button) select/move control point");
		glWrite(0.12, 0.9, "(middle-button) create control point");
		glWrite(0.12, 0.8, "(right-button) open menu");
		glWrite(0.12, 0.7, "(0-9) select color code");
		glWrite(0.12, 0.6, "(r) read TF file");
		glWrite(0.12, 0.5, "(w) write TF file");
		glWrite(0.12, 0.4, "(s) show/hide TF chart");
		glWrite(0.12, 0.3, "(x) delete selected control point");
		glWrite(0.12, 0.2, "(h|?) open/close this help");
		glWrite(0.12, 0.1, "(q|t|esc) close this window");

	} else {

		glWrite(1.0, 1.2, "(?) open help");

	}

}

/// glTF Display

void glTFDisplay(void) {

	glClear(GL_COLOR_BUFFER_BIT);

	if (showTF)
		tf->draw();

	glTFShowInfo();

	glutSwapBuffers();

}

/// glTF Reshape

void glTFReshape(int w, int h) {

	tf->setWindow(w, h);

	glViewport(0, 0, winWidth=w, winHeight=h);

}

/// glRefresh Transfer Function values

void glRefreshTF(void) {

	glutSetWindow( ptWinId );
	app.refreshTFandBrightness(tf->getBrightness());
	glutPostRedisplay();
	glutSetWindow( tfWinId );

}

/// glTF Keyboard

void glTFKeyboard( unsigned char key, int x, int y ) {

	switch(key) {
	case '0': case '1': case '2': case '3':
	case '4': case '5': case '6': case '7':
	case '8': case '9':
		tf->colorCode( ((int)key)-48 );
		glRefreshTF();
		break;
	case 'h': case 'H': case '?': // show help
		showHelp = !showHelp;
		if (showHelp) showTF = false;
		else showTF = true;
		break;
	case 'r': case 'R': // read TF
		if( app.volume.readTF(tf->getTFName().c_str()) )
			cout << "Transfer Function: " << tf->getTFName() << " readed!" << endl;
		if( tf->readCP() )
			cout << "Control points: " << tf->getCPName() << " readed!" << endl;
		break;
	case 'w': case 'W': // write TF
		if( app.volume.writeTF(tf->getTFName().c_str()) )
			cout << "Transfer Function: " << tf->getTFName() << " written!" << endl;
		if( tf->writeCP() )
			cout << "Control points: " << tf->getCPName() << " written!" << endl;
		break;
	case 's': case 'S': // show TF
		showTF = !showTF;
		break;
	case 'x': case 'X': // delete a control point
		if (buttonPressed == GLUT_LEFT_BUTTON)
			tf->deleteControlPoint();
		break;
	case 'q': case 'Q': case 27: case 't': case 'T': // hide window
		glutHideWindow();
		return;
	default: // any other key
		cerr << "No key bind for " << key
		     << " in (" << x << ", " << y << ")" << endl;
		return;
	}

	glutPostRedisplay();

}

/// glTF Command

void glTFCommand(int value) {

	glTFKeyboard( (unsigned char)value, 0, 0 );

}

/// glTF Mouse

void glTFMouse(int button, int state, int x, int y) {

	buttonPressed = button;

	if (!showTF) return;

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {

		tf->pick(x, y);
		glRefreshTF();
		glutPostRedisplay();

	}

	if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP)) {

		tf->releasePickedPoint(x, y);
		glRefreshTF();
		glutPostRedisplay();

	}

	if ((button == GLUT_MIDDLE_BUTTON) && (state == GLUT_DOWN)) {

		tf->createControlPoint(x, y);
		glutPostRedisplay();

	}

}

/// glTF Motion

void glTFMotion(int x, int y) {

	if (!showTF) return;

	if (buttonPressed == GLUT_LEFT_BUTTON) {

		tf->updatePickedPoint(x, y);
		glRefreshTF();
		glutPostRedisplay();

	}

}


/// glTF Application Setup

void glTFSetup(void) {

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(winWidth, winHeight);
	glutInitWindowPosition(520, 0);
	tfWinId = glutCreateWindow(titleWin);

	/// Event handles
	glutReshapeFunc(glTFReshape);
	glutDisplayFunc(glTFDisplay);
	glutKeyboardFunc(glTFKeyboard);
	glutMouseFunc(glTFMouse);
	glutMotionFunc(glTFMotion);

	/// Command Menu
	glutCreateMenu(glTFCommand);
	glutAddMenuEntry("[h] Open/close help", 'h');
	glutAddMenuEntry("[r] Read TF file", 'r');
	glutAddMenuEntry("[w] Write TF file", 'w');
	glutAddMenuEntry("[s] Show/hide TF chart", 's');
	glutAddMenuEntry("[x] Delete selected control point", 'x');
	glutAddMenuEntry("[t] Close TF window", 't');
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	/// ModelviewProjection setup (<-)
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	gluOrtho2D(-0.2, 1.3, -0.2, 1.3);
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();

	glutHideWindow();

	tf = new transferFunction< GLfloat, GLuint >(app.volume.tf);

	tf->glSetup();
	tf->setTFName( app.volName + app.tfExt );
	tf->setCPName( app.volName );

	tf->readCP(); /// first atempt to try to read control points from file

	tf->setWindow(winWidth, winHeight);
	tf->setOrtho(-0.2, 1.3);

}
