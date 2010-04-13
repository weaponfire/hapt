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
 * C++ implementation.
 *
 */

/**
 * Marroquim, Ricardo -- Apr, 2009
 * Included the MPVONC (MPVO for non convex meshes) visibility sorting method
 */

/// --------------------------------   Definitions   ------------------------------------

#include <iomanip>

#include "haptVol.h"

#include "errHandle.h"

#include "tables.h"

#include "psiGammaTable512.h"

#include <assert.h>

using std::setprecision;
using std::cerr;
using std::cout;
using std::endl;
using std::flush;

#define VERT_SOURCE "shader/hapt.vert"
#define GEOM_DVRISO_SOURCE "shader/hapt.geom"
#define FRAG_DVRISO_SOURCE "shader/hapt.frag"

#define GEOM_DVR_SOURCE "shader/hapt_dvr.geom"
#define FRAG_DVR_SOURCE "shader/hapt_dvr.frag"

#define GEOM_ISO_SOURCE "shader/hapt_iso.geom"
#define FRAG_ISO_SOURCE "shader/hapt_iso.frag"

/// ----------------------------------   haptVol   ------------------------------------

/// Constructor
haptVol::haptVol( bool _d ) :
	appVol(_d),
	haptShader(NULL),
	ids(NULL),
	dag(NULL),
	visited(NULL),
	visitedCycle(NULL),
	useBufObj(true),
	useLight(true),
	drawMode(dvr),
	centroidSorted(NULL),
	centroidList(NULL),
	orderTableTex(0), tfanOrderTableTex(0),
	tfTex(0), psiGammaTableTex(0),
	backGround(WHITE) {
}

/// Destructor
haptVol::~haptVol() {

	if( haptShader ) delete haptShader;

	if( ids ) delete [] ids;

	if( dag ) delete [] dag;

	if( visited ) delete [] visited;

	if( visitedCycle ) delete [] visitedCycle;
	
	for (uint i = 0; i < 4; ++i) if( bufArray[i] ) delete [] bufArray[i];

	if( centroidSorted ) delete [] centroidSorted;

	if( centroidList ) delete [] centroidList;

	glDeleteTextures(1, &orderTableTex);
	glDeleteTextures(1, &tfanOrderTableTex);
	glDeleteTextures(1, &tfTex);
	glDeleteTextures(1, &psiGammaTableTex);

	glDeleteBuffers(5, &bufObject[0]);

	cleanCUDA();

}

/// OpenGL Setup
bool haptVol::glSetup() {

	try {

		/// OpenGL settings
		glDisable(GL_CULL_FACE);

		//normalize();

		/// Source alpha is applied in second fragment shader
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		/// Background color
		glClearColor(backGround.r(), backGround.g(), backGround.b(), 0.0);

		if( debug ) cout << "::: OpenGL / CUDA :::" << endl << endl;

		if( debug ) cout << "Create textures... " << flush;

		createTextures();

		if( debug ) cout << "done!\nCreate shaders... " << flush;

		if( !createShaders() ) throw errHandle(genericErr, "GLSL Error!");

		if( debug ) cout << "done!\nCreate centroid sortings... " << flush;

		if( !createCentroidSorts() ) throw errHandle(memoryErr);

		if( debug ) cout << "done!\nCreate arrays... " << flush;

		/// Create OpenGL auxiliary data structures
		if( !createArrays() ) throw errHandle(memoryErr);

		if( debug ) cout << "done!" << endl;

		if( debug ) cout << endl << "# Memory Size = " << setprecision(4)
				 << this->sizeOf() / 1000000.0 << " MB " << endl << endl;
		

		switchShaders(dvr);

		return true;

	} catch(errHandle& e) {

		cerr << e;

		return false;

	} catch(...) {

		throw errHandle();

	}

}

/// Size of Geometry PT Volume (OpenGL in CPU)
int haptVol::sizeOf(void) {

  return ( ( (haptShader) ? haptShader->size_of() : 0 ) + ///< HAPT Shader
		   ( (centroidSorted) ? volume.numTets * sizeof(tetCentroid) : 0 ) + ///< Tet Centroids
		   ( (centroidList) ? volume.numTets * sizeof(vec3) : 0 ) + ///< Tetrahedron centroid list
		   ( 9 * sizeof(GLuint) ) + ///< All GLuints
		   ( 5 * sizeof(void*) ) + ///< All pointers
		   ( PSI_GAMMA_SIZE_BACK * PSI_GAMMA_SIZE_FRONT * sizeof(float) ) ///< Psi Gamma Table
		);

}

