///
/// Hammer to convert all p_* binary files (turbulent_jet) to a single tvvd binary file (down sampled)
///

#include <cmath>
#include <cfloat>
#include <cassert>

#include <iostream>
#include <fstream>
#include <sstream>

#include <string>

#define FIRST_STEP 1
#define LAST_STEP 150

using namespace std;

typedef unsigned int uint;

typedef float vertex [3];

typedef uint tetrahedron [4];

static const char helpString[] = "Usage: ./p2tvvd output.tvvd\n\n  Converts all p_* binary files in turbulent_jet/ to output.tvvd file\n";

static const string basePath = "./turbulent_jet/p_";

static const uint numFrames = LAST_STEP - FIRST_STEP + 1;

static const float mega = 1024 * 1024;

/// Main

int main(int argc, char** argv) {

	if( argc < 2 ) { cout << helpString << endl; return 0; }

	string fileName;
	ifstream in;

	cout << "*******************************************************************************" << endl;

	cout << "* Opening binary file for writing " << argv[1] << " ..." << flush;
	ofstream out( argv[1], ios::binary );
	if( out.fail() ) { cerr << "! Error opening " << argv[1] << endl; return 0; }
	cout << "done!" << endl;

	uint firstHeader[5];
	float *scalarField;
	float *downScalarField;

	uint dimX, dimY, dimZ, dimZY, sfSize;
	uint ddimX, ddimY, ddimZ, ddimZY, dsfSize;

	uint id[8];
	uint downId, tetId;

	uint numVerts, numTets;

	float maxDim, step, minS, maxS;

	vertex *vertices;
	tetrahedron *tetrahedra;

	for (uint i = FIRST_STEP; i <= LAST_STEP; ++i) {

		ostringstream ss;
		if( i < 10 ) ss << "0" << i;
		else ss << i;

		fileName = basePath + ss.str();

		cout << "*******************************************************************************" << endl;

		cout << "* Opening binary file for reading " << fileName << " ..." << flush;
		in.open( fileName.c_str(), ios::binary );
		if( in.fail() ) { cerr << "! Error opening " << fileName << endl; return 0; }
		cout << "done!" << endl;

		cout << "*******************************************************************************" << endl;

		uint header[5];

		cout << "> Reading header from " << argv[1] << " : " << sizeof(uint)*5 << " B" << endl;
		in.read( (char*)&header[0], sizeof(uint) * 5 );
		if( in.fail() ) { cerr << "! Error reading " << argv[1] << endl; return 0; }

		cout << "+ Fixing header..." << flush;
		for (uint j = 0; j < 5; ++j) // file is in Big Endian format
			header[j] >>= 24;
		cout << "done! " << header[0] << " " << header[1] << " " << header[2] << " "
		     << header[3] << " " << header[4] << endl;

		if( i == FIRST_STEP ) {

			for (uint j = 1; j < 4; ++j)
				firstHeader[j] = header[j];

			dimX = header[3]; dimY = header[2]; dimZ = header[1];
			dimZY = dimZ * dimY;
			sfSize = dimX * dimY * dimZ;

			ddimX = dimX / 2; ddimY = dimY / 2; ddimZ = dimZ / 2;
			ddimZY = ddimZ * ddimY;
			dsfSize = ddimX * ddimY * ddimZ;

			numVerts = ddimX * ddimY * ddimZ;
			numTets = (ddimX-1) * (ddimY-1) * (ddimZ-1) * 5;

			maxDim = ( ddimX > ddimY ) ? ( ( ddimX > ddimZ ) ? ddimX : ddimZ ) : ( ( ddimY > ddimZ ) ? ddimY : ddimZ );
			step = 1.0 / (float)maxDim;

			cout << "+ Down sampling " << ddimX << "x" << ddimY << "x" << ddimZ << endl;

			cout << "+ Max Dim " << maxDim << " step " << step << " dimZY " << dimZY << endl;

			cout << "* Allocating memory for scalarField..." << flush;
			scalarField = new float[ sfSize ];
			if( !scalarField ) { cerr << "! Error allocating memory for scalarField" << endl; return 0; }
			cout << "done! " << sfSize*sizeof(float) / mega << " MB" << endl;

			cout << "* Allocating memory for down sampling scalarField..." << flush;
			downScalarField = new float [ dsfSize ];
			if( !downScalarField ) { cerr << "! Error allocating memory for downScalarField" << endl; return 0; }
			cout << "done! " << dsfSize*sizeof(float) / mega << " MB" << endl;

			cout << "* Allocating memory for vertices..." << flush;
			vertices = new vertex[ numVerts ];
			cout << "done! " << numVerts*sizeof(vertex) / mega << " MB" << endl;

			cout << "* Allocating memory for tetrahedra (" << numTets << " tets)..." << flush;
			tetrahedra = new tetrahedron[ numTets ];
			cout << "done! " << numTets*sizeof(tetrahedron) / mega << " MB" << endl;

			cout << "$ Computing vertices..." << flush;

			for (uint x = 0; x < ddimX; x += 1) {

				for (uint y = 0; y < ddimY; y += 1) {

					for (uint z = 0; z < ddimZ; z += 1) {

						id[0] = z + y * ddimZ + x * ddimZY;

						vertices[ id[0] ][0] = x * step;
						vertices[ id[0] ][1] = y * step;
						vertices[ id[0] ][2] = z * step;

					}

				}

			}

			cout << "done!" << endl;

			cout << "$ Computing tetrahedra..." << flush;

			tetId = 0;

			for (uint x = 0; x < ddimX-1; x += 1) {

				for (uint y = 0; y < ddimY-1; y += 1) {

					for (uint z = 0; z < ddimZ-1; z += 1) {

						id[0] =     z +     y * ddimZ +     x * ddimZY;
						id[1] = (z+1) +     y * ddimZ +     x * ddimZY;
						id[2] = (z+1) + (y+1) * ddimZ +     x * ddimZY;
						id[3] =     z + (y+1) * ddimZ +     x * ddimZY;
						id[4] =     z +     y * ddimZ + (x+1) * ddimZY;
						id[5] = (z+1) +     y * ddimZ + (x+1) * ddimZY;
						id[6] = (z+1) + (y+1) * ddimZ + (x+1) * ddimZY;
						id[7] =     z + (y+1) * ddimZ + (x+1) * ddimZY;

						tetrahedra[ tetId ][0] = id[0]; tetrahedra[ tetId ][1] = id[1];
						tetrahedra[ tetId ][2] = id[2]; tetrahedra[ tetId ][3] = id[5];

						tetId++;

						tetrahedra[ tetId ][0] = id[0]; tetrahedra[ tetId ][1] = id[2];
						tetrahedra[ tetId ][2] = id[3]; tetrahedra[ tetId ][3] = id[7];

						tetId++;

						tetrahedra[ tetId ][0] = id[0]; tetrahedra[ tetId ][1] = id[2];
						tetrahedra[ tetId ][2] = id[4]; tetrahedra[ tetId ][3] = id[5];

						tetId++;

						tetrahedra[ tetId ][0] = id[2]; tetrahedra[ tetId ][1] = id[4];
						tetrahedra[ tetId ][2] = id[5]; tetrahedra[ tetId ][3] = id[7];

						tetId++;

						tetrahedra[ tetId ][0] = id[2]; tetrahedra[ tetId ][1] = id[5];
						tetrahedra[ tetId ][2] = id[6]; tetrahedra[ tetId ][3] = id[7];

						tetId++;

					}

				}

			}
			cout << "done!" << endl;

			cout << "< Writing header to " << argv[1] << " : " << sizeof(uint)*3 << " B" << endl;
			out.write( (char*)&numFrames, sizeof(uint) );
			out.write( (char*)&numVerts, sizeof(uint) );
			out.write( (char*)&numTets, sizeof(uint) );
			if( out.fail() ) { cerr << "! Error writing " << argv[1] << endl; return 0; }

			cout << "< Writing vertices to " << argv[1] << " : " << sizeof(vertex)*numVerts / mega << " MB" << endl;
			for (uint j = 0; j < numVerts; ++j) {

				out.write( (char*)&vertices[j][0], sizeof(vertex) );

				if( out.fail() ) { cerr << "! Error writing " << argv[1] << endl; return 0; }

			}

			cout << "< Writing tetrahedra to " << argv[1] << " : " << sizeof(tetrahedra)*numTets / mega << " MB" << endl;
			for (uint j = 0; j < numTets; ++j) {

				out.write( (char*)&tetrahedra[j][0], sizeof(tetrahedron) );

				if( out.fail() ) { cerr << "! Error writing " << argv[1] << endl; return 0; }

			}

			cout << "* Deallocating vertices memory..." << flush;
			delete [] vertices;
			cout << "done! " << numVerts*sizeof(vertex) / mega << " MB" << endl;

			cout << "* Deallocating tetrahedra memory..." << flush;
			delete [] tetrahedra;
			cout << "done! " << numTets*sizeof(tetrahedron) / mega << " MB" << endl;
	
		} else {

			if( header[1] != firstHeader[1] || header[2] != firstHeader[2] || header[3] != firstHeader[3] ) {

				cerr << "! File " << fileName << " has different header dimension than the first file";
				return 0;

			}

		}

		cout << "> Reading " << dimX << "x" << dimY << "x" << dimZ << " from " << fileName << " ..." << flush;
		in.read( (char*)&scalarField[0], sizeof(float) * sfSize );
		if( in.fail() ) { cerr << "! Error reading " << fileName << endl; return 0; }
		cout << "done! " << sfSize << " voxels" << endl;

		cout << "* Closing " << fileName << flush;
		in.close();
		cout << endl;

		cout << "* Fixing scalarField..." << flush;
		minS = FLT_MAX; maxS = FLT_MIN;
		for (uint j = 0; j < sfSize; ++j) { // file is in Big Endian format
			uint  _s = *(uint*)&scalarField[j];
			uint _s1 =     (_s) & 0x000000FF;
			uint _s2 =  (_s>>8) & 0x000000FF;
			uint _s3 = (_s>>16) & 0x000000FF;
			uint _s4 = (_s>>24) & 0x000000FF;
			uint _sA = (_s1<<24) + (_s2<<16) + (_s3<<8) + (_s4);
			float _f = *(float*)&_sA;
			if( _f < minS ) minS = _f;
			if( _f > maxS ) maxS = _f;
			scalarField[j] = _f;
		}
		cout << "done! min " << minS << " max " << maxS << endl;

		cout << "$ Down sampling scalarField..." << flush;

		for (uint x = 0; x < dimX-1; x += 2) {

			for (uint y = 0; y < dimY-1; y += 2) {

				for (uint z = 0; z < dimZ-1; z += 2) {

					downId = (z/2) + (y/2) * ddimZ + (x/2) * ddimZY;

					id[0] =     z +     y * dimZ +     x * dimZY;
					id[1] = (z+1) +     y * dimZ +     x * dimZY;
					id[2] = (z+1) + (y+1) * dimZ +     x * dimZY;
					id[3] =     z + (y+1) * dimZ +     x * dimZY;

					id[4] =     z +     y * dimZ + (x+1) * dimZY;
					id[5] = (z+1) +     y * dimZ + (x+1) * dimZY;
					id[6] = (z+1) + (y+1) * dimZ + (x+1) * dimZY;
					id[7] =     z + (y+1) * dimZ + (x+1) * dimZY;

					downScalarField[ downId ] = 0.0;
					for (uint j = 0; j < 8; j++)
						downScalarField[ downId ] += scalarField[ id[j] ];

					downScalarField[ downId ] /= 8.0f;

				}

			}

		}

		cout << "done! " << ddimX << "x" << ddimY << "x" << ddimZ << endl;

		cout << "< Writing downScalarField of frame " << i << " to " << argv[1] << " : " << sizeof(float)*dsfSize / mega << " MB" << endl;

		out.write( (char*)&downScalarField[0], sizeof(float) * dsfSize );

		if( out.fail() ) { cerr << "! Error writing " << argv[1] << endl; return 0; }

	}

	cout << "*******************************************************************************" << endl;

	cout << "* Closing " << argv[1] << flush;
	out.close();
	cout << endl;

	cout << "*******************************************************************************" << endl;

	cout << "* Deallocating scalarField memory..." << flush;
	delete [] scalarField;
	cout << "done! " << sfSize*sizeof(float) / mega << " MB" << endl;

	cout << "* Deallocating downScalarField memory..." << flush;
	delete [] downScalarField;
	cout << "done! " << dsfSize*sizeof(float) / mega << " MB" << endl;

	return 1;

}

