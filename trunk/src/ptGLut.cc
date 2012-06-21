/**
 *
 *    OpenGL Projected Tetrahedra PT Utilities
 *
 * Maximo, Andre -- Mar, 2008
 * Marroquim, Ricardo -- April, 2009
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

extern int isoWinId; ///< ISO Window id

static vcg::Trackball modelTrack; ///< Model Trackball

static vcg::Trackball lightTrack; ///< Light Trackball

static int buttonPressed = -1; ///< button state

static sortType currSort = gpu_quick; ///< Current sorting method

static drawType currDraw = dvr; ///< Current rendering method

static bool alwaysRotating = false; ///< Always rotating state
static int arX, arY; ///< Always rotating trackball X and Y

static bool whiteBG = true; ///< Back ground color

static bool drawVolume = true; ///< Draw volume

static bool useBO = true; ///< Use buffer object flag

static GLdouble drawTime = 0.0, sortTime = 0.0,
	totalTime = 0.0; ///< Time spent in drawing

static bool showHelp = false; ///< show help flag
static bool showInfo = true; ///< show information flag

/// ----------------------------------   Functions   -------------------------------------

void glWriteBig(GLdouble x, GLdouble y, const char *str) {

	glRasterPos2d(x, y);
	for (char *s = (char*)str; *s; s++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s);

}

/// glPT Show Information boxes

void glPTShowInfo(void) {

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.2, 1.2, -1.2, 1.2, -1.2, 1.2);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_LIGHTING);

	if( whiteBG ) glColor3f(BLACK);
	else glColor3f(WHITE);

	if (showInfo) { /// Show timing and dataset informations

		char str[256];

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

		sprintf(str, "Sort method: %s",
			(currSort == none) ? "None" :
			( (currSort == stl_sort) ? "STL Sort" :	
			  ( (currSort == gpu_bitonic) ? "GPU Bitonic" :
				( (currSort == gpu_quick) ? "GPU Quicksort" : "MPVO") ) ) );

		glWrite(-1.1, -0.8, str);

		sprintf(str, "Draw method: %s",
			(useBO) ? "Buffer Objects" : "Sending from CPU" );
		glWrite(-1.1, -0.9, str);

		sprintf(str, "Render method: %s",
				(currDraw == dvr) ? "DVR" :
				( (currDraw == isos) ? "Iso-surfaces" : "DVR + Iso-surfaces" ) );
		glWrite(-1.1, -1.0, str);

		if (!showHelp)
			glWrite(0.82, 1.1, "(?) open help");

	}

	if (showHelp) { /// Show help information

		glWrite( 0.82,  1.1, "(?) close help");
		glWrite(-0.52,  0.9, "(left-button) rotate volume");
		glWrite(-0.52,  0.8, "(wheel-button) zoom volume");
		glWrite(-0.52,  0.7, "(right-button) rotate light");
		glWrite(-0.52,  0.6, "(b) change background W/B");
		glWrite(-0.52,  0.5, "(o) turn on/off buffer object usage");
		glWrite(-0.52,  0.4, "(r) always rotating mode");
		glWrite(-0.52,  0.3, "(s) show/hide timing information");
		glWrite(-0.52,  0.2, "(t) open transfer function window");
		glWrite(-0.52,  0.1, "(i) open iso-surfaces window");
		glWrite(-0.52,  0.0, "(d) draw volume on/off");
		glWrite(-0.52, -0.1, "(0) no sort");
		glWrite(-0.52, -0.2, "(1) use stl sort");
		glWrite(-0.52, -0.3, "(2) use gpu bitonic sort");
		glWrite(-0.52, -0.4, "(3) use gpu quick sort");
		glWrite(-0.52, -0.5, "(4) use MPVO sort");
		glWrite(-0.52, -0.6, "(7) draw DVR");
		glWrite(-0.52, -0.7, "(8) draw ISO");
		glWrite(-0.52, -0.8, "(9) draw DVR+ISO");
		glWrite(-0.52, -0.9, "(q|esc) close application");

	}

	glEnable(GL_LIGHTING);

}

/// glPT Display

void glPTDisplay(void) {

	static struct timeval starttime, endtime;
	static GLdouble st = 0.0, dt = 0.0;

	glClear(GL_COLOR_BUFFER_BIT);

	if( showHelp ) {

		glPTShowInfo();
		glFinish();
		glutSwapBuffers();

		return;

	}

	/** Set light direction **/
	glPushMatrix();
	lightTrack.GetView();
	lightTrack.Apply();
	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	glShadeModel(GL_SMOOTH);
	static float lightPosF[]={0.0, 0.0, 1.0, 0.0};
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosF);
	glPopMatrix();
	/** ******************** **/

	/// Reset transformations
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	modelTrack.GetView();
	modelTrack.Apply();

	app.sort(st, currSort);

	if( drawVolume )
	  app.draw(dt);

	glPTShowInfo();

	gettimeofday(&starttime, 0);

	glFinish();
	glutSwapBuffers();

	gettimeofday(&endtime, 0);
	drawTime = dt + (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec)/1000000.0;

	sortTime = st;

	totalTime = drawTime + sortTime;
	
}

/// glPT Reshape

void glPTReshape(int w, int h) {

	glViewport(0, 0, winWidth=w, winHeight=h);

}

/// glPT animate function