/// Normalize from range [-1, 1]
void haptVol::normalize(void) {

	for (GLuint i = 0; i < volume.numVerts; ++i) {

		for (GLuint j = 0; j < 3; ++j) {

			volume.vertList[i][j] = (volume.vertList[i][j] + 1.0) / 2.0;

		}

	}

}

/// Create Arrays
bool haptVol::createArrays(void) {

	GLuint nT = volume.numTets;

	ids = new GLuint[nT];

	dag = new ivec4[nT];

	visited = new bool[nT];

	visitedCycle = new bool[nT];

	for (GLuint j = 0; j < 4; ++j)
		bufArray[j] = new GLfloat[nT * 4];

	for (GLuint i = 0; i < nT; ++i)
		for (GLuint j = 0; j < 4; ++j)
			for (GLuint k = 0; k < 4; ++k)
				bufArray[j][i*4 + k] = volume.vertList[ volume.tetList[i][j] ][k];

	glGenBuffers(5, &bufObject[0]);

	for (GLuint j = 0; j < 4; ++j) {

		glBindBuffer(GL_ARRAY_BUFFER, bufObject[j]);
		glBufferData(GL_ARRAY_BUFFER, nT * 4 * sizeof(GLfloat), bufArray[j], GL_STATIC_DRAW);

	}

 	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufObject[4]);
 	glBufferData(GL_ELEMENT_ARRAY_BUFFER, nT * sizeof(GLuint), 0, GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	return true;

}

/// Create Centroid Sorts
bool haptVol::createCentroidSorts(void) {

	GLuint i, nT = volume.numTets;

	/// Centroid sorted (stl)
	if( centroidSorted ) delete [] centroidSorted;
	centroidSorted = new tetCentroid[nT];
	if( !centroidSorted ) return false;

	for (i = 0; i < nT; ++i) {

		centroidSorted[i].id = i;
		centroidSorted[i].cZ = 0.0;

	}

	/// Allocating memory for centroids
	if( centroidList ) delete [] centroidList;
	centroidList = new vec3[ nT ];
	if( !centroidList ) return false;

	/// Compute the centroid of each tetrahedron
	for (GLuint i = 0; i < nT; ++i) {

		centroidList[i] = ( volume.vertList[ volume.tetList[i][0] ].xyz()
				    + volume.vertList[ volume.tetList[i][1] ].xyz()
				    + volume.vertList[ volume.tetList[i][2] ].xyz()
				    + volume.vertList[ volume.tetList[i][3] ].xyz() ) / 4.0;

	}

	/// Initializing CUDA environment

	if( debug ) cout << "CUDA Initialization... " << flush;

	float *h_centroidList;
	h_centroidList = new float[ nT*4 ];
	if( !h_centroidList ) return false;

	for (GLuint i = 0; i < nT; ++i) {

		for (GLuint j = 0; j < 3; ++j)
			h_centroidList[ i*4 + j ] = centroidList[i][j];

		h_centroidList[ i*4 + 3 ] = 1.0;

	}

	initCUDA( h_centroidList, nT );

	delete [] h_centroidList;

	if( debug ) cout << "done!" << endl;

	return true;

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

	haptShader->vertex_source(VERT_SOURCE);
	haptShader->geometry_source(GEOM_DVRISO_SOURCE);
	haptShader->fragment_source(FRAG_DVRISO_SOURCE);
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

	refreshISO();

	return true;

}

/// Depth-First-Search
void haptVol::DFS( GLuint id ){

  if (visited[id] == true)
	return;

  visited[id] = true;
  visitedCycle[id] = true;
  GLuint adjId = 0;

  // for each adjacent tet
  for (GLuint j = 0; j < 4; j++) {
	adjId = volume.conTet[id][j];

	// not boundary adjacency (index is current)
	if ((adjId != id)) {

	  // check if predecessor (incoming arrow)
	  if (dag[id][j] == 1) {
		if (!visited[adjId]) {
		  DFS(adjId);
		}		
		else if (visitedCycle[adjId] == true) { //CYCLE
		  //cout << "CYCLE" << endl;
		}
	  }
	}
  }
  visitedCycle[id] = false;
  
  // output cell
  ids[DFScount] = id;
  DFScount++;
 
  return;
}

