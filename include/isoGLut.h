/**
 *
 *    OpenGL iso-surfaces Utilities
 *
 * Marroquim, Ricardo -- April, 2009
 *
 **/

/**
 *   isoGLut functions : handle an auxiliary window for iso-surfaces editing.
 *
 * C++ header.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#ifndef _ISOGLUT_H_
#define _ISOGLUT_H_

#include "haptVol.h"

#include "isoSurfaces.h"

extern haptVol app;

isoSurfaces< GLfloat, GLuint > *iso;

extern
void glWrite(GLdouble x, GLdouble y, const char *str);

/// ----------------------------------   Prototypes   -------------------------------------

/// glISO Show Information boxes
void glISOShowInfo(void);

/// glISO Display
void glISODisplay(void);

/// glISO Reshape
/// @arg w, h new window size
void glISOReshape(int w, int h);

/// glISO Refresh
void glRefreshISO(void);

/// glISO Keyboard
/// @arg key keyboard key hitted
/// @arg x, y mouse position when hit
void glISOKeyboard( unsigned char key, int x, int y );

/// glISO Command
/// @arg value command menu hit
void glISOCommand(int value);

/// glISO Mouse
/// @arg button mouse button event
/// @arg state mouse state event
/// @arg x, y mouse position
void glISOMouse(int button, int state, int x, int y);

/// glISO Motion
/// @arg x, y mouse position
void glISOMotion(int x, int y);

/// glISO Application Setup
extern
void glISOSetup(void);

#endif
