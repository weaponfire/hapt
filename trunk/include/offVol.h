/**
 *   OFF (object file format) describing Volume
 *
 * Maximo, Andre -- Mar, 2008
 * Marroquim, Ricardo -- Apr, 2009
 *
 */

/**
 *   offVol : defines a class for pre-computation methods to any volume rendering
 *            technique.
 *
 * C++ header.
 *
 */

/**
 * Marroquim, Ricardo -- Apr, 2009
 * included face normals structure (used with MPVO sort)
 */

/**
 * Marroquim, Ricardo -- Apr, 2009
 * included methods for reading, writing and creating Iso-Surfaces files (.iso)
 */


/// --------------------------------   Definitions   ------------------------------------

#ifndef _OFFVOL_H_
#define _OFFVOL_H_

#include "vec.h" ///< vec template class in lcg toolkit

#include <iostream>
#include <fstream>

#include <vector>
#include <set>

#include <algorithm>

using std::vector;
using std::set;
using std::less;
using std::ifstream;
using std::ofstream;
using std::cout;
using std::endl;
using std::flush;
using std::stable_sort;

/// 4-Module operation using AND operation
/// @arg x, y numbers to be summed
/// @return 4-module of the sum: ( x + y ) % 4
#define MOD4(x,y)           ((x+y)&3)

/// ----------------------------------   offVol   ------------------------------------

/// OFF Volume Class
/// @template real number type (float, double, etc.)
/// @template natural number type (short, unsigned, long, etc.)
template< class real, class natural >
class offVol {

public:

	/// GLSL like types
	typedef vec< 2, natural > ivec2;
	typedef vec< 3, natural > ivec3;
	typedef vec< 4, natural > ivec4;
	typedef vec< 2, real > vec2;
	typedef vec< 3, real > vec3;
	typedef vec< 4, real > vec4;

	/// Incident in vertex: tetrahedra and vertices
	typedef struct _incident {
		vector < natural > tetId;
		set < natural, less<natural> > vertId;
	} incident;

	/// Set iterator type to iterate through a set of vertices
	typedef typename set< uint, less<uint> >::const_iterator setIt;

	natural numVerts, numTets, numExtFaces;

	vec4 *vertList;
	ivec4 *tetList;

	incident *incidVert;

	ivec4 *conTet;

	vec4 *tf;

	vec2 *iso;

	natural numColors;

	natural numIsos;

	real maxEdgeLength, maxZ, minZ;

	ivec2 *extFaces;

	vec3 *faceNormals; ///< Precomputed face normals for MPVONC

	/// Constructor -- instantiate zero-volume
 offVol() : numVerts(0), numTets(0),
	  numExtFaces(0), vertList(NULL),
	  tetList(NULL), incidVert(NULL),
	  conTet(NULL), tf(NULL), iso(NULL),
	  numColors(256), numIsos(7), maxEdgeLength(0),
	  maxZ(0), minZ(0),
	  extFaces(NULL), faceNormals(NULL) { }

	/// Destructor -- clean up memory
	~offVol() {

		if (vertList) delete [] vertList;
		if (tetList) delete [] tetList;
		if (incidVert) delete [] incidVert;
		if (conTet) delete [] conTet;
		if (tf) delete [] tf;
		if (iso) delete [] iso;
		if (extFaces) delete [] extFaces;
		if (faceNormals) delete [] faceNormals;
	}

	/// Size of the volume
	/// @return size of volume in Bytes
	int sizeOf(void) {
	  return ( ( (vertList) ? numVerts * sizeof(vec4) : 0 ) + ///< Vertices list
			   ( (tetList) ? numTets * sizeof(ivec4) : 0 ) + ///< Tetrahedra list
			   ( (extFaces) ? numExtFaces * sizeof(ivec2) : 0 ) + ///< External Faces
			   ( (incidVert) ? numVerts * sizeof(incident) : 0 ) + ///< Incident
			   ( (conTet) ? numTets * sizeof(ivec4) : 0 ) + ///< Connectivity
			   ( (faceNormals) ? numTets * 4 * sizeof(ivec3) : 0 ) + ///< Face Normals
			   ( (tf) ? numColors * sizeof(vec4) : 0 ) + ///< Transfer Function
			   ( 3 * sizeof(natural) ) + ///< numVerts, numTets and numExtFaces
			   ( 6 * sizeof(int) ) + ///< pointers
			   ( 3 * sizeof(real) ) ///< maxEdgeLength, maxZ and minZ
			   );
	}

