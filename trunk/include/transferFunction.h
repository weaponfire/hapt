/**
 *
 *    PTINT -- Transfer Function Editing
 *
 * Maximo, Andre; Marroquim, Ricardo -- May, 2006
 *
 **/

/**
 *   transferFunction : defines a class for Transfer Function (TF) editing
 *                      using only OpenGL (without glut functions).
 *
 * C++ header.
 *
 */

/// --------------------------------   Definitions   ------------------------------------

#ifndef _TRANSFERFUNCTION_H_
#define _TRANSFERFUNCTION_H_

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

/// -----------------------------   transferFunction   ----------------------------------

/// Transfer Function (TF) handle class
/// @template real number type (float, double, etc.)
/// @template natural number type (short, unsigned, long, etc.)
template< class real, class natural >
class transferFunction {

public:

	typedef vec< 4, real > vec4;

	typedef typename map< natural, real >::iterator mapIt;

	string cpExt;

	/// Constructor -- instantiate null-tf
	transferFunction(vec4* _tf = NULL, real _bt = 1.0) : tf(_tf),
		hueSelection(false), numColors(256), brightness(_bt),
		minBrightness(0.0), maxBrightness(8.0), ppoint(0),
		controlPoints(), minOrthoSize(-0.2), maxOrthoSize(1.3),
		winWidth(400), winHeight(300), stepColor(0),
		stepBrightness(0), tfName() {

		cpExt = string(".cp");

		unSetPickedPoint();

		/// Initializing 5 control points
		controlPoints[ 0 ] = 0.0;
		controlPoints[ numColors/4 ] = 0.25;
		controlPoints[ numColors/2 ] = 0.5;
		controlPoints[ numColors*3/4 ] = 0.75;
		controlPoints[ numColors-1 ] = 1.0;

		stepColor = 1.0 / (GLdouble)numColors;
		stepBrightness = 1.0 / (GLdouble)(maxBrightness - minBrightness);

	}

	/// Destructor
	~transferFunction() {
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
	void setTFName(string _tfName) { tfName = _tfName; }
	void setCPName(string _volName) { cpName = _volName + cpExt; }
	void unSetPickedPoint(void) { ppoint = numColors+1; }

	/// Get current brightness
	/// @return brightness
	real getBrightness(void) { return brightness; }

	/// Get TF name
	/// @return file path (TF name)
	string getTFName(void) { return tfName; }

	/// Get CP name
	/// @return file path (CP name)
	string getCPName(void) { return cpName; }

	/// Write control points file
	/// @return true if it succeed
	bool writeCP(void) {

		ofstream out( cpName.c_str() );

		if( out.fail() ) return false;

		if( !tf ) return false;

		out << controlPoints.size() << endl;

		for (mapIt it = controlPoints.begin(); it != controlPoints.end(); ++it) {

			out << it->first << " " << it->second << endl;

			if( out.fail() ) return false;

		}

		out.close();

		return true;

	}

	/// Read control points file
	/// @return true if it succeed
	bool readCP(void) {

		ifstream in( cpName.c_str() );

		if( in.fail() ) return false;

		controlPoints.clear();

		natural numControlPoints, key;
		real value;

		in >> numControlPoints;

		for (natural i = 0; i < numControlPoints; ++i) {

			in >> key >> value;

			controlPoints[ key ] = value;

			if( in.fail() ) return false;

		}

		in.close();

		return true;

	}

	/// Update picked point related value
	/// @arg x, y mouse position when changing
	void updatePickedPoint(int x, int y) {

		if (ppoint == numColors+1) return;

		if (ppoint < numColors) { /// Control point

			if( hueSelection ) {

				real hS = (yOrtho(y) - 0.2) / 0.8;
				if( hS > 1.0 ) hS = 1.0;
				if( hS < 0.0 ) hS = 0.0;
				controlPoints[ ppoint ] = hS;
				updateTFColor();

			} else {

				real yP = yOrtho(y);
				if (yP > 1.0) yP = 1.0;
				if (yP < 0.0) yP = 0.0;
				tf[ ppoint ][3] = yP;

			}

		} else { /// Brigthness point

			real xP = xOrtho(x);
			if (xP > 1.0) xP = 1.0;
			if (xP < 0.0) xP = 0.0;
			brightness = (real)( xP * (maxBrightness-minBrightness) );

		}

		updateTFAlpha();

	}

	/// Release picked point
	void releasePickedPoint(int x, int y) {

		if( ppoint >= numColors || hueSelection )
			unSetPickedPoint();

		hueSelection = false;

	}

	/// OpenGL Setup
	void glSetup() {
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f); ///< Fixed white background
	}

