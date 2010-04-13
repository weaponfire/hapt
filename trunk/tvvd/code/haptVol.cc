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
 * C++ implementation.
 *
 */

/// --------------------------------   Definitions   ------------------------------------

#include <iomanip>

#include "haptVol.h"

#include "errHandle.h"

#include "tables.h"

#include "psiGammaTable512.h"

using std::setprecision;
using std::cerr;
using std::cout;
using std::endl;
using std::flush;

/// ----------------------------------   haptVol   ------------------------------------

/// Constructor
haptVol::haptVol( bool _d ) :
	appVol(_d),
	haptShader(NULL),
	ids(NULL),
	bufScalar(NULL),
	useBufObj(true),
	orderTableTex(0), tfanOrderTableTex(0),
	tfTex(0), psiGammaTableTex(0),
	backGround(WHITE) {

}

/// Destructor
haptVol::~haptVol() {

	if( haptShader ) delete haptShader;

	if( ids ) delete [] ids;

	for (uint i = 0; i < 4; ++i) if( bufArray[i] ) delete [] bufArray[i];

	glDeleteTextures(1, &orderTableTex);
	glDeleteTextures(1, &tfanOrderTableTex);
	glDeleteTextures(1, &tfTex);
	glDeleteTextures(1, &psiGammaTableTex);

	glDeleteBuffers(5, &bufObject[0]);

}

/// OpenGL Setup
bool haptVol::glSetup() {

	try {

		/// OpenGL settings
		glDisable(GL_CULL_FACE);

		/// Source alpha is applied in second fragment shader
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		/// Background color
		glClearColor(backGround.r(), backGround.g(), backGround.b(), 0.0);

		if( debug ) cout << "::: OpenGL :::" << endl << endl;

		if( debug ) cout << "Create textures... " << flush;

		createTextures();

		if( debug ) cout << "done!\nCreate shaders... " << flush;

		if( !createShaders() ) throw errHandle(genericErr, "GLSL Error!");

		if( debug ) cout << "done!\nCreate arrays... " << flush;

		/// Create OpenGL auxiliary data structures
		if( !createArrays() ) throw errHandle(memoryErr);

		if( debug ) cout << "done!" << endl;

		if( debug ) cout << endl << "# Memory Size = " << setprecision(4)
				 << this->sizeOf() / 1048576.0 << " MB " << endl << endl;

		return true;

	} catch(errHandle& e) {

		cerr << e;

		return false;

	} catch(...) {

		throw errHandle();

	}

}

/// Size of Geometry PT Volume (OpenGL in CPU)
GLuint haptVol::sizeOf(void) {

	return ( ( (haptShader) ? haptShader->size_of() : 0 ) + ///< HAPT Shader
		 ( (ids) ? volume.numTets * sizeof(GLuint) : 0 ) + ///< Rendering indices
		 ( (bufArray[0]) ? 12 * volume.numTets * sizeof(GLfloat) : 0 ) + ///< Buffer arrays
		 ( (bufScalar) ? volume.numFrames * volume.numTets * 4 * sizeof(GLfloat) : 0 ) + ///< Buffer scalars
		 ( 9 * sizeof(GLuint) ) + ///< All GLuints
		 ( 5 * sizeof(void*) ) + ///< All pointers
		 ( volume.sizeOf() ) + ///< Size of volume
		 ( PSI_GAMMA_SIZE_BACK * PSI_GAMMA_SIZE_FRONT * sizeof(float) ) ///< Psi Gamma Table
		);

}