/**
 * Legacy Code


		for (uint z = 0; z < dimZ-1; z += 2) {

			for (uint y = 0; y < dimY-1; y += 2) {

				for (uint x = 0; x < dimX-1; x += 2) {

					id[ 0] = (x-1) + (y-1) * dimX + (z-1) * dimXY;
					id[ 1] =     x + (y-1) * dimX + (z-1) * dimXY;
					id[ 2] = (x+1) + (y-1) * dimX + (z-1) * dimXY;
					id[ 3] = (x-1) +     y * dimX + (z-1) * dimXY;
					id[ 4] =     x +     y * dimX + (z-1) * dimXY;
					id[ 5] = (x+1) +     y * dimX + (z-1) * dimXY;
					id[ 6] = (x-1) + (y+1) * dimX + (z-1) * dimXY;
					id[ 7] =     x + (y+1) * dimX + (z-1) * dimXY;
					id[ 8] = (x+1) + (y+1) * dimX + (z-1) * dimXY;

					id[ 9] = (x-1) + (y-1) * dimX +     z * dimXY;
					id[10] =     x + (y-1) * dimX +     z * dimXY;
					id[11] = (x+1) + (y-1) * dimX +     z * dimXY;
					id[12] = (x-1) +     y * dimX +     z * dimXY;
					id[13] =     x +     y * dimX +     z * dimXY;
					id[14] = (x+1) +     y * dimX +     z * dimXY;
					id[15] = (x-1) + (y+1) * dimX +     z * dimXY;
					id[16] =     x + (y+1) * dimX +     z * dimXY;
					id[17] = (x+1) + (y+1) * dimX +     z * dimXY;

					id[18] = (x-1) + (y-1) * dimX + (z+1) * dimXY;
					id[19] =     x + (y-1) * dimX + (z+1) * dimXY;
					id[20] = (x+1) + (y-1) * dimX + (z+1) * dimXY;
					id[21] = (x-1) +     y * dimX + (z+1) * dimXY;
					id[22] =     x +     y * dimX + (z+1) * dimXY;
					id[23] = (x+1) +     y * dimX + (z+1) * dimXY;
					id[24] = (x-1) + (y+1) * dimX + (z+1) * dimXY;
					id[25] =     x + (y+1) * dimX + (z+1) * dimXY;
					id[26] = (x+1) + (y+1) * dimX + (z+1) * dimXY;

					id[0] =     x +     y * ddimX +     z * ddimXY;
					id[1] = (x+1) +     y * ddimX +     z * ddimXY;
					id[2] = (x+1) + (y+1) * ddimX +     z * ddimXY;
					id[3] =     x + (y+1) * ddimX +     z * ddimXY;
					id[4] =     x +     y * ddimX + (z+1) * ddimXY;
					id[5] = (x+1) +     y * ddimX + (z+1) * ddimXY;
					id[6] = (x+1) + (y+1) * ddimX + (z+1) * ddimXY;
					id[7] =     x + (y+1) * ddimX + (z+1) * ddimXY;

					downScalarField[ downId ] = 0.0;

					for (uint j = 0; j < 8; ++j)
						downScalarField[ downId ] += scalarField[ id[j] ];

					downScalarField[ downId ] /= 8.0f;

					downId++;

				}

			}

		}


*/
