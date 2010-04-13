/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 *  Maximo, Andre -- Mar, 2009
 *
 */

/**
 *   hapt.geom : For each GL_POINTS rendered, one to four triangles (using
 *               GL_TRIANGLE_STRIP) will be generated depending on PT classification.
 *
 *  Original Projected Tetrahedra (PT) Algorithm:
 *
 *  author = {P. Shirley and A. A. Tuchman}
 *  title = {Polygonal Approximation to Direct Scalar Volume Rendering}
 *
 * GLSL code.
 *
 */

/// Shader Model 4.0 is required
#extension GL_EXT_geometry_shader4 : enable
#extension GL_EXT_gpu_shader4 : enable 

uniform isampler1D orderTableTex; ///< Ternary Truth Table (TTT) Texture
uniform isampler1D tfanOrderTableTex; ///< Triangle Fan Vertex Order Texture

vec4 tetVert[5]; ///< Original vertex position
float tetScalars[4]; ///< Original scalar values
vec4 vertOrder[4]; ///< Vertices in right order (basis graph)
float scalarOrder[4]; ///< Scalars in right order (basis graph)
float paramU1, paramU2; ///< Line x Line intersection parameters

int countTFan; ///< Count Triangle Fan (3, 4, 5 or 6 depend on the PT class)
int idTTT; ///< Index of Ternary Truth Table (TTT)
float thickness; ///< Distance between intersection point (projected and original)
float scalarFront, scalarBack; ///< Thick vertex scalar front and back

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

/// Project Tetrahedron Classification
///   Compute cross between tetrahedron vertices to classify
///   the projection using four classification tests

void ptClassification(void) {

	vec4 c;
	ivec4 tests; ///< Classification tests: 4

	vec3 vc1_0 = tetVert[1].xyz - tetVert[0].xyz;
	vec3 vc2_0 = tetVert[2].xyz - tetVert[0].xyz;
	vec3 vc3_0 = tetVert[3].xyz - tetVert[0].xyz;
	vec3 vc1_2 = tetVert[1].xyz - tetVert[2].xyz;
	vec3 vc1_3 = tetVert[1].xyz - tetVert[3].xyz;

	c.x = cross(vc1_0, vc2_0).z;
	c.y = cross(vc1_0, vc3_0).z;
	c.z = cross(vc2_0, vc3_0).z;
	c.w = cross(vc1_2, vc1_3).z;

	countTFan = 5;

	for (int i = 0; i < 4; ++i) {

		tests[i] = int(sign( c[i]) ) + 1;

		if (tests[i] == 1) --countTFan;

	}

	idTTT = tests.x * 27 + tests.y * 9 + tests.z * 3 + tests.w * 1;

}

/// Order Vertices
///   Perform TTT texture lookup to order the tetrahedron vertices

void orderVertices(void) {

	ivec4 tableRow = texelFetch1D(orderTableTex, idTTT, 0);

	for(int i = 0; i < 4; i++) {

		if (tableRow[i] == 0) {

			vertOrder[i] = tetVert[0];
			scalarOrder[i] = tetScalars[0];

		} else if (tableRow[i] == 1) {

			vertOrder[i] = tetVert[1];
			scalarOrder[i] = tetScalars[1];

		} else if (tableRow[i] == 2) {

			vertOrder[i] = tetVert[2];
			scalarOrder[i] = tetScalars[2];

		} else {

			vertOrder[i] = tetVert[3];
			scalarOrder[i] = tetScalars[3];

		}

	}

}

/// Compute line intersection parameters

void computeParams(void) {

	float denominator, numeratorU1, numeratorU2;

	if (countTFan == 3) {

		paramU1 = 1.0;
		paramU2 = 1.0;

	} else if (countTFan == 4) {

		/// Line intersection denominator between v0->v2 and v1->v3
		denominator = ((vertOrder[3].y - vertOrder[1].y) * (vertOrder[2].x - vertOrder[0].x)) -
			((vertOrder[3].x - vertOrder[1].x) * (vertOrder[2].y - vertOrder[0].y));

		/// Line defined by vector v1->v3
		numeratorU2 = ((vertOrder[2].x - vertOrder[0].x) * (vertOrder[0].y - vertOrder[1].y)) -
			((vertOrder[2].y - vertOrder[0].y) * (vertOrder[0].x - vertOrder[1].x));

		paramU1 = 1.0;
		paramU2 = numeratorU2 / denominator;

	} else {
		/// Line intersection denominator between v0->v2 and v1->v3
		denominator = ((vertOrder[3].y - vertOrder[1].y) * (vertOrder[2].x - vertOrder[0].x)) -
			((vertOrder[3].x - vertOrder[1].x) * (vertOrder[2].y - vertOrder[0].y));

		/// Line defined by vector v0->v2
		numeratorU1 = ((vertOrder[3].x - vertOrder[1].x) * (vertOrder[0].y - vertOrder[1].y)) -
			((vertOrder[3].y - vertOrder[1].y) * (vertOrder[0].x - vertOrder[1].x));

		/// Line defined by vector v1->v3
		numeratorU2 = ((vertOrder[2].x - vertOrder[0].x) * (vertOrder[0].y - vertOrder[1].y)) -
			((vertOrder[2].y - vertOrder[0].y) * (vertOrder[0].x - vertOrder[1].x));

		paramU1 = numeratorU1 / denominator;
		paramU2 = numeratorU2 / denominator;

	}

}

