/**
 *
 *    OpenGL Projected Tetrahedra PT Utilities
 *
 * Maximo, Andre -- Apr, 2009
 *
 **/

/**
 *   ptGLut functions : handle the main window for project tetrahedra volume rendering.
 *
 * C++ code.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#include "ptGLut.h"

extern "C" {
#include <GL/glut.h> // gl-utility library
}

#include <wrap/gui/trackball.h>

using std::cerr;
using std::cout;
using std::endl;
using std::flush;

/// Global Variables

static const char titleWin[] = "HAPT"; ///< Window title

static int winWidth = 1280, winHeight = 1280; ///< Window size

int ptWinId; ///< PT Window id

extern int tfWinId; ///< TF Window id

static vcg::Trackball modelTrack; ///< Model Trackball

static int buttonPressed = -1; ///< button state

static bool alwaysRotating = false; ///< Always rotating state
//static int arX, arY; ///< Always rotating trackball X and Y
static GLfloat rotX = 0.0, rotY = 0.0, rotZ = 0.0, drX = 0.0, drY = 0.0, drZ = 0.0;

static bool whiteBG = false; ///< Back ground color

static bool drawVolume = true; ///< Draw volume

static bool useBO = true; ///< Use buffer object flag

static bool play = false; ///< Play/pause toggle

static GLuint currFrame = 0; ///< Current frame animation

static GLdouble drawTime = 0.0, sortTime = 0.0,
	totalTime = 0.0; ///< Time spent in drawing

static bool showHelp = false; ///< show help flag
static bool showInfo = false; ///< show information flag

/// ----------------------------------   Functions   -------------------------------------

void glWriteBig(GLdouble x, GLdouble y, const char *str) {

	glRasterPos2d(x, y);
	for (char *s = (char*)str; *s; s++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s);

}

/// glPT Show Information boxes

void glPTShowInfo(void) {

	static char str[256];

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.2, 1.2, -1.2, 1.2, -1.2, 1.2);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glBegin(GL_QUADS);
	glColor4ub(91, 229, 144, 200);
	float x = ( currFrame / (float)app.volume.numFrames ) * 1.1 - 1.1;
	glVertex2f(-1.1, 0.96); glVertex2f( x, 0.96);
	glVertex2f( x, 1.00); glVertex2f(-1.1, 1.00);
	glEnd();
	glPolygonMode(GL_FRONT, GL_LINE);
	glBegin(GL_QUADS);
	glColor3f(0.6, 0.6, 0.6);
	glVertex2f(-1.1, 0.96); glVertex2f( 0.0, 0.96);
	glVertex2f( 0.0, 1.00); glVertex2f(-1.1, 1.00);
	glEnd();
	glPolygonMode(GL_FRONT, GL_FILL);

	if( whiteBG ) glColor3f(BLACK);
	else glColor3f(WHITE);

	if (showInfo) { /// Show timing and dataset informations

		sprintf(str, "HAPT -- %s", (char*)app.volName.c_str() );
		glWrite(-0.25, 1.1, str);

		// sprintf(str, "%d / %d", currFrame+1, app.volume.numFrames );
		// glWrite(-1.1, 0.95, str);

		sprintf(str, "Sort: %.5lf s ( %.1lf %% )", sortTime, 100*sortTime / totalTime );
		glWrite(-1.1, 0.8, str);

		sprintf(str, "Draw Step: %.5lf s ( %.1lf %% )", drawTime, 100*drawTime / totalTime );
		glWrite(-1.1, 0.7, str);

		sprintf(str, "Total Step: %.5lf s", totalTime );
		glWrite(-1.1, 0.6, str);

		sprintf(str, "# Tets / sec: %.2lf MTet/s ( %.1lf fps )", (app.volume.numTets / totalTime) / 1000000.0, 1.0 / totalTime );
		glWrite(-1.1, 0.5, str);

		sprintf(str, "# Tets: %d", app.volume.numTets );
		glWrite(-1.1, -0.5, str);

		sprintf(str, "# Verts: %d", app.volume.numVerts );
		glWrite(-1.1, -0.6, str);

		sprintf(str, "Resolution: %d x %d", winWidth, winHeight );
		glWrite(-1.1, -0.7, str);

		sprintf(str, "Draw method: %s",
			(useBO) ? "Buffer Objects" : "Sending from CPU" );
		glWrite(-1.1, -0.9, str);

		if (!showHelp)
			glWrite(0.82, 1.1, "(?) open help");

	} else {

		sprintf(str, "%.1lf FPS", 1.0 / totalTime );
		glWriteBig(0.75, -1.0, str);

	}

	if (showHelp) { /// Show help information

		glWrite( 0.82,  1.1, "(?) close help");
		glWrite(-0.52,  0.7, "(left-button) rotate volume");
		glWrite(-0.52,  0.6, "(wheel-button) zoom volume");
		glWrite(-0.52,  0.5, "(right-button) open menu");
		glWrite(-0.52,  0.4, "(b) change background W/B");
		glWrite(-0.52,  0.3, "(o) turn on/off buffer object usage");
		glWrite(-0.52,  0.2, "(r) always rotating mode");
		glWrite(-0.52,  0.1, "(s) show/hide timing information");
		glWrite(-0.52,  0.0, "(t) open transfer function window");
		glWrite(-0.52, -0.1, "(d) draw volume on/off");
		glWrite(-0.52, -0.2, "(+/-) in(de)crement current animation frame");
		glWrite(-0.52, -0.3, "(p) play/pause animation");
		glWrite(-0.52, -0.6, "(q|esc) close application");

	}

}

/// glPT Display

void glPTDisplay(void) {

	static struct timeval starttime, endtime, sorttime1, sorttime2;

	glClear(GL_COLOR_BUFFER_BIT);

	glPTShowInfo();

	if( showHelp ) {

		glFinish();
		glutSwapBuffers();

		return;

	}

	//totalTime = clock();
	gettimeofday(&starttime, 0);

	/// Reset transformations
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(rotX, 1.0, 0.0, 0.0); glRotatef(rotY, 0.0, 1.0, 0.0); glRotatef(rotZ, 0.0, 0.0, 1.0);

	modelTrack.GetView();
	modelTrack.Apply();

	gettimeofday(&sorttime1, 0);

	app.sort();

	gettimeofday(&sorttime2, 0);
	sortTime = (sorttime2.tv_sec - sorttime1.tv_sec) + (sorttime2.tv_usec - sorttime1.tv_usec)/1000000.0;

	if( drawVolume )
		app.draw();

	glFinish();
	glutSwapBuffers();

	gettimeofday(&endtime, 0);
	totalTime = (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec)/1000000.0;

	drawTime = totalTime - sortTime;

}

/// glPT Reshape

void glPTReshape(int w, int h) {

	glViewport(0, 0, winWidth=w, winHeight=h);

}

/// glPT animate function

void glPTAnimate( int value ) {

	if (alwaysRotating) {

		/*
		modelTrack.MouseMove(arX += winWidth/32, arY += winHeight/32);

		if( arX >= winWidth || arY >= winHeight ) {
			modelTrack.MouseUp(arX, arY, vcg::Trackball::BUTTON_LEFT);
			modelTrack.MouseDown(arX = 0, arY = 0, vcg::Trackball::BUTTON_LEFT);
		}
		*/

		rotX += drX;
		if( rotX > 360.0 ) rotX -= 360.0;
		if( rotX < 0.0 ) rotX += 360.0;
		rotY += drY;
		if( rotY > 360.0 ) rotY -= 360.0;
		if( rotY < 0.0 ) rotY += 360.0;
		rotZ += drZ;
		if( rotZ > 360.0 ) rotZ -= 360.0;
		if( rotZ < 0.0 ) rotZ += 360.0;

		if( glutGetWindow() == tfWinId ) {
			glutSetWindow( ptWinId );
			glutPostRedisplay();
			glutSetWindow( tfWinId );
		} else if( glutGetWindow() != 0 )
			glutPostRedisplay();

		glutTimerFunc(32, glPTAnimate, 0);

	}

}