/// Create Arrays
bool haptVol::createArrays(void) {

	GLuint nT = volume.numTets;

	ids = new GLuint[nT];

	for (GLuint j = 0; j < 4; ++j)
		bufArray[j] = new GLfloat[nT * 3];

	for (GLuint i = 0; i < nT; ++i)
		for (GLuint j = 0; j < 4; ++j)
			for (GLuint k = 0; k < 3; ++k)
				bufArray[j][i*3 + k] = volume.vertList[ volume.tetList[i][j] ][k];

	GLuint nF = volume.numFrames;

	glGenBuffers(5, &bufObject[0]);

	for (GLuint j = 0; j < 4; ++j) {

		glBindBuffer(GL_ARRAY_BUFFER, bufObject[j]);
		glBufferData(GL_ARRAY_BUFFER, nT * 3 * sizeof(GLfloat), bufArray[j], GL_STATIC_DRAW);

	}

	bufScalar = new GLfloat*[ nF ];

	for (GLuint i = 0; i < nF; ++i) {

		bufScalar[i] = new GLfloat[4*nT];

		for (GLuint j = 0; j < nT; ++j)
			for (GLuint k = 0; k < 4; ++k)
				bufScalar[i][j*4 + k] = volume.scalarList[i][ volume.tetList[j][k] ];

	}

	for(GLuint i = 0; i < nT; ++i)
		ids[i] = i;

 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufObject[4]);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, nT * sizeof(GLuint), ids, GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glClientActiveTexture(GL_TEXTURE0 + 3);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(4, GL_FLOAT, 0, bufScalar[0]);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	return true;

}

/// Change frame animation
void haptVol::changeFrame(GLuint nextFrame) {

	if( nextFrame >= volume.numFrames ) return;

	glClientActiveTexture(GL_TEXTURE0 + 3);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glTexCoordPointer(4, GL_FLOAT, 0, bufScalar[nextFrame]);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

}

/// Create Output/Input Textures
void haptVol::createTextures(void) {

	GLint *orderTableBuffer;
 	orderTableBuffer = new GLint[81*4];

	for (GLuint i = 0; i < 81; ++i)
		for (GLuint j = 0; j < 4; ++j)
			orderTableBuffer[i*4 + j] = (GLint)(order_table[i][j]);

	/// Order Table Texture
	glGenTextures(1, &orderTableTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D,  orderTableTex);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16I_EXT, 81, 0, GL_RGBA_INTEGER_EXT, GL_INT, orderTableBuffer);

	delete [] orderTableBuffer;

	GLint *tfanOrderTableBuffer;
	tfanOrderTableBuffer = new GLint[81*4];

	GLint tfan, k;

	for (GLuint i = 0; i < 81; ++i) {

		k = 0;

		for (GLuint j = 0; j < 5; ++j) {

			tfan = triangle_fan_order_table[i][j];
			if( tfan == -1 ) continue;
			if( k == 4 ) continue;
			tfanOrderTableBuffer[i*4 + k] = tfan;
			++k;

		}

	}

	/// Triangle Fan/Strip Order Table Texture
	glGenTextures(1, &tfanOrderTableTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_1D,  tfanOrderTableTex);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA16I_EXT, 81, 0, GL_RGBA_INTEGER_EXT, GL_INT, tfanOrderTableBuffer);

	delete [] tfanOrderTableBuffer;

	GLfloat *tfTexBuffer;
	tfTexBuffer = new GLfloat[256*4];

	for (GLuint i = 0; i < 256; ++i)
		for (GLuint j = 0; j < 4; ++j)
			tfTexBuffer[i*4 + j] = volume.tf[i][j];

	/// Transfer Function Texture
	glGenTextures(1, &tfTex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_1D, tfTex);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, tfTexBuffer);

	delete [] tfTexBuffer;

	/// Psi Gamma Table Texture
	glGenTextures(1, &psiGammaTableTex);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, psiGammaTableTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, PSI_GAMMA_SIZE_BACK, PSI_GAMMA_SIZE_FRONT,
		     0, GL_ALPHA, GL_FLOAT, psiGammaTable);

}