void glPTAnimate( int value ) {

	if (alwaysRotating) {

	  modelTrack.MouseMove(arX += winWidth/32, arY += winHeight/32);

		if( arX >= winWidth || arY >= winHeight ) {
			modelTrack.MouseUp(arX, arY, vcg::Trackball::BUTTON_LEFT);
			modelTrack.MouseDown(arX = 0, arY = 0, vcg::Trackball::BUTTON_LEFT);
		}

		if( glutGetWindow() == tfWinId ){
		  glutSetWindow( ptWinId );
		  glutPostRedisplay();
		  glutSetWindow( tfWinId );
		} else if( glutGetWindow() == isoWinId ){
		  glutSetWindow( ptWinId );
		  glutPostRedisplay();
		  glutSetWindow( isoWinId );
		} else if( glutGetWindow() != 0 )
		  glutPostRedisplay();


		glutTimerFunc(32, glPTAnimate, 0);

	}

}

/// glPT Keyboard

void glPTKeyboard( unsigned char key, int x, int y ) {

	switch(key) {
	case '0': // none
	    currSort = none;
	    break;
	case '1': // stl_sort
		currSort = stl_sort;
		break;
	case '2': // gpu_bitonic
		currSort = gpu_bitonic;
		break;
	case '3': // gpu_quick
	   currSort = gpu_quick;
		break;
	case '4': // MPVO
	    currSort = mpvo;
		break;
	case '7': // Direct Volume Rendering
	  currDraw = dvr;
	  app.switchShaders(currDraw);
	  break;
	case '8': // Iso-Surfaces
	  currDraw = isos;
	  app.switchShaders(currDraw);
	  break;
	case '9': // Direct Volume Rendering + Iso-Surfaces
	  currDraw = dvr_isos;
	  app.switchShaders(currDraw);
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
			modelTrack.MouseDown(arX = 0, arY = 0, vcg::Trackball::BUTTON_LEFT);
		} else modelTrack.MouseUp(arX, arY, vcg::Trackball::BUTTON_LEFT);
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
	case 'i': case 'I': // open iso window
		glutSetWindow( isoWinId );
		glutShowWindow();
		glutPostRedisplay();
		glutSetWindow( ptWinId );
		break;
	case 'q': case 'Q': case 27: // quit application
		glutDestroyWindow( ptWinId );
		glutDestroyWindow( isoWinId );
		glutDestroyWindow( tfWinId );
		return;
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

		

		} else if (button == GLUT_RIGHT_BUTTON) {

			lightTrack.MouseDown(x, y, vcg::Trackball::BUTTON_LEFT);

		}

	} else if (state == GLUT_UP) {

		if (button == GLUT_LEFT_BUTTON) {

			modelTrack.MouseUp(x, y, vcg::Trackball::BUTTON_LEFT);

		}

		if (button == GLUT_RIGHT_BUTTON) {

			lightTrack.MouseUp(x, y, vcg::Trackball::BUTTON_LEFT);

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

	else if( buttonPressed == GLUT_RIGHT_BUTTON ) {

		lightTrack.MouseMove(x, y);
		glutPostRedisplay();

	}

}

/// Initialize light
void glPTInitLight(void) {

  glEnable (GL_LIGHTING);
  glEnable (GL_LIGHT0);
  glDisable (GL_COLOR_MATERIAL);

  vec3 ambient_light ( 0.1, 0.1, 0.1 );
  vec3 diffuse_light (1.0, 1.0, 1.0 );
  vec3 specular_light ( 1.0, 1.0, 1.0 );
  vec3 light_position ( 1.0, -1.0, -1.0 );

  GLfloat al[] = {ambient_light[0], ambient_light[1],
		  ambient_light[2], 1.0};

  GLfloat dl[] = {diffuse_light[0], diffuse_light[1],
		  diffuse_light[2], 1.0};

  GLfloat sl[] = {specular_light[0], specular_light[1],
		  specular_light[2], 1.0};

  GLfloat pos[] = {light_position[0], light_position[1],
		  light_position[2], 0.0};

  glLightfv(GL_LIGHT0, GL_AMBIENT, al);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, dl);
  glLightfv(GL_LIGHT0, GL_SPECULAR, sl);
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 1);

  glShadeModel(GL_SMOOTH);
}


/// glPT Application Setup

void glPTSetup(void) {

	modelTrack.center = vcg::Point3f(0, 0, 0);
	modelTrack.radius = 1;

	lightTrack.center = vcg::Point3f(0, 0, 0);
	lightTrack.radius = 1;
	
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
	//glutIdleFunc(glPTIdle);

	/// Command Menu
// 	glutCreateMenu(glPTCommand);
// 	glutAddMenuEntry("[h] Show/close help", 'h');
// 	glutAddMenuEntry("[r] Rotate always", 'r');
// 	glutAddMenuEntry("[s] Show/close timing information", 's');
// 	glutAddMenuEntry("[t] Open TF window", 't');
// 	glutAddMenuEntry("[i] Open ISO window", 'i');
// 	glutAddMenuEntry("[q] Quit", 'q');
// 	glutAttachMenu(GLUT_RIGHT_BUTTON);

	if (alwaysRotating) {
		glutTimerFunc(32, glPTAnimate, 0);
		modelTrack.MouseDown(arX = 0, arY = 0, vcg::Trackball::BUTTON_LEFT);
	}

	glPTInitLight();

}
