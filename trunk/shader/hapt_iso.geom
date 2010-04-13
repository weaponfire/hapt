/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 *  Marroquim, Ricardo -- Apr, 2009
 *
 */

/**
 *   hapt.geom : For each GL_POINTS rendered, zero, one or two triangles (using
 *               GL_TRIANGLE_STRIP) will be generated depending on how (and if) the
 *               isosurface cuts this tetrahedron.
 *
 * GLSL code.
 *
 */

/// Shader Model 4.0 is required
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable 

//uniform float isoValue;

uniform vec4 isoValue; ///< iso-value
uniform vec4 isoOpacity; ///< iso-opacity

varying out vec3 normal_vec;

vec4 tetVert[5]; ///< Original vertex position
float tetScalars[4]; ///< Original scalar values

vec4 isoVerts[4];
int numIsoVerts;

/// Data Retrieval

void dataRetrieval(void) {

	tetVert[0] = gl_PositionIn[0];

	tetVert[1] = gl_TexCoordIn[0][0];

	tetVert[2] = gl_TexCoordIn[0][1];

	tetVert[3] = gl_TexCoordIn[0][2];

	tetScalars[0] = gl_FrontColorIn[0][0];

	tetScalars[1] = gl_FrontColorIn[0][1];

	tetScalars[2] = gl_BackColorIn[0][0];

	tetScalars[3] = gl_BackColorIn[0][1];

}

/// Checks if given isosurface crosses this tetrahedron

bool checkForIsosurface( in int iso) {

  if ((isoValue[iso] == 0.0) || (isoOpacity[iso] == 0.0))
	return false;
	  
  if ((tetScalars[0] < isoValue[iso]) && (tetScalars[1] < isoValue[iso]) && (tetScalars[2] < isoValue[iso]) && (tetScalars[3] < isoValue[iso]))
	return false;

  if ((tetScalars[0] > isoValue[iso]) && (tetScalars[1] > isoValue[iso]) && (tetScalars[2] > isoValue[iso]) && (tetScalars[3] > isoValue[iso]))
	return false;

  return true;
}


/// Computes the intersection of the isosurface with an edge, if any

bool edgeIsosurfaceIntersection(in int v0, in int v1, in int iso) {

  // isoValue is in range
  if (((tetScalars[v0] > isoValue[iso]) && (tetScalars[v1] < isoValue[iso])) ||
	  ((tetScalars[v0] < isoValue[iso]) && (tetScalars[v1] > isoValue[iso]))) {
	// compute the intersection vertice (isosurface - tetrahedron edge)
	float t = (isoValue[iso] - tetScalars[v0]) / (tetScalars[v1] - tetScalars[v0]);
	isoVerts[numIsoVerts] = tetVert[v0] + t*(tetVert[v1] - tetVert[v0]);
	numIsoVerts++;
	return true;
  }
  return false;
}

/// Check for iso-surface crossing each tetrahedron each

void computeIsosurface(in int iso) {
  
  if (edgeIsosurfaceIntersection(0, 1, iso)) {
	edgeIsosurfaceIntersection(1, 2, iso);
	edgeIsosurfaceIntersection(3, 1, iso);
	edgeIsosurfaceIntersection(3, 2, iso);
	edgeIsosurfaceIntersection(0, 3, iso);
	edgeIsosurfaceIntersection(0, 2, iso);
  }
  else {
	edgeIsosurfaceIntersection(0, 2, iso);
	edgeIsosurfaceIntersection(1, 2, iso);
	edgeIsosurfaceIntersection(3, 1, iso);
	edgeIsosurfaceIntersection(3, 2, iso);
	edgeIsosurfaceIntersection(0, 3, iso);
  }
}

/// Emit one vertex

 void emitIsoVert(in int i, in int iso) {

	gl_Position = isoVerts[ i ];
	gl_FrontColor = vec4(isoValue[iso], isoValue[iso], isoOpacity[iso], 1.0);

	EmitVertex();

}

/// Emit final triangles of isosurface

void emitIsoTriangles(in int iso) {

  normal_vec = normalize(cross(isoVerts[1].xyz - isoVerts[0].xyz, isoVerts[2].xyz - isoVerts[0].xyz));
  emitIsoVert(1, iso);
  emitIsoVert(0, iso);
  emitIsoVert(2, iso);
  if (numIsoVerts == 4) {
	normal_vec = normalize(cross(isoVerts[2].xyz - isoVerts[0].xyz, isoVerts[3].xyz - isoVerts[0].xyz));
	emitIsoVert(3, iso);
  }
  EndPrimitive();
}


void main(void) {

  normal_vec = vec3(0.0,0.0,0.0);

  dataRetrieval();

  // check the four iso-surfaces
  for (int i = 0; i < 4; ++i) {
	if (checkForIsosurface(i)) {
	  numIsoVerts = 0;
	  computeIsosurface(i);
	  emitIsoTriangles(i);
	}
  }
}
