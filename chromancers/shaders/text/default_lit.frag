#version 430 core

// Complex fragment shader which samples various textures 
// and computes lighting from directional and pointlights

// output variable for the fragment shader
out vec4 colorFrag;

in VS_OUT 
{
	// world space data
	vec3 wFragPos;
	vec3 wPointLightDir[MAX_POINT_LIGHTS];
	vec3 wDirLightDir  [MAX_DIR_LIGHTS];

	// view space data
	vec3 vwFragPos;

	// tangent space data
	vec3 twFragPos; // Local -> World (w) -> Tangent (t) position of frag N.B. THE FIRST LETTER SPECIFIES THE FINAL SPACE 
	vec3 twPointLightDir[MAX_POINT_LIGHTS];
	vec3 twDirLightDir  [MAX_DIR_LIGHTS];
	vec3 twCameraPos; 
	vec3 twNormal;

	// light space data
	vec4 lwDirFragPos[MAX_DIR_LIGHTS];

	// the output variable for UV coordinates
	vec2 interp_UV;
} fs_in;

//Lights
uniform uint nPointLights;
uniform uint nDirLights;

uniform PointLight       pointLights      [MAX_POINT_LIGHTS];
uniform DirectionalLight directionalLights[MAX_DIR_LIGHTS];

// Textures
// texture samplers
uniform sampler2D diffuse_map        ; // TexUnit0 Main material color
uniform sampler2D normal_map         ; // TexUnit1 Normals for detail and light computation
uniform sampler2D displacement_map   ; // TexUnit2 Emulated vertex displacement (also known as height/depth map)
uniform sampler2D detail_diffuse_map ; // TexUnit3 Secondary material color
uniform sampler2D detail_normal_map  ; // TexUnit4 Secondary material color

uniform sampler2D directional_shadow_maps[MAX_DIR_LIGHTS]; // TexUnit5 Shadow map 0
uniform samplerCube point_shadow_maps[MAX_POINT_LIGHTS]; // TexUnit??

uniform int sample_diffuse_map        = 0;
uniform int sample_normal_map         = 0;
uniform int sample_displacement_map   = 0;
uniform int sample_detail_diffuse_map = 0;
uniform int sample_detail_normal_map  = 0;

uniform int sample_shadow_map  = 0; // receive shadows or not

uniform float uv_repeat = 1; // texture repetitions

// Detail map attributes
uniform float detail_alpha_threshold = 0.7f;
uniform float detail_diffuse_bias = 1.75f;
uniform float detail_normal_bias = 0.05f;

// Material-light attributes
uniform vec4 ambient_color ;
uniform vec4 diffuse_color ;
uniform vec4 specular_color;

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
uniform float parallax_heightscale;

// Current light position
vec3 curr_twLightDir;

// Computed stuff
vec3 twViewDir;
vec2 finalTexCoords;
vec3 finalNormal;