/// glPT play animation

void glPTPlay( int value ) {

	if( play ) {

		if( currFrame == app.volume.numFrames-1 )
			currFrame = 0;
		else
			++currFrame;

		app.changeFrame( currFrame );

		glutTimerFunc(100, glPTPlay, 0);

	}

}

/// glPT Keyboard

void glPTKeyboard( unsigned char key, int x, int y ) {

	switch(key) {
	case '+':
		if( currFrame == app.volume.numFrames-1 ) return;
		++currFrame;
		app.changeFrame( currFrame );
		break;
	case '-':
		if( currFrame == 0 ) return;
		--currFrame;
		app.changeFrame( currFrame );
		break;
	case 'p': case 'P':
		play = !play;
		if( play ) glutTimerFunc(100, glPTPlay, 0);
		break;
	case 'd': case 'D':
		drawVolume = !drawVolume;
		break;
	case 'h': case 'H': case '?': // show help
		showHelp = !showHelp;
		if (showHelp) showInfo = false;
		else showInfo = true;
		if( alwaysRotating ) alwaysRotating = false;
		break;
	case 'b': case 'B': // change background
		whiteBG = !whiteBG;
		if (whiteBG) app.setColor(WHITE);
		else app.setColor(BLACK);
		break;
	case 'o': case 'O': // change buffer object usage flag
		useBO = !useBO;
		app.useBufferObject( useBO );
		return;
	case 'r': case 'R': // always rotating flag
		alwaysRotating = !alwaysRotating;
		if (alwaysRotating) {
			glutTimerFunc(32, glPTAnimate, 0);
			//modelTrack.MouseDown(arX = 0, arY = 0, vcg::Trackball::BUTTON_LEFT);
		} else {
			//modelTrack.MouseUp(arX, arY, vcg::Trackball::BUTTON_LEFT);
		}
		return;
	case 's': case 'S': // show/close information
		showInfo = !showInfo;
		break;
	case 't': case 'T': // open tf window
		glutSetWindow( tfWinId );
		glutShowWindow();
		glutPostRedisplay();
		glutSetWindow( ptWinId );
		break;
	case 'q': case 'Q': case 27: // quit application
		glutDestroyWindow( ptWinId );
		glutDestroyWindow( tfWinId );		
		return;
	case 'x': drX =  1.0; break;
	case 'X': drX = -1.0; break;
	case 'y': drY =  1.0; break;
	case 'Y': drY = -1.0; break;
	case 'z': drZ =  1.0; break;
	case 'Z': drZ = -1.0; break;
	default: // any other key
		cerr << "No key bind for " << key
		     << " in (" << x << ", " << y << ")" << endl;
		return;
	}

	glutPostRedisplay();

}

