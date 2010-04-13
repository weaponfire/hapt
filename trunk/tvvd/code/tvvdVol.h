/**
 *   TVVD (time-varying volume data) describing 4D Dataset
 *
 * Maximo, Andre -- Apr, 2009
 *
 */

/**
 *   tvvdVol : defines a class for pre-computation methods to any volume rendering
 *             technique.
 *
 * C++ header.
 *
 */

/// --------------------------------   Definitions   ------------------------------------

#ifndef _TVVDVOL_H_
#define _TVVDVOL_H_

#include "vec.h"

#include <iostream>
#include <fstream>

using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::flush;
using std::ios;

/// ----------------------------------   tvvdVol   ------------------------------------

/// TVVD Volume Class
/// @template real number type (float, double, etc.)
/// @template natural number type (short, unsigned, long, etc.)
template< class real, class natural >
class tvvdVol {

public:

	/// GLSL like types
	typedef vec< 2, natural > ivec2;
	typedef vec< 3, natural > ivec3;
	typedef vec< 4, natural > ivec4;
	typedef vec< 2, real > vec2;
	typedef vec< 3, real > vec3;
	typedef vec< 4, real > vec4;

	natural numFrames, numVerts, numTets;

	vec3 *vertList;
	ivec4 *tetList;
	real **scalarList;

	vec4 *tf;

	natural numColors;

	real maxEdgeLength, maxZ, minZ;

	/// Constructor -- instantiate zero-volume
	tvvdVol() : numFrames(0), numVerts(0), numTets(0),
		vertList(NULL), tetList(NULL), scalarList(NULL),
		tf(NULL), numColors(256), maxEdgeLength(0),
		maxZ(0), minZ(0) { }

	/// Destructor -- clean up memory
	~tvvdVol() {

		if( vertList ) delete [] vertList;
		if( tetList ) delete [] tetList;

		if( scalarList )
			for (natural i = 0; i < numFrames; ++i)
				delete [] scalarList[i];

		if( tf ) delete [] tf;

	}

	/// Size of the volume
	/// @return size of volume in Bytes
	natural sizeOf(void) {
		return ( ( (vertList) ? numVerts * sizeof(vec3) : 0 ) + ///< Vertices list
			 ( (tetList) ? numTets * sizeof(ivec4) : 0 ) + ///< Tetrahedra list
			 ( (scalarList) ? numFrames * numVerts * sizeof(float) : 0 ) + ///< Scalar list
			 ( (tf) ? numColors * sizeof(vec4) : 0 ) + ///< Transfer Function
			 ( 3 * sizeof(natural) ) + ///< numVerts, numTets and numExtFaces
			 ( 4 * sizeof(void*) ) + ///< pointers
			 ( 3 * sizeof(real) ) ///< maxEdgeLength, maxZ and minZ
			);
	}

	/// --- TVVD ---

	/// Read TVVD (time-varying volume data)
	/// @arg in input binary file stream
	bool readTVVD(ifstream& in) {

		if( in.fail() ) return false;

		in.read( (char*) &numFrames, sizeof(natural) );
		in.read( (char*) &numVerts, sizeof(natural) );
		in.read( (char*) &numTets, sizeof(natural) );

		if( in.fail() ) return false;

		natural i, j;

		/// Allocating memory for vertices, tetrahedra and scalar data

		vertList = new vec3[ numVerts ];
		if( !vertList ) return false;

		tetList = new ivec4[ numTets ];
		if( !tetList ) return false;

		scalarList = new float*[ numFrames ];
		if (!scalarList) return false;
		for (natural i = 0; i < numFrames; ++i)
			scalarList[i] = new float[ numVerts ];

		/// Reading vertices and tetrahedra information

		real vert[3];
		natural tet[4];

		for (i = 0; i < numVerts; i++) {

			in.read( (char*)&vert[0], sizeof(real[3]) );

			for (j = 0; j < 3; ++j)
				vertList[i][j] = vert[j];

			if (in.fail()) return false;
		}

		for(i = 0; i < numTets; i++) {

			in.read( (char*)&tet[0], sizeof(natural[4]) );

			for (j = 0; j < 4; ++j)
				tetList[i][j] = tet[j];

			if (in.fail()) return false;

		}

		/// Reading scalar information

		for (i = 0; i < numFrames; i++) {

			in.read( (char*)&scalarList[i][0], sizeof(float) * numVerts );

			if (in.fail()) return false;

		}


		in.close();

		return true;

	}

