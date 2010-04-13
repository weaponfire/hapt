/**
 *   Volume TVVD Application
 *
 * Maximo, Andre -- Apr, 2009
 *
 */

/**
 *   appVol : defines a class to setup the volume class using application
 *            input parameters.
 *
 * C++ header.
 *
 */

/// --------------------------------   Definitions   ------------------------------------

#ifndef _APPVOL_H_
#define _APPVOL_H_

extern "C" {
#include <GL/gl.h> // OpenGL library
}

#include <string>

using std::string;

#include "tvvdVol.h"

/// ----------------------------------   appVol   ------------------------------------

/// Volume Application

class appVol {

public:

	/// TVVD Volume
	tvvdVol< GLfloat, GLuint > volume;

	typedef tvvdVol< GLfloat, GLuint >::vec3 vec3;
	typedef tvvdVol< GLfloat, GLuint >::vec4 vec4;

	/// Debug flag
	bool debug;

	/// Volume name
	string volName;

	/// File extensions
	string tvvdExt, tfExt, lmtExt;

	/// Searching directory for files
	string searchDir;

	/// Constructor
	appVol(bool _d = true);

	/// Destructor
	~appVol();

	/// Volume Application Setup
	/// @arg argc main argc
	/// @arg argv main argv
	/// @return true if it succeed
	bool setup(int& argc, char** argv);

};

#endif
