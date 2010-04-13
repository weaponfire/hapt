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

uniform sampler1D tfTex; ///< Transfer Function Texture

void main(void) {

	if( texture1D(tfTex, gl_MultiTexCoord3.x).a == 0.0 )
		if( texture1D(tfTex, gl_MultiTexCoord3.y).a == 0.0 )
			if( texture1D(tfTex, gl_MultiTexCoord3.z).a == 0.0 )
				if( texture1D(tfTex, gl_MultiTexCoord3.w).a == 0.0 ) {

					gl_Position = vec4(0.0);
					gl_TexCoord[0] = vec4(0.0);
					gl_TexCoord[1] = vec4(0.0);
					gl_TexCoord[2] = vec4(0.0);

					return;

				}

	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	gl_TexCoord[0] = gl_ModelViewProjectionMatrix * vec4(gl_MultiTexCoord0.xyz, 1.0);

	gl_TexCoord[1] = gl_ModelViewProjectionMatrix * vec4(gl_MultiTexCoord1.xyz, 1.0);

	gl_TexCoord[2] = gl_ModelViewProjectionMatrix * vec4(gl_MultiTexCoord2.xyz, 1.0);

	gl_FrontColor.rg = gl_MultiTexCoord3.xy;
	gl_BackColor.rg = gl_MultiTexCoord3.zw;

}