vec2 CheapParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
	// Sample the heightmap
    float height = texture(displacement_map, texCoords).r; 
	// Calculate vector towards approximate height
	vec2 p = viewDir.xy * (height * parallax_heightscale);
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
    vec2 P = viewDir.xy / viewDir.z * parallax_heightscale; 
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(displacement_map, currentTexCoords).r;
      
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(displacement_map, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(displacement_map, prevTexCoords).r - currentLayerDepth + layerDepth;
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec2 calculateTexCoords(vec2 texCoords, vec3 viewDir)
{
	//Repeated UV coords
	vec2 computedTexCoords = texCoords * uv_repeat;

	// branchless condition to avoid divergence
	computedTexCoords = ((1 - sample_displacement_map) * computedTexCoords) + // no displacement map
	                    ((sample_displacement_map)     * CheapParallaxMapping(computedTexCoords,  viewDir));  // use displacement map
	
	return computedTexCoords;
}

vec4 calculateSurfaceColor(vec2 texCoords)
{
	// branchless condition to avoid divergence
	vec4 surface_color = ((1 - sample_diffuse_map) * diffuse_color) + // use albedo (diffuse color)
	                     ((sample_diffuse_map)     * texture(diffuse_map, texCoords)); // use diffuse map

	return surface_color;
}

vec4 calculateDetailColor(vec2 texCoords)
{
	// branchless condition to avoid divergence
	vec4 detail_color = ((1 - sample_detail_diffuse_map) * 0) + // don't consider detail
	                    ((sample_detail_diffuse_map)     * texture(detail_diffuse_map, texCoords)); // use detail map

	return detail_color;
}

vec3 calculateNormal(sampler2D normal_map, int sample_normal_map, vec3 vertexNormal, vec2 texCoords)
{
	// branchless condition to avoid divergence
	vec3 normal = ((1 - sample_normal_map) * normalize(vertexNormal)) + // use vertex normal
	              ((sample_normal_map)     * normalize(texture(normal_map, texCoords).rgb * 2.0 - 1.0)); // use normal map

	return normal;
}

// Simple averaging filter to soften the edges of a texture read (usually used with shadowmaps)
vec4 texturePCF(sampler2D map, vec2 interp_UV)
{
	vec4 sampled = vec4(0);

	vec2 texelSize = 1.0 / textureSize(map, 0);
	for(int x = -1; x <= 1; ++x)
	{
		for(int y = -1; y <= 1; ++y)
		{
			sampled += texture(map, interp_UV + vec2(x, y) * texelSize); 
		}    
	}
	sampled /= 9.0;

	return sampled;
}

// Compute shadow from a directional shadow map
float calculateShadow(sampler2D shadow_map, vec4 lwFragPos, vec3 wLightDir, vec3 normal)
{
	// perform perspective divide
    vec3 projCoords = lwFragPos.xyz / lwFragPos.w;

	// normalize coordinates to be within [0,1] instead of NDC range [-1,1]
	projCoords = projCoords * 0.5 + 0.5; 

	// get fragment depth
	float currentDepth = projCoords.z;  

	// calculate bias to solve shadow acne 
	float shadow_factor = dot(normal, wLightDir);
	float bias = max(0.01 * (1.0 - shadow_factor), 0.005);

	// sample the depthmap obtained from the light POV
	float closestDepth = texture(shadow_map, projCoords.xy).r;
	// if fragment has greater depth, then something occluded it from light POV, thus is in shadow
	float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0; 
	
	// PCF-filtered shadow sampling
	shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadow_map, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
			// sample the depthmap obtained from the light POV through the filtering mask
            float pcfDepth = texture(shadow_map, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
	// Average-out the filtering mask
    shadow /= 9.0;

	// don't affect areas outside the shadow_map
	if(projCoords.z > 1.0)
        shadow = 0.0;

	return shadow;
}

// Compute shadow from a pointlight shadow cube
float calculateShadow(samplerCube shadow_cube, vec3 wFragPos, vec3 wLightPos, float far_plane /* TODO temp*/)
{
    // get vector between fragment position and light position
    vec3 fragToLight = wFragPos - wLightPos;
    // use the light to fragment vector to sample from the depth map    
    float closestDepth = texture(shadow_cube, fragToLight).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // now test for shadows
    float bias = 0.05; 
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;

	// No PCF for shadow cube or performance tanks :(

    return shadow;
} 

// Simple blinn phong lighting solution
vec3 BlinnPhong()
{
	float shininess_factor = shininess;

	// ambient component 
	vec3 ambient_component = kA * ambient_color.rgb;

	// view direction
	vec3 V = twViewDir;

    vec2 final_texCoords = finalTexCoords;

	// obtain normal from primary normal map
	vec3 N = finalNormal;
	vec4 surface_color = diffuse_color;
	vec4 diffuse_map_color = texture(diffuse_map, final_texCoords);

	vec4 detail_diffuse_color = texturePCF(detail_diffuse_map, fs_in.interp_UV);

	if(sample_diffuse_map == 1)
		surface_color = diffuse_map_color;

	if(sample_detail_diffuse_map == 1 && sample_detail_normal_map == 1)
	{
		if(detail_diffuse_color.a > detail_alpha_threshold)
		{
			float ft = detail_diffuse_color.a;
			N += calculateNormal(detail_normal_map, sample_detail_normal_map, fs_in.twNormal, finalTexCoords) * detail_normal_bias * ft;
			surface_color = mix(surface_color, detail_diffuse_color * detail_diffuse_bias, ft);
			
			float detail_shininess = 512.f;
			shininess_factor = mix(shininess_factor, detail_shininess, ft);
		}
	}
	
	// normalization of the per-fragment light incidence direction
	vec3 L = normalize(curr_twLightDir);
	
	// Lambert coefficient
	N = normalize(N);
	float lambertian = max(dot(L,N), 0.0);
	
	vec3 final_color = ambient_component;

	// if the lambert coefficient is positive, then we calculate the specular component
	if(lambertian > 0.0)
	{
		// in the Blinn-Phong model we do not use the reflection vector, but the half vector
		vec3 H = normalize(L + V);
		// we use H to calculate the specular component
		float specAngle = max(dot(H, N), 0.0);
		// shininess application to the specular component
		float spec = pow(specAngle, shininess_factor);

		// calculate diffuse component
		vec3 diffuse_component = kD * lambertian * surface_color.rgb;

		// calculate specular component
		vec3 specular_component = kS * spec * specular_color.rgb;
		
		// We add diffusive and specular components to the final color
		// N.B. ): in this implementation, the sum of the components can be different than 1
		final_color += (diffuse_component + specular_component);
	}
	return final_color;
}

// Compute fragment's lighting from pointlights 
vec3 calculatePointLights()
{
	vec3 color = vec3(0);

	for(int i = 0; i < nPointLights; i++)
	{
		curr_twLightDir = normalize(fs_in.twPointLightDir[i]);
		float light_distance = length(fs_in.wPointLightDir[i]);
		float attenuation = 0.001f + // to avoid division by zero
							pointLights[i].attenuation_constant +
							pointLights[i].attenuation_linear * light_distance +
							pointLights[i].attenuation_quadratic * light_distance * light_distance;

		float shadow = calculateShadow(point_shadow_maps[i], fs_in.wFragPos, pointLights[i].position, 25.f) * sample_shadow_map;

		color += (1 - shadow) * BlinnPhong() * pointLights[i].color.rgb * pointLights[i].intensity * (1.f/attenuation);
	}

	return color;
}

// Compute fragment's lighting from directional lights 
vec3 calculateDirLights()
{
	vec3 color = vec3(0);
	
	for(int i = 0; i < nDirLights; i++)
	{
		curr_twLightDir = normalize(fs_in.twDirLightDir[i]);

		float shadow = calculateShadow(directional_shadow_maps[i], fs_in.lwDirFragPos[i], fs_in.wDirLightDir[i], finalNormal) * sample_shadow_map;

		color += (1 - shadow) * BlinnPhong() * directionalLights[i].color.rgb * directionalLights[i].intensity;
	}

	return color;
}

void main()
{
	vec3 color = vec3(0);

	twViewDir = normalize( fs_in.twCameraPos - fs_in.twFragPos );
	finalTexCoords = calculateTexCoords(fs_in.interp_UV, twViewDir);
	finalNormal = calculateNormal(normal_map, sample_normal_map, fs_in.twNormal, finalTexCoords);
	
	color += calculatePointLights();
	color += calculateDirLights();

	colorFrag = vec4(color, 1.0f);
}