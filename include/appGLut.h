/**
 *
 *    OpenGL Application Utilities
 *
 *  Maximo, Andre -- Mar, 2008
 *
 **/

/**
 *   appGLut : wraps GLUT functions for general usage.
 *
 * C++ header and implementation.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

extern "C" {
#include <GL/glut.h> // gl-utility library
}

/// ----------------------------------   Functions   -------------------------------------

/// OpenGL Write
/// @arg x, y raster position
/// @arg str string to write
void glWrite(GLdouble x, GLdouble y, const char *str) {

	// You should call glColor* before glWrite;
	// And the font color is also affected by texture and lighting
	glRasterPos2d(x, y);
	for (char *s = (char*)str; *s; s++)
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);

}

/// OpenGL Application Initialization

void glAppInit(int& argc, char** argv) {

	glutInit(&argc, argv);

}

/// OpenGL Main Loop

void glLoop(void) {

	glutMainLoop();

}
