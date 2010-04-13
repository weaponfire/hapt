/**
 *
 *    PTINT -- isoSurfaces Editing
 *
 * Marroquim, Ricardo -- April, 2009
 *
 **/

/**
 *   isoSurfaces : defines a class for iso-surfaces (ISO) editing
 *                      using only OpenGL (without glut functions).
 *
 * C++ header.
 *
 */

/// --------------------------------   Definitions   ------------------------------------

#ifndef _ISOSURFACES_H_
#define _ISOSURFACES_H_

extern "C" {
#include <GL/gl.h> // OpenGL library
}

#include <map>
#include <string>

#include <iostream>
#include <fstream>

using std::map;

using std::ifstream;
using std::ofstream;
using std::endl;

extern
void glWrite(GLdouble x, GLdouble y, const char *str);

/// -----------------------------   Iso-Surfaces   ----------------------------------

/// IsoSurfaces (ISO) handle class
/// @template real number type (float, double, etc.)
/// @template natural number type (short, unsigned, long, etc.)
template< class real, class natural >
class isoSurfaces {

public:

	typedef vec< 2, real > vec2;

	typedef typename map< natural, real >::iterator mapIt;

	/// Constructor -- instantiate null-iso
	isoSurfaces(vec2* _iso = NULL, real _bt = 1.0) : iso(_iso),
		numIsos(4),
		minBrightness(0.0), maxBrightness(4.0), ppoint(0),
		controlPoints(), minOrthoSize(-0.2), maxOrthoSize(1.3),
		winWidth(400), winHeight(300),
		stepBrightness(0), isoName() {

		unSetPickedPoint();

		/// Initializing 4 control points (4 iso-surfaces)
		controlPoints[ 0 ] = 0.0;
 		controlPoints[ 1 ] = 0.0;
 		controlPoints[ 2 ] = 0.0;
 		controlPoints[ 3 ] = 0.0;

		stepBrightness = 1.0 / (GLdouble)(maxBrightness - minBrightness);
	}

	/// Destructor
	~isoSurfaces() {
		controlPoints.clear();
	}

	/// Set functions
	void setOrtho(const GLdouble& _minOrtho, const GLdouble& _maxOrtho) {
		minOrthoSize = _minOrtho;
		maxOrthoSize = _maxOrtho;
	}
	void setWindow(const GLsizei& _winW, const GLsizei& _winH) {
		winWidth = _winW;
		winHeight = _winH;
	}
	void setISOName(string _isoName) { isoName = _isoName; }
	void unSetPickedPoint(void) { ppoint = numIsos+3; }

	/// Get current diffuse intensity
	/// @return diffuse
	real getDiffuse(void) { return iso[numIsos+0][0]; }

	/// Get current specular intensity
	/// @return specular
	real getSpecular(void) { return iso[numIsos+1][0]; }

	/// Get current specular intensity
	/// @return specular
	real getAmbient(void) { return iso[numIsos+2][0]; }

	/// Get ISO name
	/// @return file path (ISO name)
	string getISOName(void) { return isoName; }


	/// Update picked point related value
	/// @arg x, y mouse position when changing
	void updatePickedPoint(int x, int y) {

	  if (ppoint == numIsos+3) return; /// no point picked

	  if (ppoint < numIsos) { /// Control point
		real yP = yOrtho(y);
		if (yP > 1.0) yP = 1.0;
		if (yP < 0.0) yP = 0.0;
		real xP = xOrtho(x);
		if (xP > 1.0) xP = 1.0;
		if (xP < 0.0) xP = 0.0;
		iso[ ppoint ][0] = xP;
		iso[ ppoint ][1] = yP;
	  }
	  else if (ppoint == numIsos) { /// Diffuse intensity
		real xP = xOrtho(x);
		if (xP > 1.0) xP = 1.0;
		if (xP < 0.0) xP = 0.0;
		iso[ ppoint ][0] = (real)( xP * (maxBrightness-minBrightness) );
	  }
	  else if (ppoint == numIsos + 1) { /// Specular intensity
		real xP = xOrtho(x);
		if (xP > 1.0) xP = 1.0;
		if (xP < 0.0) xP = 0.0;
		iso[ ppoint ][0] = (real)( xP * (maxBrightness-minBrightness) );
	  }
	  else { /// Ambient intensity
		real xP = xOrtho(x);
		if (xP > 1.0) xP = 1.0;
		if (xP < 0.0) xP = 0.0;
		iso[ ppoint ][0] = (real)( xP * (maxBrightness-minBrightness) );

	  }
	}