/// Compute intersection between lines (using the basis graph)

void computeIntersection(void) {

	tetVert[4] = vec4(0.0); ///< Intersection vertex = thick vertex

	if (countTFan == 3) {

		thickness = (vertOrder[0].z - vertOrder[1].z);

	}  else if (countTFan == 4) {

		float zBackIntersection = vertOrder[1].z + paramU2*(vertOrder[3].z - vertOrder[1].z);

		thickness = (vertOrder[2].z - zBackIntersection);

	} else {

		/// Find z coordinate of back intersection point by
		///   interpolating original vertices (not projected)
		float zBackIntersection = vertOrder[1].z + paramU2*(vertOrder[3].z - vertOrder[1].z);
	
		/// Find ordered intersection point between the two ordered lines (basis graph)
		tetVert[4] = (vertOrder[0] + paramU1*(vertOrder[2] - vertOrder[0]));

		thickness = (tetVert[4].z - zBackIntersection);

	}

}

/// Compute scalar front and back
///   Interpolate scalar values in the same manner as done for intersection vertex

void computeScalars(void) {

	if (countTFan == 6) {

		scalarFront = scalarOrder[0] + paramU1*(scalarOrder[2] - scalarOrder[0]);
		scalarBack = scalarOrder[1] + paramU2*(scalarOrder[3] - scalarOrder[1]);

	} else if (countTFan == 5) {

		scalarFront = scalarOrder[2];
		float tmp = scalarOrder[1] + paramU2*(scalarOrder[3] - scalarOrder[1]);
		scalarBack = scalarOrder[0] + (tmp - scalarOrder[0])*paramU1;

	} else if (countTFan == 4) {

		scalarFront = scalarOrder[2];
		scalarBack = scalarOrder[1] + paramU2*(scalarOrder[3] - scalarOrder[1]);

	} else {

		scalarFront = scalarOrder[0];
		scalarBack = scalarOrder[1];

	}

}

/// Emit one vertex

void emitVert(in int i, in bool thick = false) {

	gl_Position = tetVert[ i ];

	if (thick) gl_FrontColor = vec4(scalarFront, scalarBack, thickness, 0.0);
	else gl_FrontColor = vec4(tetScalars[ i ], tetScalars[ i ], 0.0, 0.0);

	EmitVertex();

}

/// Emit final triangles

void emitTriangles(void) {

	ivec4 tfanId = texelFetch1D(tfanOrderTableTex, idTTT, 0);

	if (countTFan == 6) {

		emitVert( tfanId[0] );
		emitVert( tfanId[1] );
		emitVert( 4, true );
		emitVert( tfanId[2] );
		emitVert( tfanId[3] );

		EndPrimitive();

		emitVert( 4, true );
		emitVert( tfanId[3] );
		emitVert( tfanId[0] );

		EndPrimitive();

	} else if (countTFan == 5) {

		emitVert( tfanId[1] );
		emitVert( tfanId[2] );
		emitVert( tfanId[0], true );
		emitVert( tfanId[3] );
		emitVert( tfanId[1] );

		EndPrimitive();

	} else if (countTFan == 4) {

		emitVert( tfanId[1] );
		emitVert( tfanId[2] );
		emitVert( tfanId[0], true );
		emitVert( tfanId[3] );

		EndPrimitive();

	} else {

		emitVert( tfanId[1] );
		emitVert( tfanId[2] );
		emitVert( tfanId[0], true );

		EndPrimitive();

	}

}

void main(void) {

	dataRetrieval();

	ptClassification();

	if (countTFan < 3) return;

	orderVertices();

	computeParams();

	computeIntersection();

	if (countTFan == 5) {

		if (paramU1 > 1.0) {

			thickness /= paramU1;
			paramU1 = 1.0 / paramU1;

		} else { // count == 6 ; thick vertex == intersection vertex

			countTFan = 6;

		}

	}

	computeScalars();

	thickness = abs(thickness);

	emitTriangles();

}