	/// --- OFF ---

	/// Read OFF (object file format)
	/// @arg in input file stream
	bool readOff(ifstream& in) {

		if (in.fail()) return false;
		in >> numVerts >> numTets;

		/// Allocating memory for vertices and tetrahedra data
		if (vertList) delete [] vertList;
		vertList = new vec4[ numVerts ];
		if (!vertList) return false;

		if (tetList) delete [] tetList;
		tetList = new ivec4[ numTets ];
		if (!tetList) return false;

		natural i;

		/// Reading vertices and tetrahedra information
		for(i = 0; i < numVerts; i++) {
			in >> vertList[i];
			if (in.fail()) return false;
		}

		for(i = 0; i < numTets; i++) {
			in >> tetList[i];
			if (in.fail()) return false;
		}

		in.close();
		return true;

	}

	/// Read OFF (overload)
	/// @arg f off file name
	/// @return true if it succeed
	bool readOff(const char* f) {

		ifstream in(f);
		return readOff(in);

	}

	/// Normalize vertices coordinates
	void normalizeVertices(void) {

		natural i, j;
		real scaleCoord, scaleScalar, maxCoord, value;
		vec3 center;
		vec4 min, max;

		min = vertList[0];
		max = vertList[0];

		/// Find out the min/max points
		for (i = 1; i < numVerts; i++) {
			for (j = 0; j < 4; j++) { // x, y, z, s
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
		scaleScalar = 1.0 / (max[3] - min[3]);

		/// Update vertex list
		for(i = 0; i < numVerts; ++i) {
			for (j = 0; j < 3; ++j) { // x, y, z
				vertList[i][j] = (vertList[i][j] - center[j]) * scaleCoord;
			}
			vertList[i][3] = (vertList[i][3] - min[3]) * scaleScalar;
		}
	   
		real min_scalar = 0/255.0;
		real max_scalar = 255/255.0;
/* 		real min_scalar = 186/255.0; // torso */
/* 		real max_scalar = 190/255.0; // torso */
/* 		real min_scalar = 182.83/255.0; // delta */
/* 		real max_scalar = 182.85/255.0; // delta */
/* 		real min_scalar = 175/255.0; // f117 */
/* 		real max_scalar = 180/255.0; // f117 */
/* 		real min_scalar = 30/255.0; // fighter */
/* 		real max_scalar = 75/255.0; // fighter */

		for (i = 0; i < numVerts; i++) {
			if( vertList[i][3] < min_scalar ) vertList[i][3] = min_scalar;
			if( vertList[i][3] > max_scalar ) vertList[i][3] = max_scalar;
		}

		for (i = 0; i < numVerts; i++)
			vertList[i][3] = (vertList[i][3] - min_scalar) / (max_scalar - min_scalar);
	}

	/// --- Incid ---

	/// Read Con (incidents in vertex)
	/// @arg in input file stream
	bool readIncid(ifstream& in) {

		if (in.fail()) return false;

		natural i, j, numIncidVerts, tetIdSize, vertIdSize, tetId, vertId;

		in >> numIncidVerts;

		if (numIncidVerts != numVerts) return false;

		/// Allocating memory for incidents in vertex data

		if (incidVert) delete [] incidVert;
		incidVert = new incident[ numVerts ];
		if (!incidVert) return false;

		/// Reading indents in vertex information

		for(i = 0; i < numVerts; i++) {

			in >> tetIdSize;

			for (j = 0; j < tetIdSize; j++) {

				in >> tetId;

				incidVert[i].tetId.push_back( tetId );

				if (in.fail()) return false;

			}

			in >> vertIdSize;

			for (j = 0; j < vertIdSize; j++) {

				in >> vertId;

				incidVert[i].vertId.insert( vertId );

				if (in.fail()) return false;

			}

		}

		in.close();

		return true;

	}

	/// Read Incid (overload)
	/// @arg f incid file name
	/// @return true if it succeed
	bool readIncid(const char* f) {

		ifstream in(f);

		return readIncid(in);

	}

	/// Build incidTet array
	/// @return true if it succeed
	bool buildIncid(void) {

		natural i, j, k;

		incidVert = new incident[ numVerts ];
		if (!incidVert) return false;
    
		if (!tetList) return false;

		for (i = 0; i < numVerts; ++i) { /// for each vertex
            
			incidVert[i].tetId.clear();

			incidVert[i].vertId.clear();

			for (j = 0; j < numTets; ++j) {
               
				for (k = 0; k < 4; ++k) {

					/// if tetrahedron j contains the vertex i
					if (i == tetList[j][k]) {

						/// tetrahedron j is incident on vertex i
						incidVert[i].tetId.push_back( j );

						/// other three vertices are incident on vertex i
						incidVert[i].vertId.insert( tetList[j][MOD4(1, k)] );
						incidVert[i].vertId.insert( tetList[j][MOD4(2, k)] );
						incidVert[i].vertId.insert( tetList[j][MOD4(3, k)] );

						break;

					} // if

				} // k

			} // j

			if ( incidVert[i].tetId.empty() ) {

				return false; ///< some anomaly happens

			} // if

		} // i

		return true;

	}

	/// Write Incid (incidents in vertex)
	/// @arg out output file stream
	bool writeIncid(ofstream& out) {

		if (out.fail()) return false;

		if (!incidVert) return false;

		natural i, j;

		out << numVerts << endl;

		/// Writing indents in vertex information
		///  Line 1: [ # incident tet ] [ list of tet ids ... ]
		///  Line 2: [ # incident vert ] [ list of vert ids ... ]

		for(i = 0; i < numVerts; i++) {

			out << incidVert[i].tetId.size() << " ";

			for (j = 0; j < incidVert[i].tetId.size(); j++) {

				out << incidVert[i].tetId[j];

				if ( j < (incidVert[i].tetId.size()-1) )
					out << " ";
				else
					out << endl;

				if (out.fail()) return false;

			}

			out << incidVert[i].vertId.size() << " ";

			j = 0;

			for (setIt it = incidVert[i].vertId.begin(); it != incidVert[i].vertId.end(); it++) {

				out << (*it);

				if ( j < (incidVert[i].vertId.size()-1) )
					out << " ";
				else
					out << endl;

				if (out.fail()) return false;

				j++;

			}

		}

		out.close();

		return true;

	}

	/// Write Incid (overload)
	/// @arg f incid file name
	/// @return true if it succeed
	bool writeIncid(const char* f) {

		ofstream out(f);

		return writeIncid(out);

	}

	/// Delete incidTet array
	/// @return true if it succeed
	bool deleteIncid(void) {

		if (!incidVert)
			return false;

		for (natural i = 0; i < numVerts; ++i) {

			incidVert[i].tetId.clear();
			incidVert[i].vertId.clear();

		}

		if (incidVert)
			delete [] incidVert;

		incidVert = NULL;

		return true;

	}

	/// --- Con ---

	/// Read Con (tetrahedra connectivity)
	/// @arg in input file stream
	/// @return true if it succeed
	bool readCon(ifstream& in) {

		if (in.fail()) return false;

		natural numCons;

		in >> numExtFaces;

		in >> numCons;

		if (numCons != numTets) return false;

		/// Allocating memory for tetrahedra connectivity data

		if (conTet) delete [] conTet;
		conTet = new ivec4[ numTets ];
		if (!conTet) return false;

		/// Reading tetrahedra connectivity information

		for(natural i = 0; i < numTets; i++) {

			in >> conTet[i];

			if (in.fail()) return false;

		}

		in.close();

		return true;

	}

	/// Read Con (overload)
	/// @arg f conTet file name
	/// @return true if it succeed
	bool readCon(const char* f) {

		ifstream in(f);

		return readCon(in);

	}

	/// Build tetrahedra connectivity
	/// @return true if it succeed
	bool buildCon(void) {

		natural i, j, k, l, f, currTetId;

		numExtFaces = 0;

		natural currVId[3];

		if (!incidVert) return false;

		if (conTet) delete [] conTet;
		conTet = new ivec4[ numTets ];
		if (!conTet) return false;

		for (i = 0; i < numTets; i++) { /// for each tet

			for (f = 0; f < 4; f++) { /// for each face

				for (k = 0; k < 3; k++) { // for each vertex of current face

					currVId[k] = tetList[i][ MOD4(k, f) ];

				}

				conTet[i][f] = i; /// sets all face as external face

				for (j = 0; j < incidVert[ currVId[0] ].tetId.size(); j++) {

					currTetId = incidVert[ currVId[0] ].tetId[j];

					if (i != currTetId) {

						for (k = 0; k < incidVert[ currVId[1] ].tetId.size(); k++) {

							if ( incidVert[ currVId[1] ].tetId[k] > currTetId )
								break;

							if ( currTetId == incidVert[ currVId[1] ].tetId[k] ) {

								for (l = 0; l < incidVert[ currVId[2] ].tetId.size(); l++) {

									if ( incidVert[ currVId[2] ].tetId[l] > currTetId )
										break;

									if ( currTetId == incidVert[ currVId[2] ].tetId[l] ) {

										conTet[i][f] = currTetId;

										break;

									}

								} // l

								if (conTet[i][f] == currTetId) /// it has already been found
									break;

							} // if

						} // k

						if (conTet[i][f] == currTetId) /// it has already been found
							break;

					} // if

				} // j

				if (conTet[i][f] == i) /// if this is an external face
					numExtFaces++;

			} // f

		} // i

		return true;

	}

	/// Write Con (tetrahedra connectivity)
	/// @arg out output file stream
	/// @return true if it succeed
	bool writeCon(ofstream& out) {

		if (out.fail()) return false;

		if (!conTet) return false;

		out << numExtFaces << endl;

		out << numTets << endl;

		/// Writing tetrahedra connectivity information

		for(natural i = 0; i < numTets; i++) {

			out << conTet[i] << endl;

			if (out.fail()) return false;

		}

		out.close();

		return true;

	}

	/// Write Con (overload)
	/// @arg f conTet file name
	/// @return true if it succeed
	bool writeCon(const char* f) {

		ofstream out(f);

		return writeCon(out);

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
	

	/// --- ISO ---

	/// Read Isos
	/// @arg in input file stream
	/// @return true if it succeed
	bool readISO(ifstream& in) {

		if (in.fail()) return false;

		in >> numIsos;
		
		if (numIsos != 7) return false; // 4 isos + 3 light parameters

		if (iso) delete [] iso;
		iso = new vec2[numIsos];
		if (!iso) return false;

		for(natural i = 0; i < numIsos; ++i) {

		  in >> iso[i][0] >> iso[i][1];

		  if (in.fail()) return false;

		}

		in.close();

		return true;
	}


	/// Read ISO (overload)
	/// @arg f isos file name
	/// @return true if it succeed
	bool readISO(const char* f) {

		ifstream in(f);

		return readISO(in);
	}


	/// Write ISO
	/// @arg out output file stream
	/// @return true if it succeed
	bool writeISO(ofstream& out) {

		if (out.fail()) return false;

		if (!iso) return false;

		out << numIsos << endl;

		for(natural i = 0; i < numIsos; i++) {

			out << iso[i][0] << " " << iso[i][1] << endl;
			if (out.fail()) return false;

		}		

		out.close();

		return true;
	}

	/// Write ISO (overload)
	/// @arg f isos file name
	/// @return true if it succeed
	bool writeISO(const char* f) {

		ofstream out(f);

		return writeISO(out);

	}

	/// Build a generic ISO array
	/// @return true if it succeed
	bool buildISO(void) {

		vec4 c;

		if (iso) delete [] iso;
		iso = new vec2[numIsos];
		if (!iso) return false;

		for (natural i = 0; i < numIsos; ++i) {
		  iso[i][0] = 0.0;
		  iso[i][1] = 0.0;
		}

		return true;

	}

	/// --- External Faces ---

	/// Read ExtF (external faces)
	/// @arg in input file stream
	/// @return true if it succeed
	bool readExtF(ifstream& in) {

		if (in.fail()) return false;

		natural nExtFaces;

		in >> nExtFaces;
		if (nExtFaces != numExtFaces) return false;

		if (extFaces) delete [] extFaces;
		extFaces = new ivec2[ numExtFaces ];
		if (!extFaces) return false;

		for(natural i = 0; i < numExtFaces; ++i) {

			in >> extFaces[i];

			if (in.fail()) return false;

		}

		in.close();

		return true;

	}

	/// Read ExtF (overload)
	/// @arg f external faces file name
	/// @return true if it succeed
	bool readExtF(const char* f) {

		ifstream in(f);

		return readExtF(in);

	}

	/// Build External Faces vector
	/// @return true if it succeed
	bool buildExtF(void) {

		if (extFaces) delete [] extFaces;
		extFaces = new ivec2[ numExtFaces ];
		if (!extFaces) return false;

		natural extFacesId = 0;

		for (natural i = 0; i < numTets; ++i) { // tets

			for (natural f = 0; f < 4; ++f) { // faces

				/// discard non-external faces
				if (conTet[i][f] != i) continue;

				/// fill external faces vector
				extFaces[ extFacesId ][0] = i;
				extFaces[ extFacesId ][1] = f;

				extFacesId++; // external faces id

			}

		}

		return true;

	}

	/// Write ExtF (external faces)
	/// @arg out output file stream
	/// @return true if it succeed
	bool writeExtF(ofstream& out) {

		if (out.fail()) return false;

		if (!extFaces) return false;

		out << numExtFaces << endl;

		for(natural i = 0; i < numExtFaces; i++) {

			out << extFaces[i] << endl;

			if (out.fail()) return false;

		}

		out.close();

		return true;

	}

	/// Write ExtF (overload)
	/// @arg f external faces file name
	/// @return true if it succeed
	bool writeExtF(const char* f) {

		ofstream out(f);

		return writeExtF(out);

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


	/// Compute the normal for each tetrahedron face
	/// This should be further compacted because it is computing twice each normal,
	/// one for each incident tetrahedron
	bool buildFaceNormals(void) {

	  faceNormals = new vec3[numTets*4];

	  ivec4 face;
	  vec3 normal, centVec;
	  // for each tet
	  for (GLuint i = 0; i < numTets; ++i) {
		// for each face
		for (GLuint j = 0; j < 4; ++j) {

		  // retrieve verts of splitting face, face[3] is opposite vertex
		  face[0] = tetList[i][MOD4(0, j)];
		  face[1] = tetList[i][MOD4(1, j)];
		  face[2] = tetList[i][MOD4(2, j)];
		  face[3] = tetList[i][MOD4(3, j)];

		  // compute normal of splitting face
		  normal = ((vertList[face[1]] - vertList[face[0]]).xyz() %
					(vertList[face[2]] - vertList[face[0]]).xyz());
		  normal.normalize();

		  // make sure it is pointing towards current Tet
		  centVec = vertList[face[3]].xyz() - vertList[face[0]].xyz();
		  centVec.normalize();

		  // if not pointing inside, invert normal
		  if ((normal ^ centVec) < 0)
			normal *= -1.0;

		  faceNormals[i*4 + j] = normal;
		}
	  }
	  return 1;
	}
	

};

#endif
