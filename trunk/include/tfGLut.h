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
 * C++ header.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#ifndef _TFGLUT_H_
#define _TFGLUT_H_

#include "haptVol.h"

#include "transferFunction.h"

extern haptVol app;

transferFunction< GLfloat, GLuint > *tf;

extern
void glWrite(GLdouble x, GLdouble y, const char *str);

/// ----------------------------------   Prototypes   -------------------------------------

/// glTF Show Information boxes
void glTFShowInfo(void);

/// glTF Display
void glTFDisplay(void);

/// glTF Reshape
/// @arg w, h new window size
void glTFReshape(int w, int h);

/// glTF Keyboard
/// @arg key keyboard key hitted
/// @arg x, y mouse position when hit
void glTFKeyboard( unsigned char key, int x, int y );

/// glTF Command
/// @arg value command menu hit
void glTFCommand(int value);

/// glTF Mouse
/// @arg button mouse button event
/// @arg state mouse state event
/// @arg x, y mouse position
void glTFMouse(int button, int state, int x, int y);

/// glTF Motion
/// @arg x, y mouse position
void glTFMotion(int x, int y);

/// glTF Application Setup
extern
void glTFSetup(void);

#endif
