/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 * Maximo, Andre -- Mar, 2009
 *
 */

/**
 *   hapt.vert : Compute transformation for four vertices of one tetrahedron
 *               sent as a GL_POINT to the graphics pipeline using position and
 *               3 texCoords to store v_0, v_1, v_2 and v_3, respectively.
 *
 * GLSL code.
 *
 */

void main(void) {

	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	gl_TexCoord[0] = gl_ModelViewProjectionMatrix * vec4(gl_MultiTexCoord0.xyz, 1.0);

	gl_TexCoord[1] = gl_ModelViewProjectionMatrix * vec4(gl_MultiTexCoord1.xyz, 1.0);

	gl_TexCoord[2] = gl_ModelViewProjectionMatrix * vec4(gl_MultiTexCoord2.xyz, 1.0);

	gl_FrontColor.rg = vec2(gl_Vertex.w, gl_MultiTexCoord0.w);
	gl_BackColor.rg = vec2(gl_MultiTexCoord1.w, gl_MultiTexCoord2.w);

}
