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

/// glPT animate function
void glPTAnimate( int value );

/// glPT play animation
void glPTPlay( int value );

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

/// glPT Idle
void glPTIdle( void );

/// glPT Application Setup
extern
void glPTSetup(void);

#endif