	/// Picking function
	/// @arg x, y mouse position when hit
	void pick(int x, int y) {

		GLsizei xP;

		for (mapIt it = controlPoints.begin(); it != controlPoints.end(); ++it) {

			xP = xScreen((*it).first * stepColor);

			if ( ( y >= yScreen(1.0) ) && ( y <= yScreen(0.0) )
			     && ( x >= xP - 5 ) && ( x <= xP + 5 ) ) { /// Control Point picked

				ppoint = (*it).first;
				return;

			}

		}

		xP = xScreen(brightness * stepBrightness);

		if ( ( y >= yScreen(-0.15) - 5) && (y <= yScreen(-0.15) + 5)
		     && (x >= xScreen(0.0) - 5) && (x <= xScreen(1.0) - 5) ) { /// Brightness picked

			ppoint = numColors;
			return;

		}

		if( ppoint < numColors && y >= yScreen(1.0) && y <= yScreen(0.2)
		    && x >= xScreen(1.1) && x <= xScreen(1.2) ) { /// Hue Color picked

			hueSelection = true;
			return;

		}		

		unSetPickedPoint();

		updateTFAlpha();

	}

	/// Create a new control point
	/// @arg x, y mouse position when hit
	void createControlPoint(int x, int y) {

		if ( (x < xScreen(0.0)) && (x > xScreen(1.0)) ) return;

		for (GLuint i = 0; i < numColors; ++i) {

			if ( (y >= yScreen(tf[i][3]) - 2) && (y <= yScreen(tf[i][3]) + 2)
			     && (x >= xScreen(i * stepColor) - 2) && (x <= xScreen(i * stepColor) + 2) ) {

				controlPoints[ i ] = 0.0;
				mapIt it = controlPoints.find(i);
				mapIt itPrev(it); itPrev--;
				mapIt itNext(it); itNext++;

				real si = ( i - itPrev->first ) / (real) ( itNext->first - itPrev->first );
				real s = si * ( itNext->second - itPrev->second ) + itPrev->second;
				controlPoints[ i ] = s;

				return;

			}

		}

	}

	/// Delete the picked control point
	void deleteControlPoint(void) {

		if (ppoint >= numColors) return;

		/// Refuses delete of the two control points in the TF range limit
		if ((ppoint == 0) || (ppoint == numColors-1)) return;

		mapIt it = controlPoints.find( ppoint );
		mapIt itPrev(it); itPrev--;
		mapIt itNext(it); itNext++;

		if (itPrev != controlPoints.end()) ppoint = itPrev->first;
		else if (itNext != controlPoints.end()) ppoint = itNext->first;
		else ppoint = numColors+1;

		controlPoints.erase( it );

		updateTFAlpha();

		ppoint = numColors+1;

	}

	/// Color code
	void colorCode(int cc) {

		if (cc == 1) {
			controlPoints[  0  ] = 155/360.0;
			controlPoints[ 255 ] = 360/360.0;
		} else if (cc == 2) {
			controlPoints[  0  ] = 360/360.0;
			controlPoints[ 255 ] = 155/360.0;
		} else if (cc == 3) {
			controlPoints[  0  ] =   0/360.0;
			controlPoints[ 255 ] = 155/360.0;
		} else if (cc == 4) {
			controlPoints[  0  ] = 155/360.0;
			controlPoints[ 255 ] =   0/360.0;
		} else if (cc == 5) {
			controlPoints[  0  ] = 170/360.0;
			controlPoints[ 255 ] = 240/360.0;
		} else if (cc == 6) {
			controlPoints[  0  ] = 240/360.0;
			controlPoints[ 255 ] = 170/360.0;
		} else if (cc == 7) {
			controlPoints[  0  ] =  26/360.0;
			controlPoints[ 255 ] = 146/360.0;
		} else if (cc == 8) {
			controlPoints[  0  ] =  64/360.0;
			controlPoints[ 255 ] = 218/360.0;
		} else if (cc == 9) {
			controlPoints[  0  ] = 218/360.0;
			controlPoints[ 255 ] =  64/360.0;
		} else if (cc == 0) {
			controlPoints[  0  ] =   0/360.0;
			controlPoints[ 255 ] = 360/360.0;
		}

		updateControlPoints();
		updateTFColor();

	}

