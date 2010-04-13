///
/// Hammer to convert p_* binary file (turbulent_jet) to off file
///

#include <cmath>
#include <cfloat>

#include <iostream>
#include <fstream>

using namespace std;

typedef unsigned int uint;

typedef float vertex [4];

typedef uint tetrahedron [4];

static const char helpString[] = "Usage: ./p2off p_* *.off\n\n  Converts p_* binary file to *.off file\n";

/// Main

int main(int argc, char** argv) {

	if( argc < 3 ) { cout << helpString << endl; return 0; }

	cout << "* Opening binary file for reading " << argv[1] << " ..." << flush;
	ifstream in( argv[1], ios::binary );
	if( in.fail() ) { cerr << "Error opening " << argv[1] << endl; return 0; }
	cout << "done!" << endl;

	uint header[5];

	cout << "> Reading header from " << argv[1] << " : " << sizeof(uint)*5 << " B" << endl;
	in.read( (char*)&header[0], sizeof(uint) * 5 );
	if( in.fail() ) { cerr << "Error reading " << argv[1] << endl; return 0; }

	cout << "+ Fixing header..." << flush;
	for (uint i=0; i<5; ++i) /// File is in Big Endian format
		header[i] >>= 24;
	cout << "done! " << header[0] << " " << header[1] << " " << header[2] << " "
	     << header[3] << " " << header[4] << endl;

	uint dimX, dimY, dimZ, sfSize; dimX = dimY = dimZ = sfSize;

	dimX = header[3]; dimY = header[2]; dimZ = header[1]; sfSize = dimX * dimY * dimZ;

	cout << "* Allocating memory for scalarField..." << flush;
	float *scalarField = new float[ sfSize ];
	if( !scalarField ) { cerr << "Error allocating memory for scalarField" << endl; return 0; }
	cout << "done! " << (int)ceil( sfSize*sizeof(float) / (int)pow(2.0,20) ) << " MB" << endl;

	cout << "> Reading " << dimX << "x" << dimY << "x" << dimZ << " from " << argv[1] << " ..." << flush;
	in.read( (char*)&scalarField[0], sizeof(float) * sfSize );
	if( in.fail() ) { cerr << "Error reading " << argv[1] << endl; return 0; }
	cout << "done! " << sfSize << " voxels" << endl;

	in.close();

	cout << "* Fixing scalarField..." << flush;
	float minS, maxS; minS = FLT_MAX; maxS = FLT_MIN;
	for (uint i=0; i<sfSize; ++i) { /// File is in Big Endian format
		uint _s = *(uint*)&scalarField[i];
		uint _s1 =     (_s) & 0x000000FF;
		uint _s2 =  (_s>>8) & 0x000000FF;
		uint _s3 = (_s>>16) & 0x000000FF;
		uint _s4 = (_s>>24) & 0x000000FF;
		uint _sA = (_s1<<24) + (_s2<<16) + (_s3<<8) + (_s4);
		float _f = *(float*)&_sA;
		if( _f < minS ) minS = _f;
		if( _f > maxS ) maxS = _f;
		scalarField[i] = _f;
	}
	cout << "done! min " << minS << " max " << maxS << endl;

	cout << "* Allocating memory for down sampling scalarField..." << flush;
	float *downScalarField = new float [ sfSize/8 ];
	if( !downScalarField ) { cerr << "Error allocating memory for downScalarField" << endl; return 0; }
	cout << "done! " << (int)ceil( (sfSize/8)*sizeof(float) / (int)pow(2.0,20) ) << " MB" << endl;

	uint id[27];
	uint dimXY = dimX * dimY;
	uint downId = 0;

	cout << "$ Down sampling scalarField..." << flush;

	for (uint z = 1; z < dimZ-1; z += 2) {

		for (uint y = 1; y < dimY-1; y += 2) {

			for (uint x = 1; x < dimX-1; x += 2) {

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

				downScalarField[ downId ] = 0.0;
				for (uint i = 0; i < 27; ++i)
					downScalarField[ downId ] += scalarField[ id[i] ] / 27.0f;
				downId++;

			}

		}

	}

	delete [] scalarField;

	scalarField = downScalarField;
	dimX /= 2;
	dimY /= 2;
	dimZ /= 2;
	dimXY = dimX * dimY;

	cout << "done! " << dimX << "x" << dimY << "x" << dimZ << endl;

	uint numVerts = dimX * dimY * dimZ;

	float maxDim = ( dimX > dimY ) ? ( ( dimX > dimZ ) ? dimX : dimZ ) : ( ( dimY > dimZ ) ? dimY : dimZ );
	float step = 1.0 / (float)maxDim;

	cout << "+ Max Dim " << maxDim << " step " << step << " dimXY " << dimXY << endl;

	float tetScalar[8];

	cout << "* Allocating memory for vertices..." << flush;
	vertex *vertices = new vertex[ numVerts ];
	cout << "done! " << (int)ceil( numVerts*sizeof(vertex) / (int)pow(2.0,20) ) << " MB" << endl;

	cout << "$ Computing vertices..." << flush;
	for (uint z = 0; z < dimZ-1; z += 2) {

		for (uint y = 0; y < dimY-1; y += 2) {

			for (uint x = 0; x < dimX-1; x += 2) {

				id[0] =     x +     y * dimX +     z * dimXY;
				id[1] = (x+1) +     y * dimX +     z * dimXY;
				id[2] = (x+1) + (y+1) * dimX +     z * dimXY;
				id[3] =     x + (y+1) * dimX +     z * dimXY;
				id[4] =     x +     y * dimX + (z+1) * dimXY;
				id[5] = (x+1) +     y * dimX + (z+1) * dimXY;
				id[6] = (x+1) + (y+1) * dimX + (z+1) * dimXY;
				id[7] =     x + (y+1) * dimX + (z+1) * dimXY;

				vertices[ id[0] ][0] =     x * step; vertices[ id[0] ][1] =     y * step; vertices[ id[0] ][2] =     z * step;
				vertices[ id[1] ][0] = (x+1) * step; vertices[ id[1] ][1] =     y * step; vertices[ id[1] ][2] =     z * step;
				vertices[ id[2] ][0] = (x+1) * step; vertices[ id[2] ][1] = (y+1) * step; vertices[ id[2] ][2] =     z * step;
				vertices[ id[3] ][0] =     x * step; vertices[ id[3] ][1] = (y+1) * step; vertices[ id[3] ][2] =     z * step;
				vertices[ id[4] ][0] =     x * step; vertices[ id[4] ][1] =     y * step; vertices[ id[4] ][2] = (z+1) * step;
				vertices[ id[5] ][0] = (x+1) * step; vertices[ id[5] ][1] =     y * step; vertices[ id[5] ][2] = (z+1) * step;
				vertices[ id[6] ][0] = (x+1) * step; vertices[ id[6] ][1] = (y+1) * step; vertices[ id[6] ][2] = (z+1) * step;
				vertices[ id[7] ][0] =     x * step; vertices[ id[7] ][1] = (y+1) * step; vertices[ id[7] ][2] = (z+1) * step;

				for (uint i=0; i<8; ++i) vertices[ id[i] ][3] = scalarField[ id[i] ];

			}

		}

	}
	cout << "done!" << endl;

	cout << "* Deallocating scalarField memory..." << flush;
	delete [] scalarField;
	cout << "done! -" << (int)ceil( sfSize*sizeof(float) / (int)pow(2.0,20) ) << " MB" << endl;

	uint numTets = (dimX-1)*(dimY-1)*(dimZ-1)*5;

	cout << "* Allocating memory for tetrahedra (" << numTets << " tets)..." << flush;
	tetrahedron *tetrahedra = new tetrahedron[ numTets ];
	cout << "done! " << (int)ceil( numTets*sizeof(tetrahedron) / (int)pow(2.0,20) ) << " MB" << endl;

	cout << "$ Computing tetrahedra..." << flush;
	uint tetId = 0;
	for (uint z = 0; z < dimZ-1; z += 1) {

		for (uint y = 0; y < dimY-1; y += 1) {

			for (uint x = 0; x < dimX-1; x += 1) {

				id[0] =     x +     y * dimX +     z * dimXY;
				id[1] = (x+1) +     y * dimX +     z * dimXY;
				id[2] = (x+1) + (y+1) * dimX +     z * dimXY;
				id[3] =     x + (y+1) * dimX +     z * dimXY;
				id[4] =     x +     y * dimX + (z+1) * dimXY;
				id[5] = (x+1) +     y * dimX + (z+1) * dimXY;
				id[6] = (x+1) + (y+1) * dimX + (z+1) * dimXY;
				id[7] =     x + (y+1) * dimX + (z+1) * dimXY;

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

	cout << "* Opening text file for writing " << argv[2] << " ..." << flush;
	ofstream out( argv[2] );
	if( out.fail() ) { cerr << "Error opening " << argv[2] << endl; return 0; }
	cout << "done!" << endl;

	out << numVerts << " " << numTets << endl;

	for (uint i = 0; i < numVerts; ++i) {

		out << vertices[i][0] << " " << vertices[i][1] << " "
		    << vertices[i][2] << " " << vertices[i][3] << endl;

		if( out.fail() ) { cerr << "Error writing " << argv[2] << endl; return 0; }

	}

	for (uint i = 0; i < numTets; ++i) {

		out << tetrahedra[i][0] << " " << tetrahedra[i][1] << " "
		    << tetrahedra[i][2] << " " << tetrahedra[i][3] << endl;

		if( out.fail() ) { cerr << "Error writing " << argv[2] << endl; return 0; }

	}

	out.close();
	
	return 1;

}
