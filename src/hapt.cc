/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 * Maximo, Andre -- Mar, 2009
 *
 */

/**
 *   Main
 *
 * C++ code.
 *
 */

/// ----------------------------------   Definitions   ------------------------------------

#include "hapt.h"

/// -----------------------------------   Functions   -------------------------------------

/// Main

int main(int argc, char** argv) {

	glAppInit(argc, argv);

	if ( !app.setup(argc, argv) )
		return 1;

	glISOSetup();

	glTFSetup();

	glPTSetup();

	if ( !app.glSetup() )
		return 1;

	glLoop();

	return 0;

}