	/// Release picked point
	void releasePickedPoint(int x, int y) {
		if( ppoint >= numIsos )
			unSetPickedPoint();
	}

	/// OpenGL Setup
	void glSetup() {
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f); ///< Fixed white background
	}

	void animate(GLfloat frame) {
	  iso[0][0] = (real)frame;
	}

	/// Picking function
	/// @arg x, y mouse position when hit
	void pick(int x, int y) {

	  GLsizei xP, yP;


		for (mapIt it = controlPoints.begin(); it != controlPoints.end(); ++it) {
		  
			xP = xScreen(iso[(*it).first][0]);
			yP = yScreen(iso[(*it).first][1]);

			if ( (( y >= yP - 5 ) && ( y <= yP + 5 ) )
			     && ( x >= xP - 5 ) && ( x <= xP + 5 ) ) { /// Control Point picked

				ppoint = (*it).first;
				return;

			}

		}

		xP = xScreen(iso[ numIsos+0 ][0] * stepBrightness);

		if ( ( y >= yScreen(-0.04) - 5) && (y <= yScreen(-0.04) + 5)
		     && (x >= xScreen(0.0) - 5) && (x <= xScreen(1.0) - 5) ) { /// Diffuse picked

			ppoint = numIsos;
			return;

		}

		xP = xScreen(iso[ numIsos+1 ][0] * stepBrightness);

		if ( ( y >= yScreen(-0.08) - 5) && (y <= yScreen(-0.08) + 5)
		     && (x >= xScreen(0.0) - 5) && (x <= xScreen(1.0) - 5) ) { /// Specular picked

			ppoint = numIsos+1;
			return;

		}

		xP = xScreen(iso[ numIsos+2 ][0] * stepBrightness);


		if ( ( y >= yScreen(-0.12) - 5) && (y <= yScreen(-0.12) + 5)
		     && (x >= xScreen(0.0) - 5) && (x <= xScreen(1.0) - 5) ) { /// Ambient picked

			ppoint = numIsos+2;
			return;

		}

		unSetPickedPoint();

	}


	/// Draw isos informations
	void draw(void) {

		if (!iso) return;

		glBegin(GL_LINES); /// Diffuse line
		glColor3f(1.0, 1.0, 1.0); glVertex2d(0.0, -0.04);
		glColor3f(0.0, 0.0, 0.0); glVertex2d(1.0, -0.04);
		glEnd();

		glBegin(GL_LINES); /// Specular line
		glColor3f(1.0, 1.0, 1.0); glVertex2d(0.0, -0.08);
		glColor3f(0.0, 0.0, 0.0); glVertex2d(1.0, -0.08);
		glEnd();

		glBegin(GL_LINES); /// Ambient line
		glColor3f(1.0, 1.0, 1.0); glVertex2d(0.0, -0.12);
		glColor3f(0.0, 0.0, 0.0); glVertex2d(1.0, -0.12);
		glEnd();

		for (mapIt it = controlPoints.begin(); it != controlPoints.end(); ++it) {

			if ((*it).first == ppoint) {
				glColor3d(0.7, 0.3, 0.1);
				glPointSize(8.0);
			} else {
				glColor3d(0.0, 0.0, 0.0);
				glPointSize(4.0);
			}

			glBegin(GL_POINTS);
			glVertex2d( iso[(*it).first][0], iso[(*it).first][1] ); /// Draw Control point
			glEnd();

		}

		// Light parameters sliders
		for (int i = 0; i < 3; ++i) {

		  if( ppoint == numIsos + i ) {
			glColor3d(0.7, 0.3, 0.1);
			glPointSize(8.0);
		  } else {
			glColor3d(0.0, 0.0, 0.0);
			glPointSize(4.0);
		  }

		  GLfloat ypos = -0.04 * (GLfloat)(i+1);
		  glBegin(GL_POINTS);
		  glVertex2d( iso[ numIsos+i ][0] * stepBrightness, ypos);
		  glEnd();
		}

		glPointSize(1.0);

		glBegin(GL_LINES); /// Draw Axis
		glColor3d(0.0, 0.0, 0.0);
		glVertex2d(0.0, 0.0); glVertex2d(0.0, 1.1);
		glVertex2d(0.0, 0.0); glVertex2d(1.1, 0.0);
		glVertex2d(0.0, 1.1); glVertex2d(0.02, 1.08);
		glVertex2d(0.0, 1.1); glVertex2d(-0.02, 1.08);
		glVertex2d(1.1, 0.0); glVertex2d(1.08, -0.02);
		glVertex2d(1.1, 0.0); glVertex2d(1.08, 0.02);
		glEnd();

		glEnable(GL_LINE_STIPPLE); glLineStipple(4, 0xAAAA);
		glBegin(GL_LINES);
		glVertex2d(0.0, 1.0); glVertex2d(1.0, 1.0);
		glVertex2d(1.0, 0.0); glVertex2d(1.0, 1.0);
		glEnd();

		glDisable(GL_LINE_STIPPLE);

		glColor3d(0.0, 0.0, 0.0);
		glWrite(0.30, 1.2, "File:"); /// Write texts
		glWrite(0.45, 1.2, (char*)isoName.c_str());
		glWrite(-0.05, 1.14, "Alpha");
		glWrite(1.12, -0.01, "Scalar");
		glWrite(1.04, -0.05, "Diffuse");
		glWrite(1.04, -0.09, "Specular");
		glWrite(1.04, -0.13, "Ambient");

		if (ppoint != numIsos+3) {

			glColor3d(0.7, 0.3, 0.1);

			char str[256];

			if (ppoint == numIsos) { /// Diffuse point

				sprintf(str, "%.2lf", iso[ ppoint ][0] );
				glWrite( (iso[ ppoint ][0] - 0.5) * stepBrightness, -0.02, str );

			} 
			else if (ppoint == numIsos + 1) { /// Specular point

				sprintf(str, "%.2lf", iso[ ppoint ][0] );
				glWrite( (iso[ ppoint ][0] - 0.5) * stepBrightness, -0.06, str );

			}
			else if (ppoint == numIsos + 2) { /// Ambient point

				sprintf(str, "%.2lf", iso[ ppoint ][0] );
				glWrite( (iso[ ppoint ][0] - 0.5) * stepBrightness, -0.10, str );

			}
			else { /// Control point

				sprintf(str, "%.2lf", iso[ ppoint ][1] );
				glWrite( -0.12, iso[ppoint][1], str);

				sprintf(str, "%d", ppoint );
				glWrite( (ppoint - 0.5 ), -0.06, str);

			}
		}

	}