/// Direct Acyclic Graph
GLuint haptVol::DAG( void ){
  // for each tet compute if arrow is coming or going to neighbor
  GLuint nT = volume.numTets;

  // View direction x inverted modelview (avoid rotating normals)
  vcg::Matrix44f imv;
  glGetv(GL_MODELVIEW_MATRIX, imv);
  Invert(imv);
  vcg::Point3f vd = imv * vcg::Point3f(0, 0, -1);
  vec3 viewDir (vd[0], vd[1], vd[2]);

  // original modelview for centroid rotation
  GLfloat mv[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, mv);

  GLuint adjId = 0;
  vec3 normal;
  bool boundary;
  GLuint centroidId = 0;
  vec4 centroidVert;
  GLfloat centroidZ;

  // for each tet
  for (GLuint i = 0; i < nT; ++i) {

	// resets vector while computing dag
	visited[i] = false;
	visitedCycle[i] = false;
	boundary = false;

	// for each adjacent tetrahedral, i.e., each face of current tet
	for (GLuint j = 0; j < 4; ++j) {
	  adjId = volume.conTet[i][j];
	  // boundary face, check if front facing or back facing
	  // only compute once for each tet (might have more than one boundary face)
	  if ((adjId == i) && (!boundary)) {
  		normal = volume.faceNormals[i*4 + j];

		// check if it is front facing boundary face
		if ((normal ^ viewDir) >= 0.0) {			  
		  boundary = true;

		  // retrieve tetrahedron centroid
		  centroidVert = vec4( centroidList[i].x(), centroidList[i].y(), centroidList[i].z(), 1.0 );

		  /// Apply ModelView Matrix mv (z -> r=2)
		  centroidZ = 0.0;
		  for (GLuint c = 0; c < 4; ++c)
			centroidZ += mv[2+c*4] * centroidVert[c];

		  // insert centroid in list of boundary tets to be sorted
		  centroidSorted[centroidId].id = i;
		  centroidSorted[centroidId].cZ = centroidZ;
		  centroidId ++;			
		}		
	  }
	  // only compare once for each pair of neighboring tets
	  // set direction also for neighbor
	  else if ((adjId > i)) { 
		// compute if tet is occluding or occluded by neighbor
		normal = volume.faceNormals[i*4 + j];

		// if normal and view_dir are pointing in same direction current tet occluded
		if ((normal ^ viewDir) > 0.0) {
		  // arrow out, current occluded by adjacent
		  dag[i][j] = 0;
		  for (GLuint k = 0; k < 4; k++) { // search for current tet in adjacency list of neighbor
			if (volume.conTet[adjId][k] == i)
			  dag[adjId][k] = 1; // adjacent tet - arrow in
		  }
		}
		else {
		  dag[i][j] = 1;
		  for (GLuint k = 0; k < 4; k++) { // search for current tet in adjacency list of neighbor
			if (volume.conTet[adjId][k] == i) 
			  dag[adjId][k] = 0; // adjacent tet - arrow out
		  }
		} 
	  }
	}
  }
  
  return centroidId;
}

/// Meshed Polyhedra Visibility Ordering for Non-Convex Meshes
/// The mpvo for non convex meshes runs exactly as the original mpvo but
/// executes the DFS traversing the centroid ordering of the front facing boundary cells
void haptVol::MPVO( void ) {

	// Compute Direct Acyclic Graph direction (MPVO Phase II)
	GLuint boundaryTets = DAG();

	/// STL centroid sort for boundary faces only
	std::sort( centroidSorted, centroidSorted + boundaryTets, less<tetCentroid>() );

	// resets the global counter for the final ordering during DFS
	DFScount = 0;

	// Depth First Search (MPVO Phase III)
	for (GLuint i = 0; i < boundaryTets; i++)
		DFS(centroidSorted[i].id);

}

/// Sort
void haptVol::sort(sortType _sT) {

	if( _sT == none ) return;

	GLuint nT = volume.numTets;

	GLfloat mv[16];

	glGetFloatv(GL_MODELVIEW_MATRIX, mv);

	GLuint *cpuIds = ids; ///< ids in CPU

	if( useBufObj ) { // Get ids in GPU

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufObject[4]);
		ids = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

	}

	/// Switch to the selected sort method
	if( _sT == stl_sort) {

		vec4 centroidVert;
		GLfloat centroidZ;

		/// Fill the centroid sorted array using the centroid Z
		for (GLuint i = 0; i < nT; ++i) {

			centroidVert = vec4( centroidList[i].x(), centroidList[i].y(),
					     centroidList[i].z(), 1.0 );

			/// Apply ModelView Matrix mv (z -> r=2)
			centroidZ = 0.0;
			for (GLuint c = 0; c < 4; ++c)
				centroidZ += mv[2+c*4] * centroidVert[c];

			centroidSorted[i].id = i;
			centroidSorted[i].cZ = centroidZ;
		}

		/// STL sort
		std::sort( centroidSorted, centroidSorted + nT, less<tetCentroid>() );

		/// ids has ordered list of ids back-to-front
		for(GLuint i = 0; i < nT; ++i)
			ids[i] = centroidSorted[i].id;		  

	} else if(_sT == mpvo) {

		MPVO();

	} else if( _sT == gpu_bitonic ) {

		bitonicSortCUDA( ids, mv[2], mv[6], mv[10] );

	} else if( _sT == gpu_quick ) {
		quickSortCUDA( ids, mv[2], mv[6], mv[10] );
	}

	if( useBufObj ) {
		glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		ids = cpuIds;
	}	

}