/// glPT Command

void glPTCommand(int value) {

	glPTKeyboard( (unsigned char)value, 0, 0 );

}

/// glPT Mouse

void glPTMouse(int button, int state, int x, int y) {

	if( alwaysRotating ) return;

	buttonPressed = button;

	y = winHeight - y;

	if (state == GLUT_DOWN) {

		if (button == 3) { // wheel up

			modelTrack.MouseWheel(0.8);

			glutPostRedisplay();

		} else if (button == 4) { // wheel down

			modelTrack.MouseWheel(-0.8);

			glutPostRedisplay();

		} else if (button == GLUT_LEFT_BUTTON) {

			modelTrack.MouseDown(x, y, vcg::Trackball::BUTTON_LEFT);

		}

	} else if (state == GLUT_UP) {

		if (button == GLUT_LEFT_BUTTON) {

			modelTrack.MouseUp(x, y, vcg::Trackball::BUTTON_LEFT);

		}

	}

}

/// glPT Motion

void glPTMotion(int x, int y) {

	if( alwaysRotating ) return;

	y = winHeight - y;

	if( buttonPressed == GLUT_LEFT_BUTTON ) {

		modelTrack.MouseMove(x, y);
		glutPostRedisplay();

	}

}

/// glPT Idle

void glPTIdle( void ) {

	if( play ) {

		glutSetWindow( ptWinId );
		glutPostRedisplay();

	}

}

/// glPT Application Setup

void glPTSetup(void) {

	modelTrack.center = vcg::Point3f(0, 0, 0);
	modelTrack.radius = 1;

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(winWidth, winHeight);
	glutInitWindowPosition(0, 0);
	ptWinId = glutCreateWindow(titleWin);

	/// Event handles
	glutReshapeFunc(glPTReshape);
	glutDisplayFunc(glPTDisplay);
	glutKeyboardFunc(glPTKeyboard);
	glutMouseFunc(glPTMouse);
	glutMotionFunc(glPTMotion);
	glutIdleFunc(glPTIdle);

	/// Command Menu
	glutCreateMenu(glPTCommand);
	glutAddMenuEntry("[h] Show/close help", 'h');
	glutAddMenuEntry("[r] Rotate always", 'r');
	glutAddMenuEntry("[s] Show/close timing information", 's');
	glutAddMenuEntry("[t] Open TF window", 't');
	glutAddMenuEntry("[q] Quit", 'q');
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	if (alwaysRotating) {
		glutTimerFunc(32, glPTAnimate, 0);
		//modelTrack.MouseDown(arX = 0, arY = 0, vcg::Trackball::BUTTON_LEFT);
	}

	if (whiteBG) app.setColor(WHITE);
	else app.setColor(BLACK);

}
