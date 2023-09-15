#version 430 core

// output variable for the fragment shader
out vec4 colorFrag;

in VS_OUT 
{
	// tangent space data
	vec3 twPointLightDir[MAX_POINT_LIGHTS];
	vec3 twDirLightDir  [MAX_DIR_LIGHTS];
	vec3 twFragPos; // Local -> World -> Tangent position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twCameraPos; 
	vec3 tNormal;

	// the output variable for UV coordinates
	vec2 interp_UV;
} fs_in;

//Lights
uniform uint nPointLights;
uniform uint nDirLights;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];

// Textures
uniform sampler2D diffuse_tex; // texture samplers
uniform sampler2D normal_tex;
uniform sampler2D displacement_tex;

uniform float repeat; // texture repetitions

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

// uniform for parallax map
uniform float height_scale;

// Current light position
vec3 curr_twLightDir;

vec2 CheapParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
	// Sample the heightmap
    float height = texture(displacement_tex, texCoords).r; 
	// Calculate vector towards approximate height
	vec2 p = viewDir.xy * (height * height_scale);
	// Return displacement
    return texCoords - p; 
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    // depth of current layer
    float currentLayerDepth = 0.0;
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * height_scale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(displacement_tex, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(displacement_tex, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(displacement_tex, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec3 BlinnPhong()
{
	vec2 repeated_UV = mod(fs_in.interp_UV * repeat, 1.0);

	// view direction
	vec3 V = normalize( fs_in.twCameraPos - fs_in.twFragPos );

    vec2 displaced_texCoords = CheapParallaxMapping(repeated_UV,  V);   
	    
	float max_height = 1.0f; float min_height = 0.0f;

	//if(displaced_texCoords.x > max_height || displaced_texCoords.y > max_height || displaced_texCoords.x < min_height || displaced_texCoords.y < min_height)
    //	discard;

	// we repeat the UVs and we sample the texture
    vec4 surfaceColor = texture(diffuse_tex, displaced_texCoords);

	// ambient component can be calculated at the beginning
	vec3 color = kA * ambient;
	
	// obtain normal from normal map in range [0,1]
	vec3 N = texture(normal_tex, displaced_texCoords).rgb;
	
	// transform normal vector to range [-1,1]
	N = normalize(N * 2.0 - 1.0);  // this normal is in tangent space
	
	// normalization of the per-fragment light incidence direction
	vec3 L = normalize(curr_twLightDir);
	
	// Lambert coefficient
	float lambertian = max(dot(L,N), 0.0);
	
	// if the lambert coefficient is positive, then I can calculate the specular component
	if(lambertian > 0.0)
	{
		// in the Blinn-Phong model we do not use the reflection vector, but the half vector
		vec3 H = normalize(L + V);
		
		// we use H to calculate the specular component
		float specAngle = max(dot(H, N), 0.0);
		// shininess application to the specular component
		float spec = pow(specAngle, shininess);
		
		// We add diffusive and specular components to the final color
		// N.B. ): in this implementation, the sum of the components can be different than 1
		color += vec3( kD * lambertian * surfaceColor.xyz + kS * spec * specular);
	}
	return color;
}

vec4 calculatePointLights()
{
	vec4 color = vec4(0);
	for(int i = 0; i < nPointLights; i++)
	{
		curr_twLightDir = fs_in.twPointLightDir[i];
		color += vec4(BlinnPhong(), 1.0f) * pointLights[i].color * pointLights[i].intensity;
	}
	//color = normalize(color);
	return color;
}

vec4 calculateDirLights()
{
	vec4 color = vec4(0);
	
	for(int i = 0; i < nDirLights; i++)
	{
		curr_twLightDir = fs_in.twDirLightDir[i];
		color += vec4(BlinnPhong(), 1.0f) * directionalLights[i].color * directionalLights[i].intensity;
	}
	//color = normalize(color);
	return color;
}

void main()
{
	vec4 color = vec4(0);
	
	color += calculatePointLights();
	color += calculateDirLights();
	
	colorFrag = color;
}