	/// Read TVVD (overload)
	/// @arg f tvvd file name
	/// @return true if it succeed
	bool readTVVD(const char* f) {

		ifstream in(f, ios::binary);
		return readTVVD(in);

	}

	/// Normalize vertices coordinates
	void normalizeVertices(void) {

		natural i, j;
		real scaleCoord, maxCoord, value;
		vec3 center;
		vec3 min, max;

		min = max = vertList[0];

		/// Find out the min/max points
		for (i = 1; i < numVerts; i++) {
			for (j = 0; j < 3; j++) { // x, y, z
				value = vertList[i][j];
				if( value < min[j] )
					min[j] = value;
				if( value > max[j] )
					max[j] = value;
			}
		}

		/// Compute the center point
		for (i = 0; i < 3; ++i) { // x, y, z

			center[i] = (min[i] + max[i]) / 2.0;

			/// Center volume in origin
			max[i] -= center[i];
			min[i] -= center[i];

		}

		maxCoord = (max[1] > max[2]) ? max[1] : max[2];
		maxCoord = (max[0] > maxCoord) ? max[0] : maxCoord;

		scaleCoord = 1.0 / maxCoord;

		/// Update vertex list
		for(i = 0; i < numVerts; ++i)
			for (j = 0; j < 3; ++j) // x, y, z
				vertList[i][j] = (vertList[i][j] - center[j]) * scaleCoord;

	}

	/// Normalize scalars
	void normalizeScalars(void) {

		real minS, maxS;

/*
		for (natural i = 0; i < numFrames; ++i) {

			minS = maxS = scalarList[i][0];

			for (natural j = 1; j < numVerts; ++j) {

				if( scalarList[i][j] < minS ) minS = scalarList[i][j];
				if( scalarList[i][j] > maxS ) maxS = scalarList[i][j];

			}

			for (natural j = 0; j < numVerts; ++j)
				scalarList[i][j] = ( scalarList[i][j] - minS ) / (maxS - minS);
		}
*/

		minS = maxS = scalarList[0][0];

		for (natural i = 0; i < numFrames; ++i) {

			for (natural j = 0; j < numVerts; ++j) {

				if( scalarList[i][j] < minS ) minS = scalarList[i][j];
				if( scalarList[i][j] > maxS ) maxS = scalarList[i][j];

			}

		}

		for (natural i = 0; i < numFrames; ++i)
			for (natural j = 0; j < numVerts; ++j)
				scalarList[i][j] = ( scalarList[i][j] - minS ) / (maxS - minS);

	}

	/// --- TF ---

	/// Read TF (transfer function)
	/// @arg in input file stream
	/// @return true if it succeed
	bool readTF(ifstream& in) {

		if (in.fail()) return false;

		in >> numColors;

		if (numColors != 256) return false;

		if (tf) delete [] tf;
		tf = new vec4[numColors];
		if (!tf) return false;

		for(natural i = 0; i < numColors; i++) {

			in >> tf[i];

			if (in.fail()) return false;


		}

		in.close();

		return true;

	}

	/// Read TF (overload)
	/// @arg f transfer function file name
	/// @return true if it succeed
	bool readTF(const char* f) {

		ifstream in(f);

		return readTF(in);

	}

