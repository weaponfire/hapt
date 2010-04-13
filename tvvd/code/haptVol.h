/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 * Maximo, Andre -- Apr, 2009
 *
 */

/**
 *   haptVol : defines a class for volume rendering using hardware-assisted
 *             projected tetrahedra HAPT.
 *
 * C++ header.
 *
 */

/// --------------------------------   Definitions   ------------------------------------

#ifndef _PTVOL_H_
#define _PTVOL_H_

#include <sys/time.h>

#include "glslKernel.h"

#include "appVol.h"

/// Pre-defined colors
#define WHITE 1.0f, 1.0f, 1.0f
#define BLACK 0.0f, 0.0f, 0.0f

/// ----------------------------------   haptVol   ------------------------------------

/// Geometry PT Volume

class haptVol : public appVol {

public:

	/// Constructor
	haptVol( bool _d = true );

	/// Destructor
	~haptVol();

	/// Size of PT Volume (OpenGL in CPU)
	/// @return openGL usage in Bytes
	GLuint sizeOf(void);

	/// Set functions
	void setColor(const GLclampf& _r, const GLclampf& _g, const GLclampf& _b) {
		backGround = vec3( _r, _g, _b );
		glClearColor(backGround.r(), backGround.g(), backGround.b(), 0.0);
	}

	/// OpenGL Setup
	///   Create arrays, textures and shaders
	/// @return true if it succeed
	bool glSetup(void);

	/// Set use buffer object flag
	/// @arg _b new buffer object usage flag
	void useBufferObject(bool _b = true) { useBufObj = _b; }

	/// Sort
	///   Sort the tetrahedra
	/// @arg _t returns total time spent in sorting (in seconds)
	void sort(GLdouble& _t) {
		static struct timeval starttime, endtime;
		gettimeofday(&starttime, 0);
		sort();
		gettimeofday(&endtime, 0);
		_t = (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec)/1000000.0;
	}
	void sort(void);

	/// Draw
	///   Draw Arrays using one OpenGL function: glDrawArrays
	///   to draw all vertices and its attributes stored into
	///   vertex|texCoord
	/// @arg _t returns total time spent in drawing (in seconds)
	void draw(GLdouble& _t) {
		static struct timeval starttime, endtime;
		gettimeofday(&starttime, 0);
		draw();
		gettimeofday(&endtime, 0);
		_t = (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec)/1000000.0;
	}
	void draw(void);

	/// Refresh Transfer Function (TF) and Brightness
	/// @arg brightness term
	void refreshTFandBrightness(GLfloat brightness = 1.0);

	/// Change frame animation
	/// @arg nextFrame new frame
	void changeFrame(GLuint nextFrame);

private:

	/// Create Arrays
	/// @return true if it succeed
	bool createArrays(void);

	/// Create Output/Input Textures
	/// Texture 0: { Ternary Truth Table (id0, id1, id2, id3) }
	/// Texture 1: { Transfer Function (r, g, b, thau) }
	/// Texture 2: { Psi Gamma Table (psi) }
	void createTextures(void);

	/// Create Shaders
	/// HAPT Shader:  Computes projection and ray integration in the same step
	/// @return true if it succeed
	bool createShaders(void);

	glslKernel *haptShader; ///< HAPT shader

	GLuint *ids; ///< Tetrahedra ids for rendering

	GLfloat *bufArray[4]; ///< Buffer arrays

	GLfloat **bufScalar; ///< Buffer with scalar values

	GLuint bufObject[5]; ///< Buffer Objects

	bool useBufObj; ///< Flag to turn on/off buffer object usage

	GLuint orderTableTex, tfanOrderTableTex,
		tfTex, psiGammaTableTex; ///< Textures used in shaders

	vec3 backGround; ///< Background color

};

#endif
