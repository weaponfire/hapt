/**
 *   Volume TVVD Application
 *
 * Maximo, Andre -- Apr, 2009
 *
 */

/**
 *   appVol : defines a class to setup the volume class using application
 *            input parameters.
 *
 * C++ implementation.
 *
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

	tvvdExt = string(".tvvd");
	tfExt = string(".tf");
	lmtExt = string(".lmt");
	searchDir = string("../tvvd/");

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
			<< "  |_ (x) 'file'" << tvvdExt << " : time-varying volume data file" << endl
			<< "  |_ (-) 'file'" << tfExt << " : transfer function with 256 colors" << endl
			<< "  |_ (-) 'file'" << lmtExt << " : volume limits with maxEdgeLength, maxZ and minZ" << endl
			<< "  Reading from the directory: " << searchDir << endl
			<< "  Files marked by (x) need to exist." << endl
			<< "  If the files marked by (-) does not exist, it will be computed and created." << endl << endl;

		if ( argc != 2 ) throw errHandle(usageErr, ssUsage.str().c_str());

		stringstream ioss;
		string fnTVVD, fnTF, fnLmt;

		ioss << searchDir << argv[1];
		ioss >> volName;

		fnTVVD = volName + tvvdExt;
		fnTF = volName + tfExt;
		fnLmt = volName + lmtExt;

		if (debug) cout << endl << "::: Time :::" << endl << endl;

		/// Reading Volume
		if (debug) cout << "Reading volume : " << flush;
		ctBegin = clock();

		if ( !volume.readTVVD(fnTVVD.c_str()) ) throw errHandle(readErr, fnTVVD.c_str());

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

		/// Normalizing Vertices
		if (debug) cout << "Normalizing scalars : " << flush;
		ctBegin = clock();

		volume.normalizeScalars();

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

		/// Concluding
		if (debug) cout << endl
				<< "Total pre-computation : " << totalTime << " s" << endl
				<< endl
				<< "::: Volume :::" << endl
				<< endl
				<< "# Frames = " << volume.numFrames << endl
				<< "# Vertices = " << volume.numVerts << endl
				<< "# Tetrahedra = " << volume.numTets << endl
				<< "# Memory Size = " << volume.sizeOf() / 1024.0 << " KB " << endl
				<< endl;
		
		return true;

	} catch(errHandle& e) {

		cerr << e;

		return false;

	} catch(...) {

		throw errHandle();

	}

}