private:


	/// Convert x, y ortho to/from screen
	real xOrtho(GLsizei x) {
		return (real)( ((x/(real)winWidth) * (maxOrthoSize-minOrthoSize)) + minOrthoSize );
	}
	real yOrtho(GLsizei y) {
		return (real)(((((real)winHeight - y)/(real)winHeight) * (maxOrthoSize-minOrthoSize)) + minOrthoSize);
	}
	GLsizei xScreen(real x) {
		return (GLsizei)(((x - minOrthoSize)*(real)winWidth)/(maxOrthoSize-minOrthoSize));
	}
	GLsizei yScreen(real y) {
		return winHeight - (GLsizei)(((y - minOrthoSize)*(real)winHeight)/(maxOrthoSize-minOrthoSize));
	}

	vec2 *iso; ///< Pointer to iso-surface values

	natural numIsos; ///< Number of Isos

	real minBrightness, maxBrightness; ///< Brightness term

	natural ppoint; ///< Picked point = [0, numIsos-1]: control point id; numIsos: brightness; numIsos+1: no point picked

	map< natural, real > controlPoints; ///< Vector of control points

	GLdouble minOrthoSize, maxOrthoSize; /// Minimum and maximum ortho size
	GLsizei winWidth, winHeight; ///< Width x Height pixel resolution

	real stepBrightness; ///< Step distance in color and brightness

	string isoName; ///< Iso-Surfaces (ISO) name

};

#endif
