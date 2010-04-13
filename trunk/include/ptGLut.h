/**
 *
 *    OpenGL Projected Tetrahedra PT Utilities
 *
 * Maximo, Andre -- Mar, 2008
 *
 **/

/**
 *   ptGLut functions : handle the main window for project tetrahedra volume rendering.
 *
 * C++ header.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#ifndef _PTGLUT_H_
#define _PTGLUT_H_

#include "haptVol.h"

haptVol app; ///< PT Volume application

extern
void glWrite(GLdouble x, GLdouble y, const char *str);

/// ----------------------------------   Prototypes   -------------------------------------

/// glPT Show Information boxes
void glPTShowInfo(void);

/// glPT Display
void glPTDisplay(void);

/// glPT Reshape
/// @arg w, h new window size
void glPTReshape(int w, int h);

/// glPT Keyboard
/// @arg key keyboard key hitted
/// @arg x, y mouse position when hit
void glPTKeyboard( unsigned char key, int x, int y );

/// glPT Command
/// @arg value command menu hit
void glPTCommand(int value);

/// glPT Mouse
/// @arg button mouse button event
/// @arg state mouse state event
/// @arg x, y mouse position
void glPTMouse(int button, int state, int x, int y);

/// glPT Motion
/// @arg x, y mouse position
void glPTMotion(int x, int y);

/// glPT animate function
void glPTAnimate( int value );

/// glPT Application Setup
extern
void glPTSetup(void);

/// Compare the difference between two given buffers of same size
/// @param a First given buffer
/// @param a Second given buffer
/// @param w Buffers width
/// @param h Buffers height
/// @param max Outuputs maximum error
/// @param avg Outputs average error
/// @param pixels Outputs percentage of pixels with error, excluding background
void compareBuffers( GLfloat* a, GLfloat* b, GLuint w, GLuint h, GLfloat &max, GLfloat &avg, GLfloat &pixels );

/// Reads back the current buffer and writes to given array
/// @param buf Given array to write output
/// @param w Buffer width
/// @param h Buffer height
void captureBuffer( GLfloat* buf, GLuint w, GLuint  h);

/// Initialize light
void glPTInitLight(void);

#endif