	/// Draw transfer function informations
	void draw(void) {

		if (!tf) return;

		glBegin(GL_QUADS); /// Transfer Function RGBAs

		real x0, x1;

		for (GLuint i = 0; i < numColors; ++i) {

			glColor3f( tf[i].r(), tf[i].g(), tf[i].b() );
			x0 = (i - 0.5) * stepColor; x1 = (i + 0.5) * stepColor;
			glVertex2d( x0, 0.0); glVertex2d( x1, 0.0);
			glVertex2d( x1, tf[i][3]); glVertex2d( x0, tf[i][3]);

		}

		glEnd();

		glBegin(GL_LINES); /// Brightness line
		glColor3f(1.0, 1.0, 1.0); glVertex2d(0.0, -0.15);
		glColor3f(0.0, 0.0, 0.0); glVertex2d(1.0, -0.15);
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
			glVertex2d( (*it).first * stepColor, tf[(*it).first][3] ); /// Draw Control point
			glEnd();

		}

		if( ppoint == numColors ) {
			glColor3d(0.7, 0.3, 0.1);
			glPointSize(8.0);
		} else {
			glColor3d(0.0, 0.0, 0.0);
			glPointSize(4.0);
		}

		glBegin(GL_POINTS);
		glVertex2d( brightness * stepBrightness, -0.15); /// Brightness point
		glEnd();

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
		glWrite(0.45, 1.2, (char*)tfName.c_str());
		glWrite(-0.05, 1.14, "Alpha");
		glWrite(1.12, -0.01, "Scalar");
		glWrite(1.04, -0.16, "Brightness");

		if (ppoint != numColors+1) {

			glColor3d(0.7, 0.3, 0.1);

			char str[256];

			if (ppoint == numColors) { /// Brightness point

				sprintf(str, "%.2lf", brightness );
				glWrite( (brightness - 0.5) * stepBrightness, -0.12, str );

			} else { /// Control point

				sprintf(str, "%.2lf", tf[ ppoint ][3] );
				glWrite( -0.12, tf[ppoint][3], str);

				sprintf(str, "%d", ppoint );
				glWrite( (ppoint - 0.5 ) * stepColor, -0.06, str);

			}
		}

		drawScale();

	}

