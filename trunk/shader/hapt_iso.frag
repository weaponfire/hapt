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

/**
 * Marroquim, Ricardo -- 17-Apr 2009
 * Inserted iso/dvr option passed though gl_Color.a
 * Enables the opacity of the iso-surface to be independent from the transfer function alpha
 * only multiplies length by brightness for DVR
 */

/**
 * Marroquim, Ricardo -- 17-Apr 2009
 * Illumination for iso-surfaces
 */

uniform sampler1D tfTex; ///< Transfer Function Texture
uniform sampler2D psiGammaTableTex; ///< Psi Gamma Table (Pre-Integration) Texture
uniform float preIntTexSize; ///< Pre-Integration (Quad) Texture width

uniform vec3 illumination; ///< ambient, diffuse and specular illumination intensities
uniform int illuminate; ///< flag to switch on/off illumination

varying in vec3 normal_vec;

/// Main

void main(void) {

	float sf = gl_Color.r; ///< Scalar front
	float sb = gl_Color.g; ///< Scalar back
	float l = gl_Color.b; ///< Thickness or isoOpacity
	//	float iso = gl_Color.a; ///< 1.0 if iso-surface; 0.0 if DVR

	if (l == 0.0) /// No fragment color
		discard;

	vec4 colorFront = texture1D(tfTex, sf);
	vec4 colorBack = texture1D(tfTex, sb);

	vec4 color;

	// Don't use opacity from transfer function for isosurfaces
	colorFront.a = colorBack.a = 1.0;

	vec2 tau = vec2(colorFront.a, colorBack.a) * l;

	vec2 halfVec = vec2(0.5);

	float zeta = exp( -dot(tau, halfVec) );

	if (zeta == 1.0) /// No fragment color
		discard;

	vec2 gamma = tau / (1.0 + tau);

	float psi = texture2D(psiGammaTableTex, gamma + (halfVec / vec2(preIntTexSize))).a;

	color.rgb = colorFront.rgb*(1.0 - psi) + colorBack.rgb*(psi - zeta);
	color.a = 1.0 - zeta;

	// illuminate iso-surface
	if ((illuminate == 1)) {

	  vec3 c = color.rgb;
      vec3 lightDir = normalize(vec3(gl_LightSource[0].position));
     
	  color.rgb = c * (gl_LightSource[0].ambient + gl_LightModel.ambient).rgb * illumination[0];

	  float NdotL = abs(dot(normal_vec.xyz, lightDir.xyz));
	  color.rgb += c.rgb * gl_LightSource[0].diffuse.rgb * NdotL * illumination[1];

	  if (illumination[2] != 0.0) {		
		float NdotHV = abs(dot(normal_vec.xyz, gl_LightSource[0].halfVector.xyz)) ;
		color.rgb += c * gl_LightSource[0].specular.rgb * pow(NdotHV, 10.0 * illumination[2]);
	  }	  
	}

	//color.rgb *= color.a;

	gl_FragColor = color; ///< Output fragment color

}
