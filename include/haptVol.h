/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 * Maximo, Andre -- Mar, 2009
 * Marroquim, Ricardo -- Apr, 2009
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

#include "centroid.cuh"

#include <vcg/math/matrix44.h>
#include <wrap/gl/math.h>

/// Pre-defined colors
#define WHITE 1.0f, 1.0f, 1.0f
#define BLACK 0.0f, 0.0f, 0.0f

enum sortType { none, stl_sort, gpu_bitonic, gpu_quick, mpvo, compare_sort }; ///< Sort methods

enum drawType { dvr, isos, dvr_isos }; ///< Draw methods (Direct Volume Rendering and/or Iso-Surfaces)

typedef struct _tetCentroid {
	GLuint id; ///< Tetrahedron index
	GLfloat cZ; ///< Centroid Z
	friend bool operator < (const struct _tetCentroid& t1, const struct _tetCentroid& t2) {
		return t1.cZ < t2.cZ;
	}
} tetCentroid;

/// ----------------------------------   haptVol   ------------------------------------

typedef appVol::vec3 vec3;

/// Geometry PT Volume

class haptVol : public appVol {

public:
   
	/// Constructor
	haptVol( bool _d = true );

	/// Destructor
	~haptVol();

	/// Size of PT Volume (OpenGL in CPU)
	/// @return openGL usage in Bytes
	int sizeOf(void);

	/// Set functions
	void setColor(const GLclampf& _r, const GLclampf& _g, const GLclampf& _b) {
		backGround = vec3( _r, _g, _b );
		glClearColor(backGround.r(), backGround.g(), backGround.b(), 0.0);
	}

	/// OpenGL Setup
	///   Create arrays, textures and shaders
	/// @return true if it succeed
	bool glSetup(void);

	/// Normalize in range [0, 1]
	void normalize(void);

	/// Set use buffer object flag
	/// @arg _b new buffer object usage flag
	void useBufferObject(bool _b = true) { useBufObj = _b; }

	/// Set use illumination flag
	/// @arg _l new illumination usage flag
	void useIllumination(bool _l = true) { useLight = _l; }

	/// Sort
	///   Sort the tetrahedra using the selected sort method
	/// @arg _t returns total time spent in sorting (in seconds)
	void sort(GLdouble& _t, sortType _sT = none) {
		static struct timeval starttime, endtime;
		gettimeofday(&starttime, 0);
		sort(_sT);
		gettimeofday(&endtime, 0);
		_t = (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec)/1000000.0;
	}
	void sort(sortType _sT);

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

	/// Refresh iso-value
	void refreshISO( void );

	/// Changes the current draw mode
	bool switchShaders( drawType _dt );

private:

	/// Create Arrays
	/// @return true if it succeed
	bool createArrays(void);

	/// Create Centroid Sorts
	/// centroidSorted: {  (tetId, centroidZ), ... }
	/// @return true if it succeed
	bool createCentroidSorts(void);

	/// Create Output/Input Textures
	/// Texture 0: { Ternary Truth Table (id0, id1, id2, id3) }
	/// Texture 1: { Transfer Function (r, g, b, thau) }
	/// Texture 2: { Psi Gamma Table (psi) }
	void createTextures(void);

	/// Create Shaders
	/// HAPT Shader:  Computes projection and ray integration in the same step
	/// @return true if it succeed
	bool createShaders(void);

	/// Compute the centroid sort error
	/// Traverses the adjacency list (con file)
	void computeCentroidSortError( void );

	/// Compute view-dependent DAG (Direct Acyclic Graph)
	/// Second step of the MVPO
	/// @return Number of front facing boundary tets
	GLuint DAG( void );

	/// From the DAG extract the view-depent ordering in depth-first-search manner
	/// Third step of the MVPO
	/// @param Cell id
	void DFS ( GLuint );

	/// Meshed Polyhedra Visibility Ordering for Non Convex Meshes
	/// [Peter L. Williams : Visibility-OrderingMeshed Polyhedra. ACM
	/// Trans. Graph. 11, 2, 1992]
	void MPVO( void );

	glslKernel *haptShader; ///< HAPT shader

	GLuint *ids; ///< Tetrahedra ids for rendering

	ivec4 *dag; ///< MPVO, directions for complementing the connectivity
	
	bool *visited; ///< MPVO, visited flag for DFS algorithm

	bool *visitedCycle; ///< MPVO, visited flag for checking cycles, part of MPVO

	GLuint DFScount; ///< MPVO, counter for outputing the ordered cells into the ids list

	GLfloat *bufArray[4]; ///< Buffer arrays

	GLuint bufObject[5]; ///< Vertex Buffer Objects

	bool useBufObj; ///< Flag to turn on/off buffer object usage

	bool useLight; ///< Flag to turn on/off iso-surface illumination	

	drawType drawMode; ///< Current draw mode, direct volume rendering and/or iso-surfaces

	tetCentroid *centroidSorted; ///< STL sorting

	vec3 *centroidList; ///< Tetrahedron centroids list

	GLuint orderTableTex, tfanOrderTableTex,
		tfTex, psiGammaTableTex; ///< Textures used in shaders

	vec3 backGround; ///< Background color

};

#endif