/// Draw
void haptVol::draw() {

	glEnable(GL_BLEND);

	glEnableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_CULL_FACE);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[0]);
		glVertexPointer(4, GL_FLOAT, 0, 0);
	} else
		glVertexPointer(4, GL_FLOAT, 0, bufArray[0]);

	glClientActiveTexture(GL_TEXTURE0 + 0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[1]);
		glTexCoordPointer(4, GL_FLOAT, 0, 0);
	} else
		glTexCoordPointer(4, GL_FLOAT, 0, bufArray[1]);

	glClientActiveTexture(GL_TEXTURE0 + 1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[2]);
		glTexCoordPointer(4, GL_FLOAT, 0, 0);
	} else
		glTexCoordPointer(4, GL_FLOAT, 0, bufArray[2]);

	glClientActiveTexture(GL_TEXTURE0 + 2);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if( useBufObj ) {
		glBindBuffer(GL_ARRAY_BUFFER, bufObject[3]);
		glTexCoordPointer(4, GL_FLOAT, 0, 0);
	} else
		glTexCoordPointer(4, GL_FLOAT, 0, bufArray[3]);

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
	haptShader->use(0);

	if (drawMode != isos) {
	  haptShader->use();
	  haptShader->set_uniform("brightness", brightness);
	  haptShader->use(0);
	}
	delete [] tfTexBuffer;

}

/// Refresh Iso-Surface
void haptVol::refreshISO( void ) {

  if (drawMode != dvr) {	
	haptShader->use();
	haptShader->set_uniform("isoValue", volume.iso[0][0], volume.iso[1][0], volume.iso[2][0], volume.iso[3][0] );
	haptShader->set_uniform("isoOpacity", volume.iso[0][1], volume.iso[1][1], volume.iso[2][1], volume.iso[3][1] );
	haptShader->set_uniform("illumination", volume.iso[6][0], volume.iso[4][0], volume.iso[5][0]);
	haptShader->set_uniform("illuminate", (GLint)useLight);
	haptShader->use(0);
  }
}

/// Create Shaders
bool haptVol::switchShaders( drawType _dt ) {

  if( haptShader ) delete haptShader;
  haptShader = new glslKernel();
  if( !haptShader ) return false;

  drawMode = _dt;

  GLint max_geom_output = 8;

  if (_dt == dvr_isos)
	max_geom_output = 16;

  haptShader->vertex_source(VERT_SOURCE);
  if (_dt == dvr) {
	haptShader->geometry_source(GEOM_DVR_SOURCE);
	haptShader->fragment_source(FRAG_DVR_SOURCE);
  }
  else if (_dt == isos) {
	haptShader->geometry_source(GEOM_ISO_SOURCE);
	haptShader->fragment_source(FRAG_ISO_SOURCE);
  }
  else if (_dt == dvr_isos) {
	haptShader->geometry_source(GEOM_DVRISO_SOURCE);
	haptShader->fragment_source(FRAG_DVRISO_SOURCE);
  }

  haptShader->set_geom_max_output_vertices( max_geom_output );
  haptShader->set_geom_input_type(GL_POINTS);
  haptShader->set_geom_output_type(GL_TRIANGLE_STRIP);
  haptShader->install(0);

  haptShader->use();
  // for DVR and Isos
  haptShader->set_uniform("tfTex", 2);
  haptShader->set_uniform("psiGammaTableTex", 3);
  haptShader->set_uniform("preIntTexSize", (GLfloat)PSI_GAMMA_SIZE_BACK);

  // only for DVR
  if (_dt != isos) {
	haptShader->set_uniform("orderTableTex", 0);
	haptShader->set_uniform("tfanOrderTableTex", 1);
 	haptShader->set_uniform("maxEdgeLength", volume.maxEdgeLength);
	haptShader->set_uniform("brightness", (GLfloat)1.0);
  }

  haptShader->use(0);

  // only for isos
  refreshISO();
	
  return true;
}
