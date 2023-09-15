#version 430 core

// output variable for the fragment shader
out vec4 colorFrag;

in VS_OUT 
{
	// tangent space data
	vec3 twPointLightPos[MAX_POINT_LIGHTS];
	vec3 twFragPos; // Local -> World -> Tangent position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twCameraPos; 
	vec3 tNormal;

	// the output variable for UV coordinates
	vec2 interp_UV;
} fs_in;

//Lights
uniform uint nPointLights;

uniform PointLight pointLights[MAX_POINT_LIGHTS];

// Textures
uniform sampler2D diffuse_tex; // texture samplers
uniform sampler2D normal_tex;

uniform float uv_repeat; // texture repetitions

// Material-light attributes
uniform vec3 ambient ;
uniform vec3 diffuse ; // we dont need this with diffuse textures
uniform vec3 specular;

// Ambient, diffuse, specular coefficients
uniform float kA;
uniform float kD;
uniform float kS;

// shininess coefficients (passed from the application)
uniform float shininess;

// uniforms for GGX model
uniform float alpha; // rugosity - 0 : smooth, 1: rough
uniform float F0; // fresnel reflectance at normal incidence

// Current light position
vec3 curr_twLightPos;
vec3 curr_vwLightPos;

vec3 BlinnPhongTangent()
{
	// we repeat the UVs and we sample the texture
    vec2 repeated_UV = mod(fs_in.interp_UV * uv_repeat, 1.0);
    vec4 surfaceColor = texture(diffuse_tex, repeated_UV);

	// ambient component can be calculated at the beginning
	vec3 color = kA * ambient;
	
	// obtain normal from normal map in range [0,1]
	vec3 N = texture(normal_tex, repeated_UV).rgb;
	
	// transform normal vector to range [-1,1]
	N = normalize(N * 2.0 - 1.0);  // this normal is in tangent space
	
	// normalization of the per-fragment light incidence direction
	vec3 L = normalize(curr_twLightPos - fs_in.twFragPos);
	
	// Lambert coefficient
	float lambertian = max(dot(L,N), 0.0);
	
	// if the lambert coefficient is positive, then I can calculate the specular component
	if(lambertian > 0.0)
	{
		// the tangent space vector has been calculated in the vertex shader
		vec3 V = normalize( fs_in.twCameraPos - fs_in.twFragPos );
		
		// in the Blinn-Phong model we do not use the reflection vector, but the half vector
		vec3 H = normalize(L + V);
		
		// we use H to calculate the specular component
		float specAngle = max(dot(H, N), 0.0);
		// shininess application to the specular component
		float spec = pow(specAngle, shininess);
		
		// We add diffusive and specular components to the final color
		// N.B. ): in this implementation, the sum of the components can be different than 1
		color += vec3( kD * lambertian * surfaceColor.xyz + kS * spec * specular);

		// fake shadow
		float cam_frag_distance = distance(fs_in.twFragPos, fs_in.twCameraPos) + 0.01f;
		float threshold = 1.f;
		if(cam_frag_distance < threshold) color *= (cam_frag_distance / threshold);
	}

	//if(fs_in.interp_UV.x > 0.3 && fs_in.interp_UV.x < 0.5) color *= 0;

	return color;
}

vec4 calculatePointLights()
{
	vec4 color = vec4(0);
	for(int i = 0; i < nPointLights; i++)
	{
		curr_twLightPos = fs_in.twPointLightPos[i];
		color += vec4(BlinnPhongTangent(), 1.0f) * pointLights[i].color * pointLights[i].intensity;
	}
	//color.a = normalize(color.a);
	return color;
}

void main()
{
	vec4 color = vec4(0);
	
	color += calculatePointLights();
	
	colorFrag = color;
}