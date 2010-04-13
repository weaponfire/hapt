/**
 *   Volume OFF Application
 *
 * Maximo, Andre -- Mar, 2008
 * Marroquim, Ricardo -- Apr, 2009
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

#include "offVol.h"

/// ----------------------------------   appVol   ------------------------------------

/// Volume Application

class appVol {

public:

	/// OFF Volume
	offVol< GLfloat, GLuint > volume;

	typedef offVol< GLfloat, GLuint >::ivec3 ivec3;
	typedef offVol< GLfloat, GLuint >::ivec4 ivec4;
	typedef offVol< GLfloat, GLuint >::vec3 vec3;
	typedef offVol< GLfloat, GLuint >::vec4 vec4;
	

	/// Debug flag
	bool debug;

	/// Volume name
	string volName;

	/// File extensions
	string offExt, tfExt, lmtExt, conExt, isoExt;

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