/// Create Shaders
bool haptVol::createShaders(void) {

	if( !glsl_support() ) return false;

	if( !geom_shader_support() ) return false;

	if( haptShader ) delete haptShader;
	haptShader = new glslKernel();
	if( !haptShader ) return false;

	haptShader->vertex_source("hapt.vert");
	haptShader->geometry_source("hapt.geom");
	haptShader->fragment_source("hapt.frag");
	haptShader->set_geom_max_output_vertices( 8 );
	haptShader->set_geom_input_type(GL_POINTS);
	haptShader->set_geom_output_type(GL_TRIANGLE_STRIP);
	haptShader->install(debug);

	haptShader->use();
 	haptShader->set_uniform("orderTableTex", 0);
 	haptShader->set_uniform("tfanOrderTableTex", 1);
 	haptShader->set_uniform("tfTex", 2);
 	haptShader->set_uniform("psiGammaTableTex", 3);
	haptShader->set_uniform("preIntTexSize", (GLfloat)PSI_GAMMA_SIZE_BACK);
 	haptShader->set_uniform("maxEdgeLength", volume.maxEdgeLength);
	haptShader->set_uniform("brightness", (GLfloat)1.0);
	haptShader->use(0);

	return true;

}

/// Sort
void haptVol::sort() {

/*
	GLuint nT = volume.numTets;

	GLfloat mv[16];

	glGetFloatv(GL_MODELVIEW_MATRIX, mv);

	GLuint *cpuIds = ids; ///< ids in CPU

	if( useBufObj ) { // Get ids in GPU

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufObject[4]);
		ids = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

	}

	/// Sort ...

	if( useBufObj ) {

		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		ids = cpuIds;

	}
*/

}

/// Draw
void haptVol::draw() {

	glEnable(GL_BLEND);

	glEnableClientState(GL_VERTEX_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[0]);
		glVertexPointer(3, GL_FLOAT, 0, 0);
	} else
		glVertexPointer(3, GL_FLOAT, 0, bufArray[0]);

	glClientActiveTexture(GL_TEXTURE0 + 0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[1]);
		glTexCoordPointer(3, GL_FLOAT, 0, 0);
	} else
		glTexCoordPointer(3, GL_FLOAT, 0, bufArray[1]);

	glClientActiveTexture(GL_TEXTURE0 + 1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[2]);
		glTexCoordPointer(3, GL_FLOAT, 0, 0);
	} else
		glTexCoordPointer(3, GL_FLOAT, 0, bufArray[2]);

	glClientActiveTexture(GL_TEXTURE0 + 2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[3]);
		glTexCoordPointer(3, GL_FLOAT, 0, 0);
	} else
		glTexCoordPointer(3, GL_FLOAT, 0, bufArray[3]);

 	glClientActiveTexture(GL_TEXTURE0 + 3);
 	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	haptShader->use();

	if( useBufObj ) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufObject[4]);
		glDrawElements(GL_POINTS, volume.numTets, GL_UNSIGNED_INT, 0);
	} else
		glDrawElements(GL_POINTS, volume.numTets, GL_UNSIGNED_INT, ids);

	haptShader->use(0);

	if( useBufObj ) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glClientActiveTexture(GL_TEXTURE0 + 0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0 + 1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0 + 2);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0 + 3);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisable(GL_BLEND);

}

/// Refresh Transfer Function (TF) and Brightness
void haptVol::refreshTFandBrightness(GLfloat brightness) {

	GLfloat *tfTexBuffer;
	tfTexBuffer = new GLfloat[256*4];

	for (GLuint i = 0; i < 256; ++i)
		for (GLuint j = 0; j < 4; ++j)
			tfTexBuffer[i*4 + j] = volume.tf[i][j];

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_1D, tfTex);
	glTexSubImage1D(GL_TEXTURE_1D, 0, 0, 256, GL_RGBA, GL_FLOAT, tfTexBuffer);

	haptShader->use();
	haptShader->set_uniform("tfTex", 5);
	haptShader->set_uniform("brightness", brightness);
	haptShader->use(0);

	delete [] tfTexBuffer;

}
