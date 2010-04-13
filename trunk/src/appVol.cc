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
 * C++ implementation.
 *
 */

/**
 * Marroquim, Ricardo -- Apr, 2009
 * included initialization of connectivity structure
 * included face normals structure (to be used with MPVONC)
 */

/// --------------------------------   Definitions   ------------------------------------

#include <ctime>
#include <sstream>

#include "appVol.h"

#include "errHandle.h"

using std::stringstream;
using std::cerr;
using std::cout;
using std::endl;
using std::flush;

/// ----------------------------------   appVol   ------------------------------------

/// Volume Application

/// Constructor
appVol::appVol( bool _d ) : volume(), debug(_d) {

	offExt = string(".off");
	tfExt = string(".tf");
	lmtExt = string(".lmt");
	conExt = string(".con");
	isoExt = string(".iso");
	searchDir = string("tet_offs/");

}

/// Destructor
appVol::~appVol() {

}

/// Volume Application Setup
/// @arg argc main argc
/// @arg argv main argv
/// @return true if it succeed
bool appVol::setup(int& argc, char** argv) {

	try {

		clock_t ctBegin = 0;
		double stepTime = 0.0, totalTime = 0.0;

		stringstream ssUsage;

		if ( !argv ) throw errHandle();

		ssUsage << "Usage: " << argv[0] << " 'file'" << endl << endl
			<< "  Where the following files will be readed: " << endl
			<< "  |_ (x) 'file'" << offExt << " : vertex position and tetrahedra vertex ids" << endl
			<< "  |_ (-) 'file'" << tfExt << " : transfer function with 256 colors" << endl
			<< "  |_ (-) 'file'" << lmtExt << " : volume limits with maxEdgeLength, maxZ and minZ " << endl
			<< "  |_ (-) 'file'" << conExt << " : volume connectivity " << endl
			<< "  Reading from the directory: " << searchDir << endl
			<< "  Files marked by (x) need to exist." << endl
			<< "  If the files marked by (-) does not exist, it will be computed and created.\n";

		if ( argc != 2 ) throw errHandle(usageErr, ssUsage.str().c_str());

		stringstream ioss;
		string fnOff, fnTF, fnLmt, fnCon, fnISO;

		ioss << searchDir << argv[1];
		ioss >> volName;		

		fnOff = volName + offExt;
		fnTF = volName + tfExt;
		fnISO = volName + isoExt;
		fnLmt = volName + lmtExt;
		fnCon = volName + conExt;

		if (debug) cout << endl << "::: Time :::" << endl << endl;

		/// Reading Volume
		if (debug) cout << "Reading volume : " << flush;
		ctBegin = clock();

		if ( !volume.readOff(fnOff.c_str()) ) throw errHandle(readErr, fnOff.c_str());

		stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
		totalTime += stepTime;

		if (debug) cout << stepTime << " s" << endl;

		/// Normalizing Vertices
		if (debug) cout << "Normalizing vertices : " << flush;
		ctBegin = clock();

		volume.normalizeVertices();

		stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
		totalTime += stepTime;

		if (debug) cout << stepTime << " s" << endl;

		/// Reading Transfer Function
		ifstream fileTF( fnTF.c_str() );

		if (fileTF.fail()) {

			if (debug) cout << "Building and writing transfer function : " << flush;
			ctBegin = clock();

			if ( !volume.buildTF() ) throw errHandle(memoryErr);

			if ( !volume.writeTF(fnTF.c_str()) ) throw errHandle(writeErr, fnTF.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		} else {

			if (debug) cout << "Reading transfer function : " << flush;
			ctBegin = clock();

			if ( !volume.readTF(fileTF) ) throw errHandle(readErr, fnTF.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		}

		/// Reading Limits
		ifstream fileLmt( fnLmt.c_str() );

		if (fileLmt.fail()) {

			if (debug) cout << "Building and writing volume limits : " << flush;
			ctBegin = clock();

			volume.findMaxEdgeLength();
			volume.findMaxMinZ();

			if ( !volume.writeLmt(fnLmt.c_str()) ) throw errHandle(writeErr, fnLmt.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		} else {

			if (debug) cout << "Reading volume limits : " << flush;
			ctBegin = clock();

			if ( !volume.readLmt(fileLmt) ) throw errHandle(readErr, fnLmt.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		}

		/// Reading Transfer Function
		ifstream fileISO( fnISO.c_str() );
		if (fileISO.fail()) {

			if (debug) cout << "Building and writing iso-surfaces : " << flush;
			ctBegin = clock();

			if ( !volume.buildISO() ) throw errHandle(memoryErr);

			if ( !volume.writeISO(fnISO.c_str()) ) throw errHandle(writeErr, fnISO.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		} else {

			if (debug) cout << "Reading iso-surfaces : " << flush;
			ctBegin = clock();

			if ( !volume.readISO(fileISO) ) throw errHandle(readErr, fnISO.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		}

		
		/// Reading Connectivity
		ifstream fileCon( fnCon.c_str() );

		if (fileCon.fail()) {

			if (debug) cout << "Building and writing volume connectivy : " << flush;
			ctBegin = clock();

			volume.buildIncid();
			volume.buildCon();

 			if ( !volume.writeCon(fnCon.c_str()) ) throw errHandle(writeErr, fnCon.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		} else {

			if (debug) cout << "Reading volume connectivity : " << flush;
			ctBegin = clock();

			if ( !volume.readCon(fileCon) ) throw errHandle(readErr, fnCon.c_str());

			stepTime = ( clock() - ctBegin ) / (double)CLOCKS_PER_SEC;
			totalTime += stepTime;

			if (debug) cout << stepTime << " s" << endl;

		}

		volume.buildFaceNormals();

		/// Concluding
		if (debug) cout << endl
				<< "Total pre-computation : " << totalTime << " s" << endl
				<< endl
				<< "::: Volume :::" << endl
				<< endl
				<< "# Vertices = " << volume.numVerts << endl
				<< "# Tetrahedra = " << volume.numTets << endl
				<< "# External Faces = " << volume.numExtFaces << endl
				<< "# Memory Size = " << volume.sizeOf() / 1000.0 << " KB " << endl
				<< endl;
		
		return true;

	} catch(errHandle& e) {

		cerr << e;

		return false;

	} catch(...) {

		throw errHandle();

	}

}
