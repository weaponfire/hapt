/**
 *   HAPT -- Hardware-Assisted Projected Tetrahedra
 *
 * Maximo, Andre; Marroquim, Ricardo -- May, 2006
 *
 */

/**
 *   hapt.frag :
 *     [1] Determine colors for scalar front and back (2 accesses);
 *     [2] Compute psi gamma table parameters;
 *     [3] Evaluate opacity;
 *     [4] Read psi (1 access);
 *     [5] Paint the fragment.
 *
 * Second Shader of the original Projected Tetrahedra with Partial
 * Pre-Integration (PTINT) algorithm:
 *
 * author = {Ricardo Marroquim and Andre Maximo and Ricardo Farias and Claudio Esperanca}
 * title = {GPU-Based Cell Projection for Interactive Volume Rendering}
 *
 * GLSL code.
 *
 */

uniform sampler1D tfTex; ///< Transfer Function Texture
uniform sampler2D psiGammaTableTex; ///< Psi Gamma Table (Pre-Integration) Texture
uniform float preIntTexSize; ///< Pre-Integration (Quad) Texture width

uniform float maxEdgeLength; ///< Maximum edge length

uniform float brightness; ///< Brightness term

/// Main

void main(void) {

	float sf = gl_Color.r; ///< Scalar front
	float sb = gl_Color.g; ///< Scalar back
	float l = gl_Color.b; ///< Thickness or isoOpacity

	if (l == 0.0) /// No fragment color
		discard;

	l *= brightness; /// Brightness by thickness
	l /= maxEdgeLength; /// Normalize thickness [0, 1]	

	vec4 colorFront = texture1D(tfTex, sf);
	vec4 colorBack = texture1D(tfTex, sb);

	vec4 color;

	vec2 tau = vec2(colorFront.a, colorBack.a) * l;

	vec2 halfVec = vec2(0.5);

	float zeta = exp( -dot(tau, halfVec) );

	if (zeta == 1.0) /// No fragment color
		discard;

	vec2 gamma = tau / (1.0 + tau);

	float psi = texture2D(psiGammaTableTex, gamma + (halfVec / vec2(preIntTexSize))).a;

	color.rgb = colorFront.rgb*(1.0 - psi) + colorBack.rgb*(psi - zeta);
	color.a = 1.0 - zeta;

	//color.rgb *= color.a;

	gl_FragColor = color; ///< Output fragment color

}