	/// Build a generic TF array
	/// @return true if it succeed
	bool buildTF(void) {

		vec4 c;

		if (tf)	delete [] tf;
		tf = new vec4[numColors];
		if (!tf) return false;

		natural quarter = numColors / 4;
		real stepQuarter = 1.0 / (real)quarter;

		for (natural i = 0; i < numColors; ++i) {

			if (i < quarter) {
				tf[i][0] = 1.0;
				tf[i][1] = i*stepQuarter;
				tf[i][2] = 0.0;
				tf[i][3] = 0.8 - (i*stepQuarter)/5.0;
			} else if (i < 2*quarter) {
				tf[i][0] = 1.0 - (i-quarter)*stepQuarter;
				tf[i][1] = 1.0;
				tf[i][2] = 0.0;
				tf[i][3] = 0.6 - ((i-quarter)*stepQuarter)/5.0;
			} else if (i < 3*quarter) {
				tf[i][0] = 0.0;
				tf[i][1] = 1.0;
				tf[i][2] = (i-2*quarter)*stepQuarter;
				tf[i][3] = 0.4 + ((i-2*quarter)*stepQuarter)/2.0;
			} else if (i < 4*quarter) {
				tf[i][0] = 0.0;
				tf[i][1] = 1.0 - (i-3*quarter)*stepQuarter;
				tf[i][2] = 1.0;
				tf[i][3] = 0.9 - ((i-3*quarter)*stepQuarter)/5.0;
			}

		}

		return true;

	}

	/// Write TF (transfer function)
	/// @arg out output file stream
	/// @return true if it succeed
	bool writeTF(ofstream& out) {

		if (out.fail()) return false;

		if (!tf) return false;

		out << numColors << endl;

		for(natural i = 0; i < numColors; i++) {

			out << tf[i] << endl;

			if (out.fail()) return false;

		}

		out.close();

		return true;

	}

	/// Write TF (overload)
	/// @arg f transfer function file name
	/// @return true if it succeed
	bool writeTF(const char* f) {

		ofstream out(f);

		return writeTF(out);

	}

	/// --- Limits ---

	/// Read Lmt (limits)
	/// @arg in input file stream
	/// @return true if it succeed
	bool readLmt(ifstream& in) {

		if (in.fail()) return false;

		in >> maxEdgeLength >> maxZ >> minZ;

		in.close();

		return true;

	}

	/// Read Lmt (overload)
	/// @arg f limits file name
	/// @return true if it succeed
	bool readLmt(const char* f) {

		ifstream in(f);

		return readLmt(in);

	}

	/// Find maximum edge length inside the volume
	void findMaxEdgeLength(void) {

		real len;

		for (natural i = 0; i < numTets; ++i) { // for each tet

			vec3 edge[6];

			edge[0] = vertList[tetList[i][0]].xyz() - vertList[tetList[i][1]].xyz();
			edge[1] = vertList[tetList[i][0]].xyz() - vertList[tetList[i][2]].xyz();
			edge[2] = vertList[tetList[i][0]].xyz() - vertList[tetList[i][3]].xyz();
			edge[3] = vertList[tetList[i][1]].xyz() - vertList[tetList[i][2]].xyz();
			edge[4] = vertList[tetList[i][1]].xyz() - vertList[tetList[i][3]].xyz();
			edge[5] = vertList[tetList[i][2]].xyz() - vertList[tetList[i][3]].xyz();

			for (natural j = 0; j < 6; ++j) { // for each edge

				len = edge[j].length();

				if (len > maxEdgeLength)
					maxEdgeLength = len;

			}

		}

	}

	/// Find maximum and minimum Z values inside the volume

	void findMaxMinZ(void) {

		maxZ = -1.0;
		minZ = 1.0;

		for (natural i = 0; i < numVerts; ++i) { // for each vertex

			if ( vertList[i][2] > maxZ )
				maxZ = vertList[i][2];

			if ( vertList[i][2] < minZ )
				minZ = vertList[i][2];

		}

	}

	/// Write Lmt (limits)
	/// @arg out output file stream
	/// @return true if it succeed
	bool writeLmt(ofstream& out) {

		if (out.fail()) return false;

		out << maxEdgeLength << " " << maxZ << " " << minZ << endl;

		out.close();

		return true;

	}

	/// Write Lmt (overload)
	/// @arg f limits file name
	/// @return true if it succeed
	bool writeLmt(const char* f) {

		ofstream out(f);

		return writeLmt(out);

	}

};

#endif