private:

	/// Update TF Alpha values
	void updateTFAlpha(void) {

		if (ppoint >= numColors) return;

		mapIt it = controlPoints.find( ppoint );
		mapIt itPrev(it); itPrev--;
		mapIt itNext(it); itNext++;

		natural x, xCurr = it->first;
		real y, yCurr = tf[ xCurr ][3];
		real a, b;

		if ( itPrev != controlPoints.end() ) {

			x = itPrev->first;
			y = tf[ x ][3];

			a = (yCurr - y) / (real)(xCurr - x);
			b = yCurr - a * xCurr;

			for (natural i = x+1; i < xCurr; ++i) {

				tf[ i ][3] = a * i + b;

			}
			
		}

		if ( itNext != controlPoints.end() ) {

			x = itNext->first;
			y = tf[ x ][3];

			a = (y - yCurr) / (real)(x - xCurr);
			b = yCurr - a * xCurr;

			for (natural i = xCurr+1; i < x; ++i) {

				tf[ i ][3] = a * i + b;

			}
			
		}

	}

	/// Update TF Color values
	void updateTFColor( void ) {

		mapIt itCurr = controlPoints.begin();
		mapIt itNext(itCurr); itNext++;

		real si, s;

		for (natural i = 0; i < numColors; ++i) {

			if( i > itNext->first ) {
				itCurr++;
				itNext++;
			}

			si = ( i - itCurr->first ) / (real) ( itNext->first - itCurr->first );

			s = si * ( itNext->second - itCurr->second ) + itCurr->second;

			StoRGB( tf[i][0], tf[i][1], tf[i][2], s );

		}

	}

	/// Update control points hue scalar values
	void updateControlPoints( void ) {

		real min = controlPoints.begin()->second, max = controlPoints.rbegin()->second;

		for (mapIt it = controlPoints.begin(); it != controlPoints.end(); ++it) {

			if( it->first == 0 || it->first == 255 ) continue;

			it->second = (it->first / (real)255) * (max - min) + min;

		}

	}

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

	/// Convert from HSV to RGB
	/// @arg h, s, v input HSV color
	/// @arg r, g, b output RGB color
	void HSVtoRGB(const real& h, const real& s, const real& v, real& r, real& g, real& b) const {
		if (s == 0) { r = v; g = v; b = v; }
		else {
			real var_h = h * 6, var_i = floor( var_h ), var_1 = v * ( 1 - s ),
				var_2 = v * ( 1 - s * ( var_h - var_i ) ),
				var_3 = v * ( 1 - s * ( 1 - ( var_h - var_i ) ) );
			if     ( var_i == 0 ) { r = v     ; g = var_3 ; b = var_1; }
			else if( var_i == 1 ) { r = var_2 ; g = v     ; b = var_1; }
			else if( var_i == 2 ) { r = var_1 ; g = v     ; b = var_3; }
			else if( var_i == 3 ) { r = var_1 ; g = var_2 ; b = v;     }
			else if( var_i == 4 ) { r = var_3 ; g = var_1 ; b = v;     }
			else                  { r = v     ; g = var_1 ; b = var_2; }
		}
	}

	/// Convert between two Scalars (s,t) to RGB using HSV colors
	void StoRGB( real& red, real& green, real& blue, real _s ) const {

		_s = ( _s >= 1.0 ) ? 0.99999 : ( ( _s < 0.0 ) ? 0.0 : _s );

		/// Convert hue, saturation, value to red, green, blue colors
		HSVtoRGB( _s, 60/100.0, 90/100.0, red, green, blue );

	}

	/// Draw color scale in right side
	void drawScale( void ) {

		real r, g, b, s;

		glBegin(GL_QUAD_STRIP);

		for (GLuint i = 0; i <= numColors; ++i) {

			s = i / (real)numColors;

			StoRGB(r, g, b, s);

			glColor3f( r, g, b );

			glVertex2f( 1.1, s*0.8 + 0.2 ); glVertex2f( 1.2, s*0.8 + 0.2 );

		}

		glEnd();

		for (mapIt it = controlPoints.begin(); it != controlPoints.end(); ++it) {

			if ((*it).first == ppoint) {
				glColor3d(0.7, 0.3, 0.1);
				glPolygonMode(GL_FRONT, GL_FILL);
			} else {
				glColor3d(0.0, 0.0, 0.0);
				glPolygonMode(GL_FRONT, GL_LINE);
			}

			s = it->second * 0.8 + 0.2;

			glBegin(GL_QUADS);

			glVertex2f(1.095, s - 0.005 ); glVertex2f(1.205, s - 0.005 );
			glVertex2f(1.205, s + 0.005 ); glVertex2f(1.095, s + 0.005 );
			glEnd();

		}

		glPolygonMode(GL_FRONT, GL_FILL);

	}

	vec4 *tf; ///< Pointer to transfer function values

	bool hueSelection; ///< hue selection flag

	natural numColors; ///< Number of TF colors

	real brightness, minBrightness, maxBrightness; ///< Brightness term

	natural ppoint; ///< Picked point = [0, numColors-1]: control point id; numColors: brightness; numColors+1: no point picked

	map< natural, real > controlPoints; ///< Vector of control points

	GLdouble minOrthoSize, maxOrthoSize; /// Minimum and maximum ortho size
	GLsizei winWidth, winHeight; ///< Width x Height pixel resolution

	real stepColor, stepBrightness; ///< Step distance in color and brightness

	string tfName, cpName; ///< Transfer Function (TF) name

};

#endif